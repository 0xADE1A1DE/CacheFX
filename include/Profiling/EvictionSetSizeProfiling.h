/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Apr 23, 2020
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

#ifndef EVICTIONSETSIZEPROFILING_H_
#define EVICTIONSETSIZEPROFILING_H_

#include <Profiling/Profiling.h>
#include <vector>

class MemHandle;

class EvictionAttacker;

class EvictionSetSizeProfiling : public Profiling
{
public:
  EvictionSetSizeProfiling(MMU* mmu, Cache* cache);
  virtual ~EvictionSetSizeProfiling();

  void createEvictionSet(Victim* victim, uint32_t numAddresses,
                         uint32_t maxIterations) override;

  bool createEvictionSet(Victim* victim, uint32_t numAddresses,
                         uint32_t maxIterations, MemHandle* memHandle,
                         std::vector<size_t>& evSet,
                         const bool efficiencyTest = false,
                         const uint64_t targetEvictionSetSize = TAG_NONE,
                         const double targetEvictionSetEffectiveness = 0.9)
  {
    return createEvictionSet(victim, numAddresses, maxIterations, memHandle,
                             evSet, true, efficiencyTest, targetEvictionSetSize,
                             targetEvictionSetEffectiveness);
  };

  void evaluateEvictionSet(Victim* victim, uint32_t numEvaluationRuns) override;

  ProfilingStatistic getStatistics() const override;

  uint32_t getAttackMemorySize() const { return _statAttackMemorySize; }
  uint32_t getEvictionSetSize() const { return _statEvictionSetSize; }

  friend EvictionAttacker;

private:
  double testEvictionSet(Victim* victim, MemHandle* memHandle,
                         std::vector<size_t> evSet);

private:
  bool createEvictionSet(Victim* victim, uint32_t numAddresses,
                         uint32_t maxIterations, MemHandle* memHandle,
                         std::vector<size_t>& evSet, bool output,
                         const bool efficiencyTest,
                         const uint64_t targetEvictionSetSize,
                         const double targetEvictionSetEffectiveness);
  uint32_t _statAttackMemorySize;
  uint32_t _statEvictionSetSize;
  MMU* _mmu;
  Cache* _cache;
};

#endif /* EVICTIONSETSIZEPROFILING_H_ */
