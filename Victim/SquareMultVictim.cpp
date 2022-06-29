/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cassert>
#include <string>
using namespace std;

#include <Attacker/Attacker.h>
#include <MMU/MMU.h>
#include <MemArray.h>
#include <MemHandle/MemHandle.h>
#include <PlaintextKeyPairGenerator/PlaintextKeyPairGenerator.h>
#include <PlaintextKeyPairGenerator/SquareMultPlaintextKeyPairGenerator.h>
#include <Random.h>
#include <Victim/SquareMultVictim.h>
#include <Victim/Victim.h>
#include <types.h>

SquareMultVictim::SquareMultVictim(MMU* mmu, Attacker* attacker,
                                   uint32_t keysize, uint32_t size,
                                   uint16_t* mod, int32_t secbit)
    : size(size), keysize(keysize), secbit(secbit), attacker(attacker),
      key(mmu, "Key", keysize, DEFAULT_CACHE_CONTEXT_VICTIM, false, 64),
      modulus(mmu, "Modulus", size, DEFAULT_CACHE_CONTEXT_VICTIM, false, 64),
      base(mmu, "Base", size, DEFAULT_CACHE_CONTEXT_VICTIM, false, 64),
      acc(mmu, "Acc", size * 2 + 1, DEFAULT_CACHE_CONTEXT_VICTIM, false, 64),
      scratch(mmu, "Scratch", size * 2 + 1, DEFAULT_CACHE_CONTEXT_VICTIM, false,
              64)
{
  int32_t t = mod[size - 1];
  if (!(t & 0x8000))
    throw("SquareMultVictim: modulus must cover the full bit size");

  // key = new MemArray<uint16_t>(mmu, "Key", size, false);
  for (uint32_t i = 0; i < size; i++)
    modulus.set(i, mod[i]);

  keyGenerator =
      make_unique<SquareMultPlaintextKeyPairGenerator>(keysize, size, secbit);

  key.setComparatorMemHandle(base.getMem());
  modulus.setComparatorMemHandle(base.getMem());
  acc.setComparatorMemHandle(base.getMem());
  scratch.setComparatorMemHandle(base.getMem());
}

SquareMultVictim::SquareMultVictim(MMU* mmu, Attacker* attacker,
                                   uint32_t keysize, uint32_t size,
                                   uint16_t* mod, int32_t secbit, CLArgs clargs)
    : SquareMultVictim(mmu, attacker, keysize, size, mod, secbit)
{
  const AccessType accessType = clargs.get_accessType();
  key.getMem()->setAccessType(accessType);
  modulus.getMem()->setAccessType(accessType);
  acc.getMem()->setAccessType(accessType);
  scratch.getMem()->setAccessType(accessType);
}

void SquareMultVictim::setKey(const uint8_t* userKey)
{
  uint16_t* uk16 = (uint16_t*)userKey;
  for (uint32_t i = 0; i < keysize; i++)
  {
    key.set(i, uk16[i]);
  }

  secret = (userKey[secbit / 8] >> (secbit % 8)) & 1;
}

void SquareMultVictim::cipher(const uint8_t* in, uint8_t* out)
{
  // Copy base
  uint16_t* in16 = (uint16_t*)in;
  for (uint32_t i = 0; i < size; i++)
    base.set(i, in ? in16[i] : 0);

  modexp();

  uint16_t* out16 = (uint16_t*)out;
  if (out)
    for (int32_t i = 0; i < size; i++)
      out16[i] = acc.get(i);
}

void SquareMultVictim::modexp() const
{
  for (uint32_t i = 0; i < size * 2 + 1; i++)
    scratch.set(i, 0);
  for (uint32_t i = 1; i < size; i++)
    acc.set(i, 0);
  acc.set(0, 1);

  for (int32_t i = keysize; i-- > 0;)
  {
    uint16_t k = key.get(i);
    for (uint16_t m = 0x8000; m; m >>= 1)
    {
      square(); // Squares acc --> scratch
      mod();    // Reduce scratch --> acc
      if (k & m)
      {
        mul(); // mul acc*base --> scratch
        mod(); // reduce scratch-->acc
      }
      flush();
      attacker->round();
    }
  }
}

// Squares acc into scratch
void SquareMultVictim::square() const
{
  for (uint32_t i = 0; i < size * 2 + 1; i++)
    scratch.set(i, 0);

  for (uint32_t i = 0; i < size; i++)
  {
    uint64_t a = acc.get(i);
    uint64_t carry = scratch.get(i * 2) + a * a; // a^2
    scratch.set(i * 2, carry & 0xFFFFULL);
    carry >>= 16;
    for (uint32_t j = i + 1; j < size; j++)
    {
      carry += 2 * a * acc.get(j) + scratch.get(i + j);
      scratch.set(i + j, carry & 0xFFFFULL);
      carry >>= 16;
    }
    carry += scratch.get(i + size);
    scratch.set(i + size, carry & 0xFFFFULL);
  }
}

void SquareMultVictim::mul() const
{
  for (uint32_t i = 0; i < size * 2 + 1; i++)
    scratch.set(i, 0);

  for (uint32_t i = 0; i < size; i++)
  {
    uint32_t bd = base.get(i);
    uint32_t carry = 0;
    for (uint32_t j = 0; j < size; j++)
    {
      carry = carry + acc.get(j) * bd + scratch.get(i + j);
      scratch.set(i + j, carry & 0xFFFF);
      carry >>= 16;
    }
    scratch.set(size + i, carry);
  }
}

uint32_t SquareMultVictim::Step_D3(int32_t j) const
{
  uint16_t m1 = modulus.get(size - 1);
  uint16_t m2 = modulus.get(size - 2);
  uint16_t s2 = scratch.get(j + size - 2);

  uint32_t hat =
      ((uint32_t)scratch.get(j + size) << 16) + scratch.get(j + size - 1);
  uint32_t qhat = hat / m1;
  uint32_t rhat = hat % m1;

  while (qhat > 0x10000)
  {
    qhat--;
    rhat += m1;
  }
  if (qhat == 0x10000 || (size > 1 && (qhat * m2 > 0x10000 * rhat + s2)))
  {
    qhat--;
    rhat += m1;
    if (rhat < 0x10000 && size > 1 && (qhat * m2 > 0x10000 * rhat + s2))
    {
      qhat--;
      rhat += m1;
    }
  }
  return qhat;
}

uint16_t SquareMultVictim::Step_D4(int32_t j, uint32_t qhat) const
{
  uint32_t borrow = 0;
  for (uint32_t i = 0; i < size; i++)
  {
    borrow += scratch.get(j + i);
    borrow -= qhat * modulus.get(i);
    scratch.set(j + i, borrow & 0xFFFF);
    borrow >>= 16;
    if (borrow)
      borrow |= 0xFFFF0000; // The borrow is always non-positive...
  }
  borrow += scratch.get(j + size);
  scratch.set(size, borrow & 0xFFFF);
  return borrow >> 16; // The return value is 16 bits, so no need for extending
                       // the sign bit
}

void SquareMultVictim::Step_D6(int32_t j) const
{
  uint32_t carry = 0;
  for (uint32_t i = 0; i < size; i++)
  {
    carry += scratch.get(j + i);
    carry += modulus.get(i);
    scratch.set(j + i, carry & 0xFFFF);
    carry >>= 16;
  }
  carry += scratch.get(j + size);
  scratch.set(j + size, carry & 0xFFFF);
}

// Translation:
//   u --> scratch (numerator)
//   v --> modulus
//   d --> modhift
//   n --> size

void SquareMultVictim::mod() const
{
  // Mapping to  Knuth's variable names:
  //   scratch  -- u -- numerator
  //   modulus  -- v -- denominator
  //   acc      -- q -- quotient
  //   modshift -- d -- normalisation factor
  //   size     -- n -- length of denominator
  //   size     -- m -- length difference between numerator and denominator
  //   0x10000  -- b -- base
  //   Our modulus is guaranteed to have the top bit set (see constuctor)
  //   Hence, no need to do shifts.

  // Step D1 - gone.  Modulus must be full-size
  for (uint32_t i = 0; i < size; i++)
    acc.set(i, 0);
  // Steps D2, D7
  for (int32_t j = size; j >= 0; j--)
  {
    // Step D3
    uint32_t qhat = Step_D3(j);

    // Step D4
    uint16_t borrow = Step_D4(j, qhat);

    // Step D5
    acc.set(j, qhat);
    if (borrow)
    {
      // Step D6
      assert(qhat != 0);
      Step_D6(j);
      acc.set(j, acc.get(j) - 1);
    }
  }

  // Step D8 - no need for shift

  // We do, however, need to copy the result to x because we're lazy
  for (int32_t i = 0; i < size; i++)
    acc.set(i, scratch.get(i));
}

int32_t SquareMultVictim::accessAddress()
{
  MemHandle* mem = base.getMem();
  return mem->read(0);
}

void SquareMultVictim::invalidateAddress() const
{
  MemHandle* mem = base.getMem();
  mem->flush(0);
}

int32_t SquareMultVictim::hasCollision(MemHandle* memHandle,
                                       address_t address) const
{
  MemHandle* mem = base.getMem();
  return memHandle->hasCollision(address, mem, 0);
}

uint64_t SquareMultVictim::getUniqueVictimTags()
{
  return key.getMem()->getUniqueTags() + modulus.getMem()->getUniqueTags() +
         scratch.getMem()->getUniqueTags() + base.getMem()->getUniqueTags() +
         acc.getMem()->getUniqueTags();
}
void SquareMultVictim::clearUniqueVictimTags()
{
  key.getMem()->clearUniqueTags();
  modulus.getMem()->clearUniqueTags();
  scratch.getMem()->clearUniqueTags();
  base.getMem()->clearUniqueTags();
  acc.getMem()->clearUniqueTags();
}

void SquareMultVictim::setAttackerEvictionSet(unordered_set<tag_t>* evSet)
{
  key.getMem()->setEvSet(evSet);
  modulus.getMem()->setEvSet(evSet);
  scratch.getMem()->setEvSet(evSet);
  base.getMem()->setEvSet(evSet);
  acc.getMem()->setEvSet(evSet);
}
uint64_t SquareMultVictim::getAttackerAddressesEvicted() const
{
  return key.getMem()->getAttackerAddressesEvicted() +
         modulus.getMem()->getAttackerAddressesEvicted() +
         scratch.getMem()->getAttackerAddressesEvicted() +
         base.getMem()->getAttackerAddressesEvicted() +
         acc.getMem()->getAttackerAddressesEvicted();
}
void SquareMultVictim::resetAttackerAddressesEvicted()
{
  key.getMem()->resetAttackerAddressesEvicted();
  modulus.getMem()->resetAttackerAddressesEvicted();
  scratch.getMem()->resetAttackerAddressesEvicted();
  base.getMem()->resetAttackerAddressesEvicted();
  acc.getMem()->resetAttackerAddressesEvicted();
}

uint64_t SquareMultVictim::getCorrectEvictions()
{
  return key.getMem()->getCorrectEvictions() +
         modulus.getMem()->getCorrectEvictions() +
         scratch.getMem()->getCorrectEvictions() +
         base.getMem()->getCorrectEvictions() +
         acc.getMem()->getCorrectEvictions();
}

void SquareMultVictim::clearCorrectEvictions()
{
  key.getMem()->clearCorrectEvictions();
  modulus.getMem()->clearCorrectEvictions();
  scratch.getMem()->clearCorrectEvictions();
  base.getMem()->clearCorrectEvictions();
  acc.getMem()->clearCorrectEvictions();
}
uint64_t SquareMultVictim::getIncorrectEvictions()
{
  return key.getMem()->getIncorrectEvictions() +
         modulus.getMem()->getIncorrectEvictions() +
         scratch.getMem()->getIncorrectEvictions() +
         base.getMem()->getIncorrectEvictions() +
         acc.getMem()->getIncorrectEvictions();
}
void SquareMultVictim::clearIncorrectEvictions()
{
  key.getMem()->clearIncorrectEvictions();
  modulus.getMem()->clearIncorrectEvictions();
  scratch.getMem()->clearIncorrectEvictions();
  base.getMem()->clearIncorrectEvictions();
  acc.getMem()->clearIncorrectEvictions();
}