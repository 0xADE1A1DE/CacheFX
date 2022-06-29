/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Feb 25, 2020
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

#include <Cache/CEASERSCache.h>

#include <crypto/speck.h>
#include <iostream>

const char* CEASERSCache::CACHE_TYPESTR = "ceaser-s";

CEASERSCache::CEASERSCache(size_t sets, size_t ways, size_t partitions)
    : _nSets(sets), _nWays(ways), _nPartitions(partitions), _invalidFirst(false)
{
  if ((_nWays % _nPartitions) != 0)
  {
    _nPartitions = 1;
  }

  _cacheEntries.resize(_nWays);
  for (size_t w = 0; w < _nWays; w++)
  {
    _cacheEntries[w].resize(_nSets);
    for (size_t s = 0; s < _nSets; s++)
    {
      _cacheEntries[w][s].tag = TAG_INIT;
      _cacheEntries[w][s].accessTime = 0;
      _cacheEntries[w][s].flags = 0;
    }
  }

  initKeys();
}

void CEASERSCache::initKeys()
{
  // ASAN overflow: speck64ExpandKey() accesses 4th
  // element of type uint32_t*
  uint32_t K[] = {0x06FADE60, 0xCAB4BEEF, 0x04866840, 0x80866808};

  _key = new uint32_t[27];

  speck64ExpandKey(K, _key);
}

CEASERSCache::~CEASERSCache() { delete[] _key; }

int32_t CEASERSCache::readCl(tag_t cl, const CacheContext& context,
                             std::list<CacheResponse>& response)
{
  std::vector<cacheEntry*> vSet = getVirtualSet(cl);

  int32_t replaceWay = -1;
  for (size_t w = 0; w < _nWays; w++)
  {
    if (vSet[w]->tag == cl)
    {
      access(*vSet[w]);
      response.push_back(CacheResponse(true));
      return 1;
    }
    if (vSet[w]->tag == TAG_NONE)
      replaceWay = w;
  }

  if (replaceWay != -1 && _invalidFirst)
  {
    vSet[replaceWay]->tag = cl;
    access(*vSet[replaceWay]);
    response.push_back(CacheResponse(false));
    return 0;
  }

  replaceWay = random() % _nWays;
  if (vSet[replaceWay]->tag == TAG_NONE)
  {
    response.push_back(CacheResponse(false));
  }
  else
  {
    response.push_back(CacheResponse(false, vSet[replaceWay]->tag));
  }
  vSet[replaceWay]->tag = cl;
  access(*vSet[replaceWay]);
  return 0;
}

int32_t CEASERSCache::evictCl(tag_t cl, const CacheContext& context,
                              std::list<CacheResponse>& response)
{
  std::vector<cacheEntry*> vSet = getVirtualSet(cl);

  for (size_t w = 0; w < _nWays; w++)
  {
    if (vSet[w]->tag == cl)
    {
      vSet[w]->tag = TAG_NONE;
      response.push_back(CacheResponse(true, cl));
      return 1;
    }
  }

  response.push_back(CacheResponse(false));
  return 0;
}

const char* CEASERSCache::getCacheType() const { return CACHE_TYPESTR; }

size_t CEASERSCache::getNLines() const { return _nSets * _nWays; }

size_t CEASERSCache::getNWays() const { return _nWays; }

size_t CEASERSCache::getNSets() const { return _nSets; }

tag_t CEASERSCache::getIdx(tag_t cl, size_t partition) const
{
  uint64_t v, tweak;
  uint32_t* vPtr = (uint32_t*)&v;

  v = cl & 0xFFFFFFFFFFFFFFFF;
  tweak = (partition & 0xFF) * 0x0101010101010101;

  // Tweak via XEX construction

  v ^= tweak;
  speck64Encrypt(vPtr + 0, vPtr + 1, _key);
  v ^= tweak;

  return v % _nSets;
}

std::vector<cacheEntry*> CEASERSCache::getVirtualSet(tag_t cl)
{
  std::vector<cacheEntry*> vSet(_nWays);
  const size_t partitionSize = (_nWays / _nPartitions);
  for (size_t p = 0; p < _nPartitions; p++)
  {
    const tag_t idx = getIdx(cl, p);
    for (size_t w = 0; w < partitionSize; w++)
    {
      vSet[p * partitionSize + w] =
          &(_cacheEntries[p * partitionSize + w][idx]);
    }
  }
  return vSet;
}

std::vector<tag_t> CEASERSCache::getWayIndices(tag_t cl) const
{
  std::vector<tag_t> wayIndices(_nWays);
  const size_t partitionSize = (_nWays / _nPartitions);
  for (size_t p = 0; p < _nPartitions; p++)
  {
    const tag_t idx = getIdx(cl, p);
    for (size_t w = 0; w < partitionSize; w++)
      wayIndices[p * partitionSize + w] = idx;
  }
  return wayIndices;
}

int32_t CEASERSCache::hasCollision(tag_t cl1, const CacheContext& ctx1,
                                   tag_t cl2, const CacheContext& ctx2) const
{
  std::vector<tag_t> wayIndices1 = getWayIndices(cl1);
  std::vector<tag_t> wayIndices2 = getWayIndices(cl2);

  for (size_t w = 0; w < _nWays; w++)
  {
    if (wayIndices1[w] == wayIndices2[w])
    {
      return 1;
    }
  }
  return 0;
}

size_t CEASERSCache::getNumParams() const { return 2; }

uint32_t CEASERSCache::getParam(size_t idx) const
{
  if (idx == 0)
  {
    return _nPartitions;
  }
  else if (idx == 1)
  {
    return _invalidFirst;
  }
  return 0;
}
