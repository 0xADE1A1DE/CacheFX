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

#ifndef PROFILING_H_
#define PROFILING_H_

#include "Victim/SingleAccessVictim.h"
#include <Victim/Victim.h>

struct ProfilingStatistic
{
  uint32_t numProfilingRuns;
  uint32_t numEvaluationRuns;
  uint32_t numEvaluationMisses;
  uint32_t numEvaluationMissesWFlush;
  uint32_t numEvaluationMissesWEvict;
  uint32_t numEvaluationTruePositives;
  uint32_t numEvaluationFalsePositives;
  uint64_t attackMemorySize;
};

class Profiling
{
public:
  Profiling(){};
  virtual ~Profiling(){};
  virtual void createEvictionSet(Victim* v, uint32_t numAddresses,
                                 uint32_t maxIterations) = 0;
  virtual void evaluateEvictionSet(Victim* victim,
                                   uint32_t numEvaluationRuns) = 0;
  virtual ProfilingStatistic getStatistics() const = 0;
};

#endif /* PROFILING_H_ */
