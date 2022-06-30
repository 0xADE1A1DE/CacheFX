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

// Implementation of a fully associative cache with LRU replacement policy

#ifndef __ASSOCCACHE_H__
#define __ASSOCCACHE_H__ 1

#include "Cache.h"

struct cacheEntry
{
  tag_t tag;
  uint32_t accessTime;
  uint32_t flags;
  CacheContext context;
};

class AssocCache : public Cache
{
public:
  static const char* CACHE_TYPESTR;

protected:
  cacheEntry* entries;
  int32_t bitPLRUBitsSet;
  int32_t treePLRULevels;
  bool* treePLRU;
  bool* bitPLRU;
  int32_t* srrip;
  uint32_t srripM;
  int32_t size;
  uint32_t clock;
  replAlg algorithm;
  uint32_t psel_bits;
  uint32_t psel;
  double bip_throttle;
  double brrip_long_chance;
  bool invalidFirst;

  void access(int32_t way, bool newEntry = false);

public:
  AssocCache(int32_t size) : AssocCache(REPL_LRU, size){};
  AssocCache(replAlg alg, int32_t size) : AssocCache(alg, size, true){};
  AssocCache(replAlg alg, int32_t size, bool invalidFirst);
  virtual ~AssocCache();

  int32_t hasCollision(tag_t cl1, const CacheContext& ctx1, tag_t cl2,
                       const CacheContext& ctx2) const override;

  int32_t getSize() const { return size; };
  tag_t getTag(int32_t way) const { return entries[way].tag; };
  void setTag(int32_t way, tag_t tag)
  {
    entries[way].tag = tag;
    access(way);
  };

  void setAlgorithm(replAlg alg);
  replAlg getAlgorithm(void) const override { return algorithm; };

  const char* getCacheType() const override { return CACHE_TYPESTR; }

  size_t getNLines() const override { return size; };
  size_t getNSets() const { return 1; };
  size_t getNWays() const { return size; };
  size_t getEvictionSetSize() const { return size; };
  size_t getGHMGroupSize() const { return 1; };

protected:
  int32_t readCl(tag_t cl, const CacheContext& context,
                 std::list<CacheResponse>& response) override;
  int32_t evictCl(tag_t cl, const CacheContext& context,
                  std::list<CacheResponse>& response) override;
};

#endif // __ASSOCCACHE_H__
