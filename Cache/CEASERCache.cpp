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

#include <Cache/CEASERCache.h>
#include <crypto/speck.h>
#include <iostream>

const char* CEASERCache::CACHE_TYPESTR = "ceaser";

void CEASERCache::initKeys()
{
  // ASAN overflow: speck64ExpandKey() accesses 4th
  // element of type uint32_t*
  uint32_t K[] = {0xDEADBEEF, 0x000CAFFE, 0x47111174, 0x08155180};

  _key = new uint32_t[27];

  speck64ExpandKey(K, _key);
}

AssocCache* CEASERCache::getSet(tag_t cl) const { return sets[getIdx(cl)]; }

tag_t CEASERCache::getIdx(tag_t cl) const
{
  uint64_t v;
  uint32_t* vPtr = (uint32_t*)&v;

  v = cl & 0xFFFFFFFFFFFFFFFF;

  speck64Encrypt(vPtr + 0, vPtr + 1, _key);

  return v % nsets;
}
