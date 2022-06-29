/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Mar 3, 2020
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

#ifndef EVALUATION_H_
#define EVALUATION_H_

#include <string>

#include <Cache/Cache.h>
#include <statistics.h>
#include <types.h>

struct EvaluationCacheCfg
{
  std::string cacheType;
  size_t nSets;
  size_t nWays;
  replAlg algorithm;
  int32_t param[4];
};

template <typename T, typename Z, typename Y>
struct ProfilingEvaluationStatistic
{
  Z evictionSetSize;
  T evictionSetTruePositiveRate;
  T evictionSR;
  T evictionSRWFlush;
  T evictionSRWEvict;
  Y attackMemorySize;
  CacheStatistics<Y> cacheStatsVictim;
  CacheStatistics<Y> cacheStatsAttacker;
  uint32_t profilingIterations;
  uint32_t evaluationIterations;
  uint32_t numStatisticalRuns;
  EvaluationCacheCfg cacheCfg;
};

#endif /* EVALUATION_H_ */
