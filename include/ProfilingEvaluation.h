/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Feb 14, 2020
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

#ifndef PROFILINGEVALUATION_H_
#define PROFILINGEVALUATION_H_

#include <Profiling/Profiling.h>
#include <algorithm>
#include <evaluation.h>
#include <ostream>
#include <string>
#include <vector>

class ProfilingEvaluation
{
public:
  ProfilingEvaluation();
  virtual ~ProfilingEvaluation();
  void evaluate(uint32_t numExperiments);

  void printStatistics();
  void printStatistics(std::string& outputFilePath);

private:
  ProfilingEvaluationStatistic<StatisticalValue<double>,
                               StatisticalValue<uint32_t>,
                               StatisticalValue<uint64_t>>
  runProfiling(std::string& profilingType, uint32_t evictionSetSizeFactor,
               uint32_t numExperiments, uint32_t numProfilingsPerAddress,
               uint32_t numEvaluationsPerExperiment, bool useFlush);
  template <typename T> T getMedian(std::vector<T> vec)
  {
    size_t medianIdx = vec.size() / 2;
    std::nth_element(vec.begin(), vec.begin() + medianIdx, vec.end());
    return vec[medianIdx];
  };
  template <typename T>
  StatisticalValue<T> computeStatistics(std::vector<T> vec)
  {
    StatisticalValue<T> result;

    result.max = std::numeric_limits<T>::lowest();
    result.min = std::numeric_limits<T>::max();
    result.avg = 0;
    result.var = 0;

    for (size_t i = 0; i < vec.size(); i++)
    {
      if (vec[i] > result.max)
      {
        result.max = vec[i];
      }
      if (vec[i] < result.min)
      {
        result.min = vec[i];
      }
      result.avg += vec[i];
    }

    result.avg /= vec.size();

    for (size_t i = 0; i < vec.size(); i++)
    {
      result.var += (vec[i] - result.avg) * (vec[i] - result.avg);
    }

    result.var /= vec.size();

    result.median = getMedian(vec);

    return result;
  };

  ProfilingEvaluationStatistic<StatisticalValue<double>,
                               StatisticalValue<uint32_t>,
                               StatisticalValue<uint64_t>>
      computeStatistics(
          std::vector<
              ProfilingEvaluationStatistic<double, uint32_t, uint64_t>>);
  CacheStatistics<StatisticalValue<uint64_t>>
  computeStatistics(std::vector<CacheStatistics<uint64_t>> vec);

  Profiling* createProfiling(std::string& profilingType, MMU* mmu, Cache* cache,
                             bool useFlush);

  void printStatsMax(std::ofstream& outputFile, std::string& delimiter,
                     size_t idx);
  void printStatsMin(std::ofstream& outputFile, std::string& delimiter,
                     size_t idx);
  void printStatsAvg(std::ofstream& outputFile, std::string& delimiter,
                     size_t idx);
  void printStatsMedian(std::ofstream& outputFile, std::string& delimiter,
                        size_t idx);
  void printStatsVar(std::ofstream& outputFile, std::string& delimiter,
                     size_t idx);

  size_t determineEvictionSetSize();

private:
  std::vector<
      std::tuple<const char*, uint32_t, uint32_t, uint32_t, uint32_t, bool>>
      _profilingConfigs;
  std::vector<ProfilingEvaluationStatistic<StatisticalValue<double>,
                                           StatisticalValue<uint32_t>,
                                           StatisticalValue<uint64_t>>>
      _profilingResults;

private:
  static const char* PT_FILLED_CACHE;
  static const char* PT_FILLED_CACHE_PROBABILISTIC;
  static const char* PT_FILLED_CACHE_PROBABILISTIC_PROFILED;
  static const char* PT_FILLED_CACHE_LRU;
  static const char* PT_FILLED_CACHE_PLRU;
  static const char* PT_FILLED_CACHE_SINGLE_HOLD;
  static const char* PT_FILLED_CACHE_SINGLE_HOLD_P;
  static const char* PT_FILLED_CACHE_GROUP_ELIM;
  static const char* PT_FILLED_CACHE_GROUP_ELIM_P;
  static const char* PT_EVSET_SIZE_P90;

  std::map<replAlg, std::string> repAlgStrMap;
};

#endif /* PROFILINGEVALUATION_H_ */
