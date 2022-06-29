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
 
#ifndef PHANTOMCACHE_H_
#define PHANTOMCACHE_H_

#include <Cache/SetAssocCache.h>

class PhantomCache : public SetAssocCache
{
public:
  static const char* CACHE_TYPESTR;

  PhantomCache(int32_t nsets, int32_t nways, int32_t r)
      : PhantomCache(REPL_LRU, nsets, nways,
                     r){}; // r is the number of randomized sets
  PhantomCache(replAlg alg, int32_t nsets, int32_t nways, int32_t r);
  virtual ~PhantomCache();

  const char* getCacheType() const override { return CACHE_TYPESTR; }
  size_t getEvictionSetSize() const override { return nways * _r; };
  size_t getGHMGroupSize() const override { return nways; };

  size_t getNRandomSets() const { return _r; }

  size_t getNumParams() const override;
  uint32_t getParam(size_t idx) const override;

  int32_t hasCollision(tag_t cl1, const CacheContext& ctx1, tag_t cl2,
                       const CacheContext& ctx2) const override;

protected:
  AssocCache* getSet(tag_t tag) const override;

private:
  void initKeys();
  void generateSalts(int32_t r);
  tag_t getIdx(tag_t cl, int32_t r) const;

private:
  int32_t _r; // number of randomly selected sets
  uint32_t* _key;
  uint64_t* _salts;
};

#endif /* PANTOMCACHE_H_ */
