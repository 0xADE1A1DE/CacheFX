/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Feb 24, 2020
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

#ifndef CEASERCACHE_H_
#define CEASERCACHE_H_

#include <Cache/SetAssocCache.h>

class CEASERCache : public SetAssocCache
{
public:
  static const char* CACHE_TYPESTR;

  CEASERCache(int32_t nsets, int32_t nways)
      : CEASERCache(REPL_LRU, nsets, nways){};
  CEASERCache(replAlg alg, int32_t nsets, int32_t nways)
      : SetAssocCache(alg, nsets, nways)
  {
    initKeys();
  };
  virtual ~CEASERCache() { delete[] _key; };

  const char* getCacheType() const override { return CACHE_TYPESTR; }
  size_t getEvictionSetSize() const override { return nways; };
  size_t getGHMGroupSize() const override { return nways; };

protected:
  virtual AssocCache* getSet(tag_t tag) const;

private:
  void initKeys();
  tag_t getIdx(tag_t cl) const;

private:
  uint32_t* _key;
};

#endif /* CEASERCACHE_H_ */
