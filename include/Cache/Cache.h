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

#ifndef __CACHE_H__
#define __CACHE_H__

#include <CacheContext.h>
#include <cstddef>
#include <iostream>
#include <list>
#include <map>
#include <statistics.h>
#include <types.h>

enum replAlg
{
  REPL_LRU,
  REPL_RANDOM,
  REPL_BIT_PLRU,
  REPL_TREE_PLRU,
  REPL_SRRIP,
  REPL_LIP,
  REPL_BIP,
  REPL_DIP,
  REPL_BRRIP,
  REPL_DRRIP
};

static const uint8_t CACHE_LEVEL_UNKNOWN = 0;

class CacheResponse
{
public:
  uint8_t _level;
  bool _hit;
  bool _eviction;
  tag_t _evictedTag;

  CacheResponse(bool hit, tag_t evictedTag)
      : _level(CACHE_LEVEL_UNKNOWN), _hit(hit), _eviction(true),
        _evictedTag(evictedTag){};
  CacheResponse(bool hit)
      : _level(CACHE_LEVEL_UNKNOWN), _hit(hit), _eviction(false),
        _evictedTag(0){};
  CacheResponse(uint8_t level, bool hit, tag_t evictedTag)
      : _level(level), _hit(hit), _eviction(true), _evictedTag(evictedTag){};
  CacheResponse(uint8_t level, bool hit)
      : _level(level), _hit(hit), _eviction(false), _evictedTag(0){};
};

static const CacheContext DEFAULT_CACHE_CONTEXT = CacheContext(0);
static const CacheContext DEFAULT_CACHE_CONTEXT_VICTIM = CacheContext(1);

class Cache
{
public:
  virtual ~Cache(){};
  int32_t read(tag_t cl) { return read(cl, DEFAULT_CACHE_CONTEXT); };
  int32_t write(tag_t cl) { return write(cl, DEFAULT_CACHE_CONTEXT); };
  int32_t exec(tag_t cl) { return exec(cl, DEFAULT_CACHE_CONTEXT); };
  int32_t evict(tag_t cl) { return evict(cl, DEFAULT_CACHE_CONTEXT); };
  int32_t hasCollision(tag_t cl1, tag_t cl2) const
  {
    return hasCollision(cl1, DEFAULT_CACHE_CONTEXT, cl2, DEFAULT_CACHE_CONTEXT);
  };
  int32_t read(tag_t cl, const CacheContext& context)
  {
    std::list<CacheResponse> resp = std::list<CacheResponse>();
    return read(cl, context, resp);
  };
  int32_t write(tag_t cl, const CacheContext& context)
  {
    std::list<CacheResponse> resp = std::list<CacheResponse>();
    return write(cl, context, resp);
  };
  int32_t exec(tag_t cl, const CacheContext& context)
  {
    std::list<CacheResponse> resp = std::list<CacheResponse>();
    return exec(cl, context, resp);
  };
  int32_t evict(tag_t cl, const CacheContext& context)
  {
    std::list<CacheResponse> resp = std::list<CacheResponse>();
    return evict(cl, context, resp);
  };
  int32_t read(tag_t cl, const CacheContext& context,
               std::list<CacheResponse>& response)
  {
    int32_t miss = readCl(cl, context, response);
    auto cacheResp = response.back();
    CacheStatistics<uint64_t>& stats = _cacheStatistics[context];
    if (cacheResp._hit)
    {
      stats.rdHits++;
    }
    else
    {
      stats.rdMisses++;
      if (cacheResp._eviction)
      {
        stats.rdEvictions++;
      }
    }
    return miss;
  };
  int32_t write(tag_t cl, const CacheContext& context,
                std::list<CacheResponse>& response)
  {
    int32_t miss = writeCl(cl, context, response);
    auto cacheResp = response.back();
    CacheStatistics<uint64_t>& stats = _cacheStatistics[context];
    if (cacheResp._hit)
    {
      stats.wrHits++;
    }
    else
    {
      stats.wrMisses++;
      if (cacheResp._eviction)
      {
        stats.wrEvictions++;
      }
    }
    return miss;
  };
  int32_t exec(tag_t cl, const CacheContext& context,
               std::list<CacheResponse>& response)
  {
    int32_t miss = execCl(cl, context, response);
    auto cacheResp = response.back();
    CacheStatistics<uint64_t>& stats = _cacheStatistics[context];
    if (cacheResp._hit)
    {
      stats.execHits++;
    }
    else
    {
      stats.execMisses++;
      if (cacheResp._eviction)
      {
        stats.execEvictions++;
      }
    }
    return miss;
  };
  int32_t evict(tag_t cl, const CacheContext& context,
                std::list<CacheResponse>& response)
  {
    int32_t miss = evictCl(cl, context, response);
    auto cacheResp = response.back();
    CacheStatistics<uint64_t>& stats = _cacheStatistics[context];
    if (cacheResp._hit)
    {
      stats.invHits++;
    }
    else
    {
      stats.invMisses++;
    }
    return miss;
  };
  virtual const char* getCacheType() const = 0;
  virtual size_t getNLines() const = 0;
  virtual size_t getNSets() const = 0;
  virtual size_t getNWays() const = 0;
  virtual size_t getEvictionSetSize() const = 0;
  virtual size_t getGHMGroupSize() const = 0;
  virtual replAlg getAlgorithm() const { return REPL_RANDOM; }
  virtual size_t getNumParams() const { return 0; }
  virtual uint32_t getParam(size_t idx) const { return 0; }
  virtual int32_t hasCollision(tag_t cl1, const CacheContext& ctx1, tag_t cl2,
                               const CacheContext& ctx2) const = 0;
  std::map<CacheContext, CacheStatistics<uint64_t>,
           CacheContextCoreIdComparator>
  getCacheStatistics() const
  {
    return _cacheStatistics;
  };
  CacheStatistics<uint64_t>
  getCacheStatistics(const CacheContext& context) const
  {
    auto stats = _cacheStatistics.find(context);
    if (stats == _cacheStatistics.end())
    {
      return CacheStatistics<uint64_t>();
    }
    return stats->second;
  };

protected:
  virtual int32_t readCl(tag_t cl, const CacheContext& context,
                         std::list<CacheResponse>& response) = 0;
  virtual int32_t writeCl(tag_t cl, const CacheContext& context,
                          std::list<CacheResponse>& response)
  {
    return readCl(cl, context, response);
  };
  virtual int32_t execCl(tag_t cl, const CacheContext& context,
                         std::list<CacheResponse>& response)
  {
    return readCl(cl, context, response);
  };
  virtual int32_t evictCl(tag_t cl, const CacheContext& context,
                          std::list<CacheResponse>& response) = 0;

private:
  std::map<CacheContext, CacheStatistics<uint64_t>,
           CacheContextCoreIdComparator>
      _cacheStatistics;
};

#endif
