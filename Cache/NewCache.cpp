/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Aug 4, 2019
 *     Author: Fangfei
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

#include <Cache/NewCache.h>
#include <cstdlib>
#include <iostream>

const char* NewCache::CACHE_TYPESTR = "newcache";

NewCache::NewCache(size_t nbits, size_t kbits) : nbits(nbits), kbits(kbits)
{
  nLines = (1 << nbits);
  LDM_size = (1 << (nbits + kbits));
  entries.resize(nLines);
  for (size_t idx = 0; idx < nLines; idx++)
  {
    entries[idx].setTag(TAG_INIT);
    entries[idx].setContext(DEFAULT_CACHE_CONTEXT);
    entries[idx].setLnreg(TAG_INIT);
  }
}

NewCache::~NewCache() {}

int32_t NewCache::readCl(tag_t cl, const CacheContext& context,
                         std::list<CacheResponse>& response)
{

  tag_t lnreg = cl % LDM_size;
  tag_t tag = cl / LDM_size;

  NewCacheLine* blk = lnregMapLookup(context, lnreg);

  if (blk && (blk->getTag() == tag))
  {
    response.push_back(CacheResponse(true));
    return 1;
  }
  else if (blk)
  { // tag miss
    // directly replace blk with new tag
    if (blk->getTag() == TAG_NONE)
    {
      response.push_back(CacheResponse(false));
    }
    else
    {
      response.push_back(
          CacheResponse(false, blk->getTag() * LDM_size + lnreg));
    }
    blk->setTag(tag);
    return 0;
  }
  else
  {
    // index miss
    int32_t victim = random() % nLines;
    blk = &entries[victim];
    LnregKey key = std::make_pair(context.getCoreId(), lnreg);

    if (blk->getTag() == TAG_NONE)
    { // invalid line, directly insert
      blk->setTag(tag);
      blk->setContext(context);
      blk->setLnreg(lnreg);
      response.push_back(CacheResponse(false));
      // response.push_back(CacheResponse(false, blk->getTag() * LDM_size +
      // lnreg));
      lnregMap[key] = blk;
    }
    else
    { // erase old mapping and insert new mapping
      LnregKey old_key =
          std::make_pair(blk->getContext().getCoreId(), blk->getLnreg());
      lnregMap.erase(old_key);
      response.push_back(
          CacheResponse(false, blk->getTag() * LDM_size + blk->getLnreg()));
      // response.push_back(CacheResponse(false));
      blk->setTag(tag);
      blk->setContext(context);
      blk->setLnreg(lnreg);
      lnregMap[key] = blk;
    }

    return 0;
  }
}

int32_t NewCache::evictCl(tag_t cl, const CacheContext& context,
                          std::list<CacheResponse>& response)
{
  tag_t lnreg = cl % LDM_size;
  tag_t tag = cl / LDM_size;

  NewCacheLine* blk = lnregMapLookup(context, lnreg);

  if (blk && (blk->getTag() == tag))
  {
    response.push_back(CacheResponse(true, cl));
    blk->setTag(TAG_NONE);
    blk->setContext(-1);
    blk->setLnreg(TAG_NONE);
    lnregMap.erase(std::make_pair(context.getCoreId(), lnreg));
    return 1;
  }
  response.push_back(CacheResponse(false));
  return 0;
}

NewCacheLine* NewCache::lnregMapLookup(const CacheContext& context, tag_t lnreg)
{
  LnregKey key = std::make_pair(context.getCoreId(), lnreg);
  lnregMapIter iter = lnregMap.find(key);

  if (iter != lnregMap.end())
    return (*iter).second;

  return nullptr;
}

const char* NewCache::getCacheType() const { return CACHE_TYPESTR; }

size_t NewCache::getNLines() const { return nLines; }

size_t NewCache::getNSets() const { return 1; }

size_t NewCache::getNWays() const { return nLines; }

int32_t NewCache::hasCollision(tag_t cl1, const CacheContext& ctx1, tag_t cl2,
                               const CacheContext& ctx2) const
{
  return 1;
}

size_t NewCache::getNumParams() const { return 1; }

uint32_t NewCache::getParam(size_t idx) const
{
  if (idx == 0)
  {
    return kbits;
  }
  return 0;
}
