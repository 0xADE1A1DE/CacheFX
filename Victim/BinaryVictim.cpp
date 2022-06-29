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

#include <memory>
#include <string>
using namespace std;

#include <MMU/MMU.h>
#include <MemHandle/MemHandle.h>
#include <PlaintextKeyPairGenerator/BinaryPlaintextKeyPairGenerator.h>
#include <Random.h>
#include <Victim/BinaryVictim.h>
#include <Victim/Victim.h>
#include <types.h>

BinaryVictim::BinaryVictim(MMU* mmu, CLArgs clargs)
{
  const size_t cacheSize = mmu->getCache()->getNLines() * CACHE_LINE_SIZE;
  memHandle = mmu->allocate("BINARYVICTIMBUFFER", cacheSize,
                            DEFAULT_CACHE_CONTEXT_VICTIM, true);
  keyGenerator = make_unique<BinaryPlaintextKeyPairGenerator>();
  memHandle->setAccessType(clargs.get_accessType());
  addressA = rand() % cacheSize;
  do
  {
    addressB = rand() % cacheSize;
  } while (abs(long(addressB - addressA)) < 64);
  memHandle->setVictimTargetAddress(addressA);
}

void BinaryVictim::setKey(const uint8_t* userKey) { key = *userKey; }

void BinaryVictim::cipher(const uint8_t* in, uint8_t* out) { accessAddress(); }

int32_t BinaryVictim::accessAddress()
{
  uint64_t address;

  if (!key)
    address = addressB;
  else
    address = addressA;

  // printf("Key: %d, Accessing address: %0lx\n", key, address);
  return memHandle->read(address);
}

void BinaryVictim::invalidateAddress() const
{
  if (!key)
    memHandle->flush(addressA);
  else
    memHandle->flush(addressB);
}

int32_t BinaryVictim::hasCollision(MemHandle* memHandle,
                                   address_t address) const
{
  return memHandle->hasCollision(address, memHandle, addressA);
}

uint64_t BinaryVictim::getUniqueVictimTags()
{
  return memHandle->getUniqueTags();
}
void BinaryVictim::clearUniqueVictimTags() { memHandle->clearUniqueTags(); }

void BinaryVictim::setAttackerEvictionSet(unordered_set<tag_t>* evSet)
{
  memHandle->setEvSet(evSet);
}
uint64_t BinaryVictim::getAttackerAddressesEvicted() const
{
  return memHandle->getAttackerAddressesEvicted();
}
void BinaryVictim::resetAttackerAddressesEvicted()
{
  memHandle->resetAttackerAddressesEvicted();
}

uint64_t BinaryVictim::getCorrectEvictions()
{
  return memHandle->getCorrectEvictions();
}

void BinaryVictim::clearCorrectEvictions()
{
  memHandle->clearCorrectEvictions();
}
uint64_t BinaryVictim::getIncorrectEvictions()
{
  return memHandle->getIncorrectEvictions();
}
void BinaryVictim::clearIncorrectEvictions()
{
  memHandle->clearIncorrectEvictions();
}
