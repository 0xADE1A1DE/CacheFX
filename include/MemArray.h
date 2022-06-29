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

#ifndef __MEMARRAY_H__
#define __MEMARRAY_H__ 1

#include <stdexcept>

#include "CacheContext.h"

#include <MMU/MMU.h>

template <class T> class MemArray
{
private:
  T* values;
  uint32_t size;
  MemHandle* mem;
  MMU* mmu;
  const uint32_t clsize;
  uint32_t* readmap;
  uint32_t* writemap;

  static uint32_t entry(uint32_t index, uint32_t clsize)
  {
    int32_t cacheindex = (index * sizeof(T)) / clsize;
    return cacheindex / 32;
  }

  static uint32_t mask(uint32_t index, uint32_t clsize)
  {
    int32_t cacheindex = (index * sizeof(T)) / clsize;
    return 1 << (cacheindex % 32);
  }

public:
  MemArray(MMU* mmu, const char* desc, uint32_t size, bool pub)
      : MemArray(mmu, desc, size, DEFAULT_CONTEXT, pub){};

  MemArray(MMU* mmu, const char* desc, uint32_t size, CacheContext context,
           bool pub)
      : MemArray(mmu, desc, size, context, pub, 0){};

  MemArray(MMU* mmu, const char* desc, uint32_t size, CacheContext context,
           bool pub, uint32_t clsize)
      : size(size), mmu(mmu), clsize(clsize)
  {
    mem = mmu->allocate(desc, size * sizeof(T), context, pub);
    values = new T[size];
    if (clsize != 0)
    {
      readmap =
          new uint32_t[(size * sizeof(T) + 32 * clsize - 1) / (32 * clsize)];
      writemap =
          new uint32_t[(size * sizeof(T) + 32 * clsize - 1) / (32 * clsize)];
    }
    else
    {
      readmap = writemap = NULL;
    }
  }

  ~MemArray()
  {
    delete[] values;
    mmu->free(mem);
    if (readmap != NULL)
      delete[] readmap;
    if (writemap != NULL)
      delete[] writemap;
  }

  // Is there a way to overload the [] operator and still
  // distinguish read from write access?

  T get(uint32_t index) const
  {
    if (index >= size)
      throw std::out_of_range("MemArray::get");
    if (clsize == 0)
      mem->read(index * sizeof(T));
    else
      readmap[entry(index, clsize)] |= mask(index, clsize);
    return values[index];
  }

  // const here is a bit ugly, but we only care about changes to secret, so
  // that's OK here...
  void set(uint32_t index, T value) const
  {
    if (index >= size)
      throw std::out_of_range("MemArray::set");
    if (clsize == 0)
      mem->write(index * sizeof(T));
    else
      writemap[entry(index, clsize)] |= mask(index, clsize);
    values[index] = value;
  }

  void flush() const
  {
    if (clsize == 0)
      return;
    int32_t maxindex = (size * sizeof(T)) / clsize;
    for (int32_t i = 0; i < maxindex; i++)
    {
      if ((readmap[entry(i, clsize)] & mask(i, clsize)) != 0)
        mem->read(i * clsize);
      if ((writemap[entry(i, clsize)] & mask(i, clsize)) != 0)
        mem->write(i * clsize);
    }
    for (int32_t i = 0; i < maxindex; i += 32)
      readmap[i / 32] = writemap[i / 32] = 0;
  }

  void setComparatorMemHandle(MemHandle* const memHandle)
  {
    mem->setComparatorMemHandle(memHandle);
  }

  MemHandle* getMem() const { return mem; };
};

#endif // __MEMARRAY_H__
