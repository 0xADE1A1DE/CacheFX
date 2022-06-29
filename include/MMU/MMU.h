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

#ifndef __MMU_H__
#define __MMU_H__ 1

#include <CacheContext.h>
#include <MemHandle/MemHandle.h>
#include <string>

class MMU
{
private:
  static const int32_t defaultAlign = 128;

public:
  virtual ~MMU(){};
  virtual MemHandle* allocate(std::string name, address_t size, address_t align,
                              address_t fix, CacheContext context,
                              bool pub) = 0;
  MemHandle* allocate(std::string name, address_t size, CacheContext context,
                      bool pub)
  {
    return allocate(name, size, defaultAlign, 0, context, pub);
  };
  MemHandle* allocate(std::string name, address_t size, bool pub)
  {
    return allocate(name, size, defaultAlign, 0, DEFAULT_CONTEXT, pub);
  };
  virtual void free(MemHandle* handle) = 0;

  virtual Cache* getCache() const = 0;
};

#endif // __MMU_H__
