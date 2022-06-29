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
 
#ifndef __DIRECTMMU_H__
#define __DIRECTMMU_H__ 1

#include <Cache/Cache.h>
#include <MMU/MMU.h>
#include <MemHandle/MemHandle.h>
#include <map>
#include <memory>
#include <string>

class DirectMemHandle;

class DirectMMU : public MMU
{
private:
  address_t last;
  std::unique_ptr<Cache> cache;
  std::map<std::string, DirectMemHandle*> handles;

public:
  DirectMMU(std::unique_ptr<Cache> cache) : cache(std::move(cache))
  {
    last = 4096;
  };
  DirectMMU(std::unique_ptr<Cache> cache, bool randomize)
      : cache(std::move(cache))
  {
    if (randomize)
      last = (std::rand() % 1024) * 4096;
    else
      last = 4096;
  };
  virtual ~DirectMMU() override;
  Cache* getCache() const { return cache.get(); };

  using MMU::allocate;
  MemHandle* allocate(std::string name, address_t size, address_t align,
                      address_t fix, CacheContext context, bool pub);
  void free(MemHandle* handle);
};

#endif // __DirectMMU_H__
