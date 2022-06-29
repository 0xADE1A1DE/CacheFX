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
#include <cmath>
#include <cstdlib>
#include <iostream>

#include <Cache/AssocCache.h>
#include <types.h>

const char* AssocCache::CACHE_TYPESTR = "associative";

AssocCache::AssocCache(replAlg algo, int32_t size, bool invalidFirst)
    : size(size), algorithm(algo), invalidFirst(invalidFirst)
{
  entries = new cacheEntry[size];
  treePLRU = new bool[size];
  bitPLRU = new bool[size];

  treePLRULevels = (int32_t)log2(size);
  if ((algorithm == REPL_TREE_PLRU) &&
      ((int32_t)pow(2, (double)treePLRULevels) != size))
  {
    algorithm = REPL_BIT_PLRU;
  }

  bitPLRUBitsSet = 0;
  clock = 0;

  for (int32_t i = 0; i < size; i++)
  {
    entries[i].tag = TAG_INIT;
    entries[i].accessTime = clock++;
    entries[i].flags = 0;
    entries[i].context = DEFAULT_CACHE_CONTEXT;
    treePLRU[i] = false;
    bitPLRU[i] = false;
  }
}

AssocCache::~AssocCache()
{
  delete[] entries;
  delete[] treePLRU;
  delete[] bitPLRU;
}

void AssocCache::access(int32_t way)
{
  entries[way].accessTime = clock++;
  if (!bitPLRU[way])
  {
    bitPLRUBitsSet++;
    if (bitPLRUBitsSet == size)
    {
      for (int32_t i = 0; i < size; i++)
      {
        bitPLRU[i] = false;
      }
      bitPLRUBitsSet = 1;
    }
    bitPLRU[way] = true;
  }
  uint32_t nodeOffset = 0;
  uint32_t levelOffset = 1;
  for (int32_t i = 0; i < treePLRULevels; i++)
  {
    int32_t bit = (way >> (treePLRULevels - 1 - i)) & 0x01;
    treePLRU[levelOffset - 1 + nodeOffset] = !bit;
    nodeOffset = nodeOffset * 2 + bit;
    levelOffset = levelOffset * 2;
  }
}

int32_t AssocCache::readCl(tag_t cl, const CacheContext& context,
                           std::list<CacheResponse>& response)
{
  int32_t free = -1;
  for (int32_t i = 0; i < size; i++)
  {
    if (entries[i].tag == cl)
    {
      access(i);
      response.push_back(CacheResponse(true));
      return 1;
    }
    if (entries[i].tag == TAG_NONE)
      free = i;
  }

  if (free != -1 && invalidFirst)
  {
    entries[free].tag = cl;
    response.push_back(CacheResponse(false));
    access(free);
    return 0;
  }

  switch (algorithm)
  {
  case REPL_LRU: {
    // Search for oldest entry.  Integer overflow can break LRU in some extreme
    // cases
    uint32_t oldesttime = 0;
    for (int32_t i = 0; i < size; i++)
    {
      uint32_t time = clock - entries[i].accessTime;
      if (time > oldesttime)
      {
        oldesttime = time;
        free = i;
      }
    }
    break;
  }
  case REPL_BIT_PLRU: {
    int32_t i = 0;
    if (size == 1)
    {
      free = 0;
    }
    else
    {
      while (i < size && bitPLRU[i])
        i++;
      assert(i < size);
      free = i;
    }
    break;
  }
  case REPL_TREE_PLRU: {
    uint32_t nodeOffset = 0;
    uint32_t levelOffset = 1;
    for (int32_t i = 0; i < treePLRULevels; i++)
    {
      bool bit = treePLRU[levelOffset - 1 + nodeOffset];
      nodeOffset = nodeOffset * 2 + bit;
      levelOffset = levelOffset * 2;
    }
    free = nodeOffset;
    break;
  }
  case REPL_RANDOM:
    free = random() % size;
    break;
  }
  response.push_back(CacheResponse(false, entries[free].tag));
  entries[free].tag = cl;
  access(free);
  return 0;
}

int32_t AssocCache::evictCl(tag_t cl, const CacheContext& context,
                            std::list<CacheResponse>& response)
{
  for (int32_t i = 0; i < size; i++)
  {
    if (entries[i].tag == cl)
    {
      entries[i].tag = TAG_NONE;
      response.push_back(CacheResponse(true, cl));
      return 1;
    }
  }
  response.push_back(CacheResponse(false));
  return 0;
}

void AssocCache::setAlgorithm(replAlg alg)
{
  algorithm = alg;
  if ((algorithm == REPL_TREE_PLRU) &&
      ((int32_t)pow(2, (double)treePLRULevels) != size))
  {
    algorithm = REPL_BIT_PLRU;
  }
  for (int32_t i = 0; i < size; i++)
  {
    entries[i].accessTime = 0;
    bitPLRU[i] = false;
    treePLRU[i] = false;
  }
}

int32_t AssocCache::hasCollision(tag_t cl1, const CacheContext& ctx1, tag_t cl2,
                                 const CacheContext& ctx2) const
{
  return 1;
}
