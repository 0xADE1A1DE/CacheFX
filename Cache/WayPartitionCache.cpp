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

#include <Cache/WayPartitionCache.h>

const char* WayPartitionCache::CACHE_TYPESTR = "way-partition";

WayPartitionCache::WayPartitionCache(replAlg alg, int32_t nsets, int32_t nways,
                                     int32_t nsecways)
    : _cacheContext0(DEFAULT_CONTEXT), _cacheContext1(DEFAULT_CONTEXT)
{
  _cacheDomain0 = new SetAssocCache(alg, nsets, nways - nsecways);
  _cacheDomain1 = new SetAssocCache(alg, nsets, nsecways);
  _cacheContext1.setCoreId(1);
}

WayPartitionCache::~WayPartitionCache()
{
  delete _cacheDomain0;
  delete _cacheDomain1;
}

int32_t WayPartitionCache::readCl(tag_t cl, const CacheContext& context,
                                  std::list<CacheResponse>& response)
{
  if (context == _cacheContext0)
  {
    return _cacheDomain0->read(cl, context, response);
  }
  else if (context == _cacheContext1)
  {
    return _cacheDomain1->read(cl, context, response);
  }
  return 0;
}

int32_t WayPartitionCache::evictCl(tag_t cl, const CacheContext& context,
                                   std::list<CacheResponse>& response)
{
  if (context == _cacheContext0)
  {
    return _cacheDomain0->evict(cl, context, response);
  }
  else if (context == _cacheContext1)
  {
    return _cacheDomain1->evict(cl, context, response);
  }
  return 0;
}

int32_t WayPartitionCache::writeCl(tag_t cl, const CacheContext& context,
                                   std::list<CacheResponse>& response)
{
  if (context == _cacheContext0)
  {
    return _cacheDomain0->write(cl, context, response);
  }
  else if (context == _cacheContext1)
  {
    return _cacheDomain1->write(cl, context, response);
  }
  return 0;
}

int32_t WayPartitionCache::execCl(tag_t cl, const CacheContext& context,
                                  std::list<CacheResponse>& response)
{
  if (context == _cacheContext0)
  {
    return _cacheDomain0->exec(cl, context, response);
  }
  else if (context == _cacheContext1)
  {
    return _cacheDomain1->exec(cl, context, response);
  }
  return 0;
}

int32_t WayPartitionCache::hasCollision(tag_t cl1, const CacheContext& ctx1,
                                        tag_t cl2,
                                        const CacheContext& ctx2) const
{
  return 0;
}
