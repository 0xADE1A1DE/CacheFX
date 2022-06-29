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

#ifndef __NEWCACHE_H__
#define __NEWCACHE_H__ 1

#include <Cache/Cache.h>
#include <CacheLine/CacheLine.h>
#include <map>
#include <types.h>
#include <vector>

class NewCacheLine : public CacheLine
{
private:
  CacheContext context;
  tag_t lnreg;

public:
  CacheContext& getContext() { return context; }
  void setContext(const CacheContext& context)
  {
    this->context = std::move(context);
  }
  tag_t getLnreg() { return lnreg; }
  void setLnreg(tag_t lnreg) { this->lnreg = lnreg; }
};

class NewCache : public Cache
{
public:
  static const char* CACHE_TYPESTR;

private:
  std::vector<NewCacheLine> entries;
  typedef std::pair<int32_t, tag_t> LnregKey;
  typedef std::map<LnregKey, NewCacheLine*> map_t;
  typedef typename map_t::const_iterator lnregMapIter;
  map_t lnregMap;

  size_t nbits;  // number of bits to index physical cache
  size_t kbits;  // number of extra bits to index LDM
  size_t nLines; // number of cache lines in physical cache
  size_t LDM_size;

public:
  NewCache(size_t nbits, size_t kbits);
  virtual ~NewCache();

  int32_t hasCollision(tag_t cl1, const CacheContext& ctx1, tag_t cl2,
                       const CacheContext& ctx2) const override;
  const char* getCacheType() const override;
  size_t getNLines() const override;
  size_t getNSets() const override;
  size_t getNWays() const override;
  size_t getEvictionSetSize() const override { return nLines; };
  size_t getGHMGroupSize() const override { return 1; };

  size_t getNumParams() const override;
  uint32_t getParam(size_t idx) const override;

  size_t getNBits() const { return nbits; }
  size_t getKBits() const { return kbits; }

protected:
  int32_t readCl(tag_t cl, const CacheContext& context,
                 std::list<CacheResponse>& response) override;
  int32_t evictCl(tag_t cl, const CacheContext& context,
                  std::list<CacheResponse>& response) override;

private:
  NewCacheLine* lnregMapLookup(const CacheContext& context, tag_t cl);
};

#endif // __NEWCACHE_H__
