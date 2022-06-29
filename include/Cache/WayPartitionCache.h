/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Sep 24, 2019
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

#ifndef WAYPARTITIONCACHE_H_
#define WAYPARTITIONCACHE_H_

#include "SetAssocCache.h"

class WayPartitionCache : public Cache
{
public:
  static const char* CACHE_TYPESTR;

protected:
  SetAssocCache* _cacheDomain0;
  SetAssocCache* _cacheDomain1;
  CacheContext _cacheContext0;
  CacheContext _cacheContext1;

public:
  WayPartitionCache(int32_t nsets, int32_t nways)
      : WayPartitionCache(REPL_LRU, nsets, nways, 1){};
  WayPartitionCache(replAlg alg, int32_t nsets, int32_t nways,
                    int32_t nsecways);
  virtual ~WayPartitionCache();

  size_t getNSets() const override { return _cacheDomain0->getNSets(); };
  size_t getNWays() const override
  {
    return _cacheDomain0->getNWays() + _cacheDomain1->getNWays();
  };
  size_t getEvictionSetSize() const override
  {
    return _cacheDomain0->getNWays();
  };
  size_t getGHMGroupSize() const override { return _cacheDomain0->getNWays(); };

  size_t getNSecWays() const { return _cacheDomain1->getNWays(); };

  void setAlgorithm(replAlg alg)
  {
    _cacheDomain0->setAlgorithm(alg);
    _cacheDomain1->setAlgorithm(alg);
  };
  replAlg getAlgorithm() const override
  {
    return _cacheDomain0->getAlgorithm();
  }

  const char* getCacheType() const override { return CACHE_TYPESTR; }

  size_t getNLines() const override { return getNSets() * getNWays(); }

  int32_t hasCollision(tag_t cl1, const CacheContext& ctx1, tag_t cl2,
                       const CacheContext& ctx2) const override;

protected:
  int32_t readCl(tag_t cl, const CacheContext& context,
                 std::list<CacheResponse>& response) override;
  int32_t evictCl(tag_t cl, const CacheContext& context,
                  std::list<CacheResponse>& response) override;
  int32_t writeCl(tag_t cl, const CacheContext& context,
                  std::list<CacheResponse>& response) override;
  int32_t execCl(tag_t cl, const CacheContext& context,
                 std::list<CacheResponse>& response) override;
};

#endif /* WAYPARTITIONCACHE_H_ */
