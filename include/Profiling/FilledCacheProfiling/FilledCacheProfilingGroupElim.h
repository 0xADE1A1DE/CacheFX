/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Feb 28, 2020
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

#ifndef FILLEDCACHEPROFILINGGROUPELIM_H_
#define FILLEDCACHEPROFILINGGROUPELIM_H_

#include "FilledCacheProfiling.h"
#include <Cache/Cache.h>

class FilledCacheProfilingGroupElim : public FilledCacheProfiling
{
public:
  FilledCacheProfilingGroupElim(MMU* mmu, Cache* cache);
  FilledCacheProfilingGroupElim(MMU* mmu, Cache* cache, bool plru);
  virtual ~FilledCacheProfilingGroupElim();

  void createEvictionSet(Victim* victim, uint32_t numAddresses,
                         uint32_t maxIterations) override;

protected:
  size_t selectCandidates(std::vector<char>& candidateSet);
  size_t selectCandidatesPLRU(std::vector<char>& candidateSet);
  bool hasConflict(Victim* victim, std::vector<char>& candidateSet,
                   address_t startAddress);
  size_t pruneCandidateSet(std::vector<char>& candidateSet,
                           std::vector<int32_t>& candidateSetGroupIndx,
                           size_t pruneGroupIndx);
  void unpruneCandidateSet(std::vector<char>& candidateSet,
                           std::vector<int32_t>& candidateSetGroupIndx,
                           size_t pruneIndx);
  void cleanCandidateGroup(std::vector<char>& candidateSet,
                           std::vector<int32_t>& candidateSetGroupIndx,
                           size_t pruneIndx);
  void resetCandidateGroup(std::vector<int32_t>& candidateSetGroupIndx);
  void assignToGroups(std::vector<char>& candidateSet,
                      size_t candidateSetSelected,
                      std::vector<int32_t>& candidateSetGroupIndx,
                      uint32_t numOfGroups);
  double determineEvictionProbability(Victim* victim,
                                      std::vector<char>& candidateSet,
                                      address_t startAddress,
                                      int32_t totalTests);

  //  void probeCache(std::vector<char> candidateSet, std::vector<int32_t>
  //  &candidateCount, address_t startAddress); void
  //  probeCacheFwd(std::vector<char> candidateSet, std::vector<int32_t>
  //  &candidateCount, address_t startAddress);

  bool _plru;
  Cache* _cache;
};

#endif /* FILLEDCACHEPROFILINGGROUPELIM_H_ */
