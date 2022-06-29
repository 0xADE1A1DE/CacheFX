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

#ifndef FILLEDCACHEPROFILINGLRU_H_
#define FILLEDCACHEPROFILINGLRU_H_

#include "FilledCacheProfiling.h"

class FilledCacheProfilingLRU : public FilledCacheProfiling
{
public:
  FilledCacheProfilingLRU(MMU* mmu, size_t cacheSize);
  FilledCacheProfilingLRU(MMU* mmu, size_t cacheSize, bool plru);
  virtual ~FilledCacheProfilingLRU();

  void createEvictionSet(Victim* victim, uint32_t numAddresses,
                         uint32_t maxIterations) override;

protected:
  void selectCandidates(std::vector<char>& candidateSet);
  void selectCandidatesPLRU(std::vector<char>& candidateSet);
  int32_t pruneCandidateSet(std::vector<char>& candidateSet,
                            address_t startAddress);
  void probeCache(std::vector<char> candidateSet,
                  std::vector<int32_t>& candidateCount, address_t startAddress);
  void probeCacheFwd(std::vector<char> candidateSet,
                     std::vector<int32_t>& candidateCount,
                     address_t startAddress);

  bool _plru;
};

#endif /* FILLEDCACHEPROFILINGLRU_H_ */
