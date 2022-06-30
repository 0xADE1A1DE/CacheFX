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

#ifndef __DRRIPCACHE_H__
#define __DRRIPCACHE_H__ 1

#include <Cache/CEASERCache.h>
#include <Cache/CEASERSCache.h>
#include <Cache/CacheHierarchy.h>
#include <Cache/NewCache.h>
#include <Cache/PLcache.h>
#include <Cache/PhantomCache.h>
#include <Cache/ScatterCache.h>
#include <Cache/SetAssocCache.h>
#include <Cache/WayPartitionCache.h>

#include <cmath>
#include <memory>
#include <algorithm>

class DRRIPCache : public Cache
{
public:
  const char* CACHE_TYPESTR;
  std::string cache_typestr;

protected:
  AssocCache** sets;
  int32_t nlines;
  int32_t nways;
  replAlg algorithm;

  std::unique_ptr<Cache> SRRIPCache;
  std::unique_ptr<Cache> BRRIPCache;

  uint32_t psel_bits;
  int32_t psel;

  //   virtual AssocCache* getSet(tag_t tag) const { return sets[tag % nsets];
  //   };

public:
  // DIPCache(int32_t nsets, int32_t nways)
  //     : DIPCache(REPL_LRU, nsets, nways){};
  DRRIPCache(const std::string& description, replAlg alg, int32_t nCacheLines,
           int32_t nWays, int32_t r)
  {
    psel_bits = 4;
    psel = 0;

    nlines = nCacheLines;
    nways = nWays;
    
    cache_typestr = description;
    CACHE_TYPESTR = cache_typestr.c_str();

    if (description == SetAssocCache::CACHE_TYPESTR)
    {
      SRRIPCache = std::unique_ptr<Cache>(
          new SetAssocCache(REPL_SRRIP, nCacheLines / nWays, nWays));
      BRRIPCache = std::unique_ptr<Cache>(
          new SetAssocCache(REPL_BRRIP, nCacheLines / nWays, nWays));
    }
    else if (description == AssocCache::CACHE_TYPESTR)
    {
      SRRIPCache = std::unique_ptr<Cache>(new AssocCache(REPL_SRRIP, nCacheLines));
      BRRIPCache = std::unique_ptr<Cache>(new AssocCache(REPL_BRRIP, nCacheLines));
    }
    else if (description == ScatterCache::CACHE_TYPESTR)
    {
      perror("Incompatible configuration: ScatterCache and DIP Replacement!\n");
      exit(-1);
    }
    else if (description == NewCache::CACHE_TYPESTR)
    {
      perror("Incompatible configuration: NewCache and DIP Replacement!\n");
      exit(-1);
    }
    else if (description == AssocPLcache::CACHE_TYPESTR)
    {
      SRRIPCache =
          std::unique_ptr<Cache>(new AssocPLcache(REPL_SRRIP, nCacheLines));
      BRRIPCache =
          std::unique_ptr<Cache>(new AssocPLcache(REPL_BRRIP, nCacheLines));
    }
    else if (description == PLcache::CACHE_TYPESTR)
    {
      SRRIPCache = std::unique_ptr<Cache>(
          new PLcache(REPL_SRRIP, nCacheLines / nWays, nWays));
      BRRIPCache = std::unique_ptr<Cache>(
          new PLcache(REPL_BRRIP, nCacheLines / nWays, nWays));
    }
    else if (description == WayPartitionCache::CACHE_TYPESTR)
    {
      SRRIPCache = std::unique_ptr<Cache>(
          new WayPartitionCache(REPL_SRRIP, nCacheLines / nWays, nWays, 1));
      BRRIPCache = std::unique_ptr<Cache>(
          new WayPartitionCache(REPL_BRRIP, nCacheLines / nWays, nWays, 1));
    }
    else if (description == CEASERCache::CACHE_TYPESTR)
    {
      SRRIPCache = std::unique_ptr<Cache>(
          new CEASERCache(REPL_SRRIP, nCacheLines / nWays, nWays));
      BRRIPCache = std::unique_ptr<Cache>(
          new CEASERCache(REPL_BRRIP, nCacheLines / nWays, nWays));
    }
    else if (description == CEASERSCache::CACHE_TYPESTR)
    {
      perror("Incompatible configuration: Ceasers and DIP Replacement!\n");
      exit(-1);
    }
    else if (description == PhantomCache::CACHE_TYPESTR)
    {
      SRRIPCache = std::unique_ptr<Cache>(
          new PhantomCache(REPL_SRRIP, nCacheLines, nWays, r));
      BRRIPCache = std::unique_ptr<Cache>(
          new PhantomCache(REPL_BRRIP, nCacheLines, nWays, r));
    }
  }
  virtual ~DRRIPCache() {}

  size_t getNSets() const override { return nlines; };
  size_t getNWays() const override { return nways; };
  size_t getEvictionSetSize() const override { return nways; };
  size_t getGHMGroupSize() const override { return nways; };

  void setAlgorithm(replAlg alg){};
  replAlg getAlgorithm() const override { return algorithm; }

  const char* getCacheType() const override { return CACHE_TYPESTR; }

  size_t getNLines() const override { return nlines; }

  int32_t hasCollision(tag_t cl1, const CacheContext& ctx1, tag_t cl2,
                       const CacheContext& ctx2) const override
  {
    return SRRIPCache->hasCollision(cl1, ctx1, cl2, ctx2) ||
           BRRIPCache->hasCollision(cl1, ctx1, cl2, ctx2);
  };

protected:
  int32_t readCl(tag_t cl, const CacheContext& context,
                 std::list<CacheResponse>& response) override
  {
    // return getSet(cl)->read(cl, context, response);
    std::list<CacheResponse> SRRIPResponse;
    std::list<CacheResponse> BRRIPResponse;
    const auto SRRIPRead = SRRIPCache->read(cl, context, SRRIPResponse);
    const auto BRRIPRead = BRRIPCache->read(cl, context, BRRIPResponse);

    int32_t result;
    
    // printf("psel == %d, ", psel);
    if (psel & uint32_t(pow(2, psel_bits - 1)))
    {
      // printf("reading from BIP\n");
      result = BRRIPRead;
      response = BRRIPResponse;
    }
    else
    {
      // printf("reading from LRU\n");
      result = SRRIPRead;
      response = SRRIPResponse;
    }
    if (!BRRIPRead)
    {
      // printf("BIP miss\n");
      psel = std::max(0, psel - 1);
    }
    if (!SRRIPRead)
    {
      // printf("LRU miss\n");
      psel = std::min(int32_t(pow(2, psel_bits) - 1), psel + 1);
    }

    return result;
  };
  int32_t evictCl(tag_t cl, const CacheContext& context,
                  std::list<CacheResponse>& response) override
  {
    std::list<CacheResponse> SRRIPResponse;
    std::list<CacheResponse> BRRIPResponse;
    const auto SRRIPRead = SRRIPCache->evict(cl, context, SRRIPResponse);
    const auto BRRIPRead = BRRIPCache->evict(cl, context, BRRIPResponse);

    int32_t result;

    if (psel & uint32_t(pow(2, psel_bits - 1)))
    {
      result = BRRIPRead;
      response = BRRIPResponse;
    }
    else
    {
      result = SRRIPRead;
      response = SRRIPResponse;
    }
    if (!BRRIPRead)
    {
      psel--;
    }
    if (!SRRIPRead)
    {
      psel++;
    }

    return result;
  };
  int32_t writeCl(tag_t cl, const CacheContext& context,
                  std::list<CacheResponse>& response) override
  {
    std::list<CacheResponse> SRRIPResponse;
    std::list<CacheResponse> BRRIPResponse;
    const auto SRRIPRead = SRRIPCache->write(cl, context, SRRIPResponse);
    const auto BRRIPRead = BRRIPCache->write(cl, context, BRRIPResponse);

    int32_t result;

    if (psel & uint32_t(pow(2, psel_bits - 1)))
    {
      result = BRRIPRead;
      response = BRRIPResponse;
    }
    else
    {
      result = SRRIPRead;
      response = SRRIPResponse;
    }
    if (!BRRIPRead)
    {
      psel--;
    }
    if (!SRRIPRead)
    {
      psel++;
    }

    return result;
  };
  int32_t execCl(tag_t cl, const CacheContext& context,
                 std::list<CacheResponse>& response) override
  {
    std::list<CacheResponse> SRRIPResponse;
    std::list<CacheResponse> BRRIPResponse;
    const auto SRRIPRead = SRRIPCache->exec(cl, context, SRRIPResponse);
    const auto BRRIPRead = BRRIPCache->exec(cl, context, BRRIPResponse);

    int32_t result;

    if (psel & uint32_t(pow(2, psel_bits - 1)))
    {
      result = BRRIPRead;
      response = BRRIPResponse;
    }
    else
    {
      result = SRRIPRead;
      response = SRRIPResponse;
    }
    if (!BRRIPRead)
    {
      psel--;
    }
    if (!SRRIPRead)
    {
      psel++;
    }

    return result;
  };
};

#endif // __DRRIPCACHE_H__
