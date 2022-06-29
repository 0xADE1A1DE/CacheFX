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

#include <Cache/PhantomCache.h>
#include <algorithm>
#include <crypto/speck.h>
#include <types.h>
#include <vector>

const char* PhantomCache::CACHE_TYPESTR = "phantom";

PhantomCache::PhantomCache(replAlg alg, int32_t nsets, int32_t nways, int32_t r)
    : SetAssocCache(alg, nsets, nways)
{
  initKeys();
  generateSalts(r);
}

PhantomCache::~PhantomCache()
{
  delete[] _key;
  delete[] _salts;
}

void PhantomCache::initKeys()
{
  // ASAN overflow: speck64ExpandKey() accesses 4th
  // element of type uint32_t*
  uint32_t K[] = {0xDEADBEEF, 0x000CAFFE, 0x10FADE01, 0xFE0123ED};

  _key = new uint32_t[27];

  speck64ExpandKey(K, _key);
}

void PhantomCache::generateSalts(int32_t r)
{
  _r = r;
  _salts = new uint64_t[r];

  for (int32_t i = 0; i < r; i++)
    _salts[i] = random() & 0xFFFFFFFFFFFFFFFF;
}

AssocCache* PhantomCache::getSet(tag_t cl) const
{
  // if match (hit): return the pointer to the set
  // otherwise: return a randomly selected set
  int32_t i, j;
  tag_t setIdx;

  for (i = 0; i < _r; i++)
  {
    setIdx = getIdx(cl, i);
    for (j = 0; j < nways; j++)
    {
      if (cl == sets[setIdx]->getTag(j))
        return sets[setIdx]; // find a match
    }
  }
  // no match, randomly select a set within the _r cache sets
  i = random() % _r;
  setIdx = getIdx(cl, i);
  return sets[setIdx];
}

tag_t PhantomCache::getIdx(tag_t cl, int32_t r) const
{
  uint64_t v, salt;
  uint32_t* vPtr = (uint32_t*)&v;

  v = cl & 0xFFFFFFFFFFFFFFFF;
  salt = _salts[r];
  // XEX construction of tweak
  v ^= salt;
  speck64Encrypt(vPtr + 0, vPtr + 1, _key);
  v ^= salt;

  return v % nsets;
}

size_t PhantomCache::getNumParams() const { return 1; }

uint32_t PhantomCache::getParam(size_t idx) const
{
  if (idx == 0)
  {
    return _r;
  }
  return 0;
}

int32_t PhantomCache::hasCollision(tag_t cl1, const CacheContext& ctx1,
                                   tag_t cl2, const CacheContext& ctx2) const
{
  std::vector<tag_t> setIndices1;

  for (int32_t i = 0; i < _r; i++)
  {
    tag_t setIdx = getIdx(cl1, i);
    setIndices1.push_back(setIdx);
  }

  for (int32_t i = 0; i < _r; i++)
  {
    tag_t setIdx = getIdx(cl2, i);
    auto it = std::find(setIndices1.begin(), setIndices1.end(), setIdx);
    if (it != setIndices1.end())
    {
      return 1;
    }
  }

  return 0;
}
