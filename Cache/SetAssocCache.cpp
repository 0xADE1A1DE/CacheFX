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
 
#include <Cache/SetAssocCache.h>
#include <types.h>

const char* SetAssocCache::CACHE_TYPESTR = "set-associative";

SetAssocCache::SetAssocCache(replAlg alg, int32_t nsets, int32_t nways)
    : nsets(nsets), nways(nways)
{
  sets = new AssocCache*[nsets];

  for (int32_t i = 0; i < nsets; i++)
    sets[i] = new AssocCache(alg, nways);

  if (sets > 0)
  {
    algorithm = sets[0]->getAlgorithm();
  }
  else
  {
    algorithm = alg;
  }
}

SetAssocCache::~SetAssocCache()
{
  for (int32_t i = 0; i < nsets; i++)
    delete sets[i];

  delete[] sets;
}

void SetAssocCache::setAlgorithm(replAlg alg)
{
  for (int32_t i = 0; i < nsets; i++)
    sets[i]->setAlgorithm(alg);

  if (sets > 0)
  {
    algorithm = sets[0]->getAlgorithm();
  }
  else
  {
    algorithm = alg;
  }
}

int32_t SetAssocCache::hasCollision(tag_t cl1, const CacheContext& ctx1,
                                    tag_t cl2, const CacheContext& ctx2) const
{
  const AssocCache* assoc1 = this->getSet(cl1);
  const AssocCache* assoc2 = this->getSet(cl2);
  if (assoc1 == assoc2)
  {
    return assoc1->hasCollision(cl1, ctx1, cl2, ctx2);
  }
  return 0;
}
