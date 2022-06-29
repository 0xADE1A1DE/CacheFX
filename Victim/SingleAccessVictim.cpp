/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Jan 30, 2020
 *     Author: thomas
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

#include <MMU/MMU.h>
#include <MemHandle/MemHandle.h>
#include <PlaintextKeyPairGenerator/BinaryPlaintextKeyPairGenerator.h>
#include <Victim/SingleAccessVictim.h>
#include <iostream>
#include <memory>

using namespace std;

SingleAccessVictim::SingleAccessVictim(MMU* mmu, int32_t cacheSize)
    : _mmu(mmu), _address(0)
{
  _memHandle =
      _mmu->allocate("SAVictim", cacheSize, DEFAULT_CACHE_CONTEXT_VICTIM, true);
  keyGenerator = make_unique<BinaryPlaintextKeyPairGenerator>();
}

SingleAccessVictim::SingleAccessVictim(MMU* mmu, int32_t cacheSize,
                                       bool randomAddress)
    : SingleAccessVictim::SingleAccessVictim(mmu, cacheSize)
{
  if (randomAddress)
  {
    setAddress(rand() % cacheSize);
    _memHandle->setVictimTargetAddress(_address);
  }
}
SingleAccessVictim::~SingleAccessVictim()
{
  // TODO Auto-generated destructor stub
}

void SingleAccessVictim::setAddress(address_t addr) { _address = addr; }

address_t SingleAccessVictim::getAddress() const
{
  auto allocatedSize = _memHandle->getSize();
  address_t offset = _address % allocatedSize;
  return offset;
}

int32_t SingleAccessVictim::accessAddress()
{
  auto offset = getAddress();
  // printf("Accessing address: %0lx\n", offset);
  return _memHandle->read(offset);
}

int32_t SingleAccessVictim::accessAddress(std::list<CacheResponse>& response)
{
  auto offset = getAddress();
  return _memHandle->read(offset, response);
}

/*
const MemHandle *SingleAccessVictim::getMemHandle() const {
  return _memHandle;
}
*/

void SingleAccessVictim::invalidateAddress() const
{
  auto offset = getAddress();
  _memHandle->flush(offset);
}

int32_t SingleAccessVictim::hasCollision(MemHandle* memHandle,
                                         address_t address) const
{
  return memHandle->hasCollision(address, _memHandle, _address);
}
