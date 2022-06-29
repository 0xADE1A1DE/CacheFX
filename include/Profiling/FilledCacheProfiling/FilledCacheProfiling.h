/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Jan 30, 2020
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

#ifndef FILLEDCACHEPROFILING_H_
#define FILLEDCACHEPROFILING_H_

#include <Profiling/Profiling.h>
#include <vector>

class MMU;
class MemHandle;

class FilledCacheProfiling : public Profiling
{
public:
  FilledCacheProfiling(MMU* mmu, size_t cacheSize);
  virtual ~FilledCacheProfiling();
  virtual void createEvictionSet(Victim* victim, uint32_t numAddresses,
                                 uint32_t maxIterations);
  virtual void evaluateEvictionSet(Victim* victim, uint32_t numEvaluationRuns);

  uint32_t getStatNumProfilingRuns() const { return _statNumProfilingRuns; };
  uint32_t getStatNumEvaluationRuns() const { return _statNumEvaluationRuns; };
  double getStatAvgCandidateCnt() const { return _statAvgCandidateCnt; };
  uint32_t getStatNumEvaluationMisses() const
  {
    return _statNumEvaluationMisses;
  };
  uint32_t getStatNumEvaluationMissesWFlush() const
  {
    return _statNumEvaluationMissesWFlush;
  };
  uint32_t getStatNumEvaluationMissesWEvict() const
  {
    return _statNumEvaluationMissesWEvict;
  };
  uint32_t getStatNumEvaluationTruePositives() const
  {
    return _statNumEvaluationTruePositives;
  };
  uint32_t getStatNumEvaluationFalsePositives() const
  {
    return _statNumEvaluationFalsePositives;
  };

  virtual ProfilingStatistic getStatistics() const;

protected:
  void flushCacheLines(MemHandle* memHandle, address_t startAddress,
                       size_t numElements);
  void flushSet(std::vector<address_t> addressSet, bool useFlush);
  void flushCacheRandAcc();
  uint32_t testEvictionSet(Victim* victim, uint32_t numRuns,
                           bool evictAddressesBeforeRun = false,
                           bool evictWithFlush = false);
  void selectCandidates(std::vector<char>& candidateSet);
  void primeCache(std::vector<char> candidateSet, address_t startAddress);
  void probeCache(std::vector<char> candidateSet,
                  std::vector<int32_t>& candidateCount, address_t startAddress);
  int32_t pruneCandidateSet(std::vector<char>& candidateSet,
                            address_t startAddress);

protected:
  // Memory access and cache size
  MMU* _mmu;
  MemHandle* _memHandle;
  size_t _cacheSize;

  // Constructed eviction set
  std::vector<address_t> _evictionSet;

  // Statistics
  uint32_t _statNumProfilingRuns;
  uint32_t _statNumEvaluationRuns;
  double _statAvgCandidateCnt;
  uint32_t _statNumEvaluationMisses;
  uint32_t _statNumEvaluationMissesWFlush;
  uint32_t _statNumEvaluationMissesWEvict;
  uint32_t _statNumEvaluationTruePositives;
  uint32_t _statNumEvaluationFalsePositives;
  uint32_t _statAttackMemorySize;
};

#endif /* FILLEDCACHEPROFILING_H_ */
