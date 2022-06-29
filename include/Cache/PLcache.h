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

/*
 * Implementation of simplified version of PLcache where every access is a
 * locked access otherwise requires change to either MMU or cache interface to
 * indicate whether an access is locked access or not
 */
#ifndef __PLCACHE_H__
#define __PLCACHE_H__ 1

#include "AssocCache.h"
#include "SetAssocCache.h"
#include <iostream>

class AssocPLcache : public AssocCache
{
public:
  static const char* CACHE_TYPESTR;

private:
  CacheContext getContext(int32_t way) const { return entries[way].context; };
  void setContext(int32_t way, CacheContext context)
  {
    entries[way].context = std::move(context);
  };

public:
  AssocPLcache(int32_t size) : AssocPLcache(REPL_LRU, size){};
  AssocPLcache(replAlg alg, int32_t size) : AssocCache(alg, size){};
  virtual ~AssocPLcache(){};

  const char* getCacheType() const override { return CACHE_TYPESTR; }
  size_t getEvictionSetSize() const override { return size; };
  size_t getGHMGroupSize() const override { return 1; };

  int32_t hasCollision(tag_t cl1, const CacheContext& ctx1, tag_t cl2,
                       const CacheContext& ctx2) const override;

protected:
  int32_t readCl(tag_t cl, const CacheContext& context,
                 std::list<CacheResponse>& response) override;
  int32_t evictCl(tag_t cl, const CacheContext& context,
                  std::list<CacheResponse>& response) override;
};

class PLcache : public Cache
{
public:
  static const char* CACHE_TYPESTR;

protected:
  AssocPLcache** sets;
  int32_t nsets;
  int32_t nways;
  replAlg algorithm;

  AssocPLcache* getSet(tag_t tag) const { return sets[tag % nsets]; };

public:
  PLcache(int32_t nsets, int32_t nways) : PLcache(REPL_LRU, nsets, nways){};
  PLcache(replAlg alg, int32_t nsets, int32_t nways);
  virtual ~PLcache();

  size_t getNSets() const override { return (size_t)nsets; };
  size_t getNWays() const override { return (size_t)nways; };
  size_t getEvictionSetSize() const override { return nways; };
  size_t getGHMGroupSize() const override { return nways; };

  void setAlgorithm(replAlg alg);
  replAlg getAlgorithm() const { return algorithm; }

  const char* getCacheType() const override { return CACHE_TYPESTR; }

  size_t getNLines() const { return nsets * nways; }

  int32_t hasCollision(tag_t cl1, const CacheContext& ctx1, tag_t cl2,
                       const CacheContext& ctx2) const override;

protected:
  int32_t readCl(tag_t cl, const CacheContext& context,
                 std::list<CacheResponse>& response) override
  {
    return getSet(cl)->read(cl, context, response);
  };
  int32_t evictCl(tag_t cl, const CacheContext& context,
                  std::list<CacheResponse>& response) override
  {
    return getSet(cl)->evict(cl, context, response);
  };
  int32_t writeCl(tag_t cl, const CacheContext& context,
                  std::list<CacheResponse>& response) override
  {
    return getSet(cl)->write(cl, context, response);
  };
  int32_t execCl(tag_t cl, const CacheContext& context,
                 std::list<CacheResponse>& response) override
  {
    return getSet(cl)->exec(cl, context, response);
  };
};

#endif // __PLCACHE_H__
