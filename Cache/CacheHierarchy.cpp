/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Jul 23, 2019
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

#include <Cache/CacheHierarchy.h>
#include <Cache/SetAssocCache.h>
#include <algorithm>
#include <iostream>

const char* CacheHierarchy::CACHE_TYPESTR = "cache-hierarchy";

CacheHierarchy::CacheHierarchy() : _cacheHierarchy(0) {}

CacheHierarchy::~CacheHierarchy() {}

int32_t CacheHierarchy::readCl(tag_t cl, const CacheContext& context,
                               std::list<CacheResponse>& response)
{
  size_t i = 0;
  int32_t cacheHit = 0;
  do
  {
    std::list<CacheResponse> newResponses;
    cacheHit = _cacheHierarchy[i]->read(cl, context, newResponses);
    auto it = newResponses.begin();
    while (it != newResponses.end())
    {
      it->_level = i + 1;
      it++;
    }
    response.splice(response.end(), newResponses);
    i++;
  } while (i < _cacheHierarchy.size() && cacheHit == 0);

  return (cacheHit != 0) ? i : 0;
}

int32_t CacheHierarchy::writeCl(tag_t cl, const CacheContext& context,
                                std::list<CacheResponse>& response)
{
  size_t i = 0;
  int32_t cacheHit = 0;
  do
  {
    std::list<CacheResponse> newResponses;
    cacheHit = _cacheHierarchy[i]->write(cl, context, newResponses);
    auto it = newResponses.begin();
    while (it != newResponses.end())
    {
      it->_level = i + 1;
      it++;
    }
    response.splice(response.end(), newResponses);
    i++;
  } while (i < _cacheHierarchy.size() && cacheHit == 0);

  return (cacheHit != 0) ? i : 0;
}

int32_t CacheHierarchy::execCl(tag_t cl, const CacheContext& context,
                               std::list<CacheResponse>& response)
{
  size_t i = 0;
  int32_t cacheHit = 0;
  do
  {
    std::list<CacheResponse> newResponses;
    cacheHit = _cacheHierarchy[i]->exec(cl, context, newResponses);
    auto it = newResponses.begin();
    while (it != newResponses.end())
    {
      it->_level = i + 1;
      it++;
    }
    response.splice(response.end(), newResponses);
    i++;
  } while (i < _cacheHierarchy.size() && cacheHit == 0);

  return (cacheHit != 0) ? i : 0;
}

int32_t CacheHierarchy::evictCl(tag_t cl, const CacheContext& context,
                                std::list<CacheResponse>& response)
{
  int32_t delay = 0;
  for (size_t i = 0; i < _cacheHierarchy.size(); i++)
  {
    std::list<CacheResponse> newResponses;
    delay += _cacheHierarchy[i]->evict(cl, context, newResponses);
    auto it = newResponses.begin();
    while (it != newResponses.end())
    {
      it->_level = i;
      it++;
    }
    response.splice(response.end(), newResponses);
  }

  return delay;
}

void CacheHierarchy::setCacheLevels(std::vector<std::unique_ptr<Cache>> caches)
{
  _cacheHierarchy = std::move(caches);
}

void CacheHierarchy::addCacheLevel(std::unique_ptr<Cache> cache)
{
  _cacheHierarchy.push_back(std::move(cache));
}

void CacheHierarchy::moveCacheUp(uint32_t level)
{
  if (level > 0 && level < _cacheHierarchy.size())
  {
    auto iter = _cacheHierarchy.begin() + (level - 1);
    std::iter_swap(iter, iter + 1);
  }
}

void CacheHierarchy::moveCacheDown(uint32_t level)
{
  if (level < (_cacheHierarchy.size() - 1))
  {
    auto iter = _cacheHierarchy.begin() + level;
    std::iter_swap(iter, iter + 1);
  }
}

void CacheHierarchy::removeCacheLevel(uint32_t level)
{
  if (level < _cacheHierarchy.size())
  {
    auto iter = _cacheHierarchy.begin() + level;
    _cacheHierarchy.erase(iter);
  }
}

void CacheHierarchy::replaceCacheLevel(std::unique_ptr<Cache> cache,
                                       uint32_t level)
{
  if (level < _cacheHierarchy.size())
  {
    _cacheHierarchy[level].reset();
    _cacheHierarchy[level] = std::move(cache);
  }
}

Cache* CacheHierarchy::getCache(uint32_t level)
{
  return _cacheHierarchy[level].get();
}

void CacheHierarchy::printHierarchy() const
{
  for (int32_t i = 0; i < (int32_t)_cacheHierarchy.size(); i++)
  {
    std::cout << "Level: " << (i + 1)
              << " Type: " << _cacheHierarchy[i]->getCacheType()
              << " NLines: " << _cacheHierarchy[i]->getNLines() << std::endl;
  }
}

size_t CacheHierarchy::getNLines() const
{
  size_t nlines = 0;

  for (int32_t i = 0; i < (int32_t)_cacheHierarchy.size(); i++)
  {
    nlines += _cacheHierarchy[i]->getNLines();
  }

  return nlines;
}

size_t CacheHierarchy::getNSets() const
{
  size_t nsets = 0;
  for (int32_t i = 0; i < (int32_t)_cacheHierarchy.size(); i++)
  {
    if (nsets < _cacheHierarchy[i]->getNSets())
    {
      nsets = _cacheHierarchy[i]->getNSets();
    }
  }
  return nsets;
}
size_t CacheHierarchy::getNWays() const { return getNLines() / getNSets(); }

int32_t CacheHierarchy::hasCollision(tag_t cl1, const CacheContext& ctx1,
                                     tag_t cl2, const CacheContext& ctx2) const
{
  uint32_t level = 0;
  while (level < _cacheHierarchy.size())
  {
    if (_cacheHierarchy[level]->hasCollision(cl1, ctx1, cl2, ctx2))
    {
      return level + 1;
    }
    level++;
  }
  return 0;
}

replAlg CacheHierarchy::getAlgorithm() const
{
  if (_cacheHierarchy.size() == 1)
  {
    return _cacheHierarchy[0]->getAlgorithm();
  }
  return REPL_RANDOM;
}

size_t CacheHierarchy::getEvictionSetSize() const
{
  if (_cacheHierarchy.size() == 1)
  {
    return _cacheHierarchy[0]->getEvictionSetSize();
  }
  return 0;
}

size_t CacheHierarchy::getGHMGroupSize() const
{
  if (_cacheHierarchy.size() == 1)
  {
    return _cacheHierarchy[0]->getGHMGroupSize();
  }
  return 0;
}

size_t CacheHierarchy::getNumParams() const
{
  if (_cacheHierarchy.size() == 1)
  {
    return _cacheHierarchy[0]->getNumParams();
  }
  return 0;
}

uint32_t CacheHierarchy::getParam(size_t idx) const
{
  if (_cacheHierarchy.size() == 1)
  {
    return _cacheHierarchy[0]->getParam(idx);
  }
  return 0;
}
