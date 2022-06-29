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

#ifndef __SQUAREMULTVICTIM_H__
#define __SQUAREMULTAICTIM_H__ 1

#include <Attacker/Attacker.h>
#include <MemArray.h>

class SquareMultVictim : public Victim
{
private:
  uint32_t size;
  uint32_t keysize;
  int32_t secbit;

  Attacker* attacker;

  MemArray<uint16_t> key;
  MemArray<uint16_t> modulus;

  MemArray<uint16_t> base;
  MemArray<uint16_t> acc;
  MemArray<uint16_t> scratch;

  MemHandle* code;

  uint8_t secret;

  void modexp() const;
  void square() const; // Squares scratch, stores result in acc
  void mul() const;    // Multiplies scratch by base, stores result in acc
  void mod() const;    // Reduce acc modulo modulo, stores result in scratch

  uint32_t Step_D3(int32_t j) const;
  uint16_t Step_D4(int32_t j, uint32_t qhat) const;
  void Step_D6(int32_t j) const;

  void flush() const
  {
    key.flush();
    modulus.flush();
    base.flush();
    acc.flush();
    scratch.flush();
  }

public:
  SquareMultVictim(MMU* mmu, Attacker* attacker, uint32_t keysize,
                   uint32_t size, uint16_t* modulus, int32_t secbit);
  SquareMultVictim(MMU* mmu, Attacker* attacker, uint32_t keysize,
                   uint32_t size, uint16_t* modulus, int32_t secbit,
                   CLArgs clargs);

  virtual int32_t getKeySize(void) const { return keysize * 2; };

  virtual int32_t getInputSize(void) const { return size * 2; };
  virtual int32_t getOutputSize(void) const { return size * 2; };

  // external numbers are represented in
  virtual void setKey(const uint8_t* secret);
  virtual uint8_t getSecret() { return secret; };
  virtual void cipher(const uint8_t* input, uint8_t* output);

  int32_t accessAddress();
  void invalidateAddress() const;
  int32_t hasCollision(MemHandle* memHandle, address_t address) const;

  virtual uint64_t getUniqueVictimTags();
  virtual void clearUniqueVictimTags();

  virtual void setAttackerEvictionSet(unordered_set<tag_t>* evSet);
  virtual uint64_t getAttackerAddressesEvicted() const;
  virtual void resetAttackerAddressesEvicted();

  virtual uint64_t getCorrectEvictions();
  virtual void clearCorrectEvictions();

  virtual uint64_t getIncorrectEvictions();
  virtual void clearIncorrectEvictions();

  virtual ~SquareMultVictim(){};
};

#endif // __SQUAREMULTVICTIM_H__
