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
#include <cstdlib>

#include <Cache/PLcache.h>
#include <types.h>

const char* AssocPLcache::CACHE_TYPESTR = "associativePL";
const char* PLcache::CACHE_TYPESTR = "PLcache";

int32_t AssocPLcache::readCl(tag_t cl, const CacheContext& context,
                             std::list<CacheResponse>& response)
{

  int32_t free = -1;
  for (int32_t i = 0; i < size; i++)
  {
    if (entries[i].tag == cl)
    {
      access(i);
      setContext(i, context);
      response.push_back(CacheResponse(true));
      return 1;
    }
    if (entries[i].tag == TAG_NONE)
      free = i;
  }

  if (free != -1)
  {
    response.push_back(CacheResponse(false));
    entries[free].tag = cl;
    access(free);
    setContext(free, context);
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
  case REPL_RANDOM:
    free = random() % size;
    break;
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
  }

  if (getContext(free).getCoreId() == context.getCoreId())
  {
    response.push_back(CacheResponse(false, entries[free].tag));
    entries[free].tag = cl;
    access(free);
  }
  else
  {
    // Verify this later: this fixes reading cacherespons.back() when no
    // previous reponse wash added to the vector.
    response.push_back(CacheResponse(false));
    access(free); // move the victim cl to MRU position, no replacement
  }
  return 0;
}

int32_t AssocPLcache::evictCl(tag_t cl, const CacheContext& context,
                              std::list<CacheResponse>& response)
{
  for (int32_t i = 0; i < size; i++)
  {
    if (entries[i].tag == cl &&
        context.getCoreId() == getContext(i).getCoreId())
    {
      response.push_back(CacheResponse(true, cl));
      entries[i].tag = TAG_NONE;
      return 1;
    }
  }
  response.push_back(CacheResponse(false));
  return 0;
}

int32_t AssocPLcache::hasCollision(tag_t cl1, const CacheContext& ctx1,
                                   tag_t cl2, const CacheContext& ctx2) const
{
  return 1;
}

PLcache::PLcache(replAlg alg, int32_t nsets, int32_t nways)
    : nsets(nsets), nways(nways)
{
  sets = new AssocPLcache*[nsets];

  for (int32_t i = 0; i < nsets; i++)
    sets[i] = new AssocPLcache(alg, nways);

  algorithm = alg;
}

PLcache::~PLcache()
{
  for (int32_t i = 0; i < nsets; i++)
    delete sets[i];

  delete[] sets;
}

void PLcache::setAlgorithm(replAlg alg)
{
  for (int32_t i = 0; i < nsets; i++)
    sets[i]->setAlgorithm(alg);

  if ((uintptr_t)sets > 0)
  {
    algorithm = sets[0]->getAlgorithm();
  }
  else
  {
    algorithm = alg;
  }
}

int32_t PLcache::hasCollision(tag_t cl1, const CacheContext& ctx1, tag_t cl2,
                              const CacheContext& ctx2) const
{
  AssocCache* assoc1 = this->getSet(cl1);
  AssocCache* assoc2 = this->getSet(cl2);
  if (assoc1 == assoc2)
  {
    return assoc1->hasCollision(cl1, ctx1, cl2, ctx2);
  }
  return 0;
}
