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

#ifndef __DIPCACHE_H__
#define __DIPCACHE_H__ 1

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

class DIPCache : public Cache
{
public:
  const char* CACHE_TYPESTR;
  std::string cache_typestr;

protected:
  AssocCache** sets;
  int32_t nlines;
  int32_t nways;
  replAlg algorithm;

  std::unique_ptr<Cache> BIPCache;
  std::unique_ptr<Cache> LRUCache;

  uint32_t psel_bits;
  int32_t psel;

  //   virtual AssocCache* getSet(tag_t tag) const { return sets[tag % nsets];
  //   };

public:
  // DIPCache(int32_t nsets, int32_t nways)
  //     : DIPCache(REPL_LRU, nsets, nways){};
  DIPCache(const std::string& description, replAlg alg, int32_t nCacheLines,
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
      BIPCache = std::unique_ptr<Cache>(
          new SetAssocCache(REPL_BIP, nCacheLines / nWays, nWays));
      LRUCache = std::unique_ptr<Cache>(
          new SetAssocCache(REPL_LRU, nCacheLines / nWays, nWays));
    }
    else if (description == AssocCache::CACHE_TYPESTR)
    {
      BIPCache = std::unique_ptr<Cache>(new AssocCache(REPL_BIP, nCacheLines));
      LRUCache = std::unique_ptr<Cache>(new AssocCache(REPL_LRU, nCacheLines));
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
      BIPCache =
          std::unique_ptr<Cache>(new AssocPLcache(REPL_BIP, nCacheLines));
      LRUCache =
          std::unique_ptr<Cache>(new AssocPLcache(REPL_LRU, nCacheLines));
    }
    else if (description == PLcache::CACHE_TYPESTR)
    {
      BIPCache = std::unique_ptr<Cache>(
          new PLcache(REPL_BIP, nCacheLines / nWays, nWays));
      LRUCache = std::unique_ptr<Cache>(
          new PLcache(REPL_LRU, nCacheLines / nWays, nWays));
    }
    else if (description == WayPartitionCache::CACHE_TYPESTR)
    {
      BIPCache = std::unique_ptr<Cache>(
          new WayPartitionCache(REPL_BIP, nCacheLines / nWays, nWays, 1));
      LRUCache = std::unique_ptr<Cache>(
          new WayPartitionCache(REPL_LRU, nCacheLines / nWays, nWays, 1));
    }
    else if (description == CEASERCache::CACHE_TYPESTR)
    {
      BIPCache = std::unique_ptr<Cache>(
          new CEASERCache(REPL_BIP, nCacheLines / nWays, nWays));
      LRUCache = std::unique_ptr<Cache>(
          new CEASERCache(REPL_LRU, nCacheLines / nWays, nWays));
    }
    else if (description == CEASERSCache::CACHE_TYPESTR)
    {
      perror("Incompatible configuration: Ceasers and DIP Replacement!\n");
      exit(-1);
    }
    else if (description == PhantomCache::CACHE_TYPESTR)
    {
      BIPCache = std::unique_ptr<Cache>(
          new PhantomCache(REPL_BIP, nCacheLines, nWays, r));
      LRUCache = std::unique_ptr<Cache>(
          new PhantomCache(REPL_LRU, nCacheLines, nWays, r));
    }
  }
  virtual ~DIPCache() {}

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
    return BIPCache->hasCollision(cl1, ctx1, cl2, ctx2) ||
           LRUCache->hasCollision(cl1, ctx1, cl2, ctx2);
  };

protected:
  int32_t readCl(tag_t cl, const CacheContext& context,
                 std::list<CacheResponse>& response) override
  {
    // return getSet(cl)->read(cl, context, response);
    std::list<CacheResponse> LRUResponse;
    std::list<CacheResponse> BIPResponse;
    const auto LRUread = LRUCache->read(cl, context, LRUResponse);
    const auto BIPread = BIPCache->read(cl, context, BIPResponse);

    int32_t result;
    
    // printf("psel == %d, ", psel);
    if (psel & uint32_t(pow(2, psel_bits - 1)))
    {
      // printf("reading from BIP\n");
      result = BIPread;
      response = BIPResponse;
    }
    else
    {
      // printf("reading from LRU\n");
      result = LRUread;
      response = LRUResponse;
    }
    if (!BIPread)
    {
      // printf("BIP miss\n");
      psel = std::max(0, psel - 1);
    }
    if (!LRUread)
    {
      // printf("LRU miss\n");
      psel = std::min(int32_t(pow(2, psel_bits) - 1), psel + 1);
    }

    return result;
  };
  int32_t evictCl(tag_t cl, const CacheContext& context,
                  std::list<CacheResponse>& response) override
  {
    std::list<CacheResponse> LRUResponse;
    std::list<CacheResponse> BIPResponse;
    const auto LRUread = LRUCache->evict(cl, context, LRUResponse);
    const auto BIPread = BIPCache->evict(cl, context, BIPResponse);

    int32_t result;

    if (psel & uint32_t(pow(2, psel_bits - 1)))
    {
      result = BIPread;
      response = BIPResponse;
    }
    else
    {
      result = LRUread;
      response = LRUResponse;
    }
    if (!BIPread)
    {
      psel--;
    }
    if (!LRUread)
    {
      psel++;
    }

    return result;
  };
  int32_t writeCl(tag_t cl, const CacheContext& context,
                  std::list<CacheResponse>& response) override
  {
    std::list<CacheResponse> LRUResponse;
    std::list<CacheResponse> BIPResponse;
    const auto LRUread = LRUCache->write(cl, context, LRUResponse);
    const auto BIPread = BIPCache->write(cl, context, BIPResponse);

    int32_t result;

    if (psel & uint32_t(pow(2, psel_bits - 1)))
    {
      result = BIPread;
      response = BIPResponse;
    }
    else
    {
      result = LRUread;
      response = LRUResponse;
    }
    if (!BIPread)
    {
      psel--;
    }
    if (!LRUread)
    {
      psel++;
    }

    return result;
  };
  int32_t execCl(tag_t cl, const CacheContext& context,
                 std::list<CacheResponse>& response) override
  {
    std::list<CacheResponse> LRUResponse;
    std::list<CacheResponse> BIPResponse;
    const auto LRUread = LRUCache->exec(cl, context, LRUResponse);
    const auto BIPread = BIPCache->exec(cl, context, BIPResponse);

    int32_t result;

    if (psel & uint32_t(pow(2, psel_bits - 1)))
    {
      result = BIPread;
      response = BIPResponse;
    }
    else
    {
      result = LRUread;
      response = LRUResponse;
    }
    if (!BIPread)
    {
      psel--;
    }
    if (!LRUread)
    {
      psel++;
    }

    return result;
  };
};

#endif // __DIPCACHE_H__
