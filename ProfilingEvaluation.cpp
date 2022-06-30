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

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <tuple>

using namespace std;

#include <Attacker/Attacker.h>
#include <Cache/Cache.h>
#include <MMU/MMU.h>
#include <MemArray.h>
#include <ProfilingEvaluation.h>
#include <Victim/Victim.h>
#include <types.h>
#include <MMU/DirectMMU.h>
#include <Profiling/EvictionSetSizeProfiling.h>
#include <Profiling/FilledCacheProfiling/FilledCacheProfiling.h>
#include <Profiling/FilledCacheProfiling/FilledCacheProfilingGroupElim.h>
#include <Profiling/FilledCacheProfiling/FilledCacheProfilingLRU.h>
#include <Profiling/FilledCacheProfiling/FilledCacheProfilingProbabilistic.h>
#include <Profiling/FilledCacheProfiling/FilledCacheProfilingSingleHold.h>
#include <Cache/CacheHierarchy.h>
#include <CacheFactory/CacheFactory.h>

const char* ProfilingEvaluation::PT_FILLED_CACHE = "FilledCacheProfiling";
const char* ProfilingEvaluation::PT_FILLED_CACHE_PROBABILISTIC =
    "FilledCacheProfilingProbabilistic";
const char* ProfilingEvaluation::PT_FILLED_CACHE_PROBABILISTIC_PROFILED =
    "FilledCacheProfilingProbabilisticProfiled";
const char* ProfilingEvaluation::PT_FILLED_CACHE_LRU =
    "FilledCacheProfilingLRU";
const char* ProfilingEvaluation::PT_FILLED_CACHE_PLRU =
    "FilledCacheProfilingPLRU";
const char* ProfilingEvaluation::PT_FILLED_CACHE_SINGLE_HOLD =
    "FilledCacheProfilingSingleHold";
const char* ProfilingEvaluation::PT_FILLED_CACHE_SINGLE_HOLD_P =
    "FilledCacheProfilingSingleHoldP";
const char* ProfilingEvaluation::PT_FILLED_CACHE_GROUP_ELIM =
    "FilledCacheProfilingGroupElim";
const char* ProfilingEvaluation::PT_FILLED_CACHE_GROUP_ELIM_P =
    "FilledCacheProfilingGroupElimP";
const char* ProfilingEvaluation::PT_EVSET_SIZE_P90 = "EvictionSetSizeP90";

ProfilingEvaluation::ProfilingEvaluation()
{
  repAlgStrMap[REPL_LRU] = "lru";
  repAlgStrMap[REPL_BIT_PLRU] = "bit-plru";
  repAlgStrMap[REPL_TREE_PLRU] = "tree-plru";
  repAlgStrMap[REPL_RANDOM] = "random";
  repAlgStrMap[REPL_SRRIP] = "srrip";
  repAlgStrMap[REPL_BRRIP] = "brrip";
  repAlgStrMap[REPL_DRRIP] = "drrip";
  repAlgStrMap[REPL_LIP] = "lip";
  repAlgStrMap[REPL_BIP] = "bip";
  repAlgStrMap[REPL_DIP] = "dip";
}

ProfilingEvaluation::~ProfilingEvaluation() {}

CacheStatistics<StatisticalValue<uint64_t>>
ProfilingEvaluation::computeStatistics(
    std::vector<CacheStatistics<uint64_t>> vec)
{
  CacheStatistics<StatisticalValue<uint64_t>> result;

  std::vector<uint64_t> rdHits, rdMisses, rdEvictions;
  std::vector<uint64_t> wrHits, wrMisses, wrEvictions;
  std::vector<uint64_t> exeHits, exeMisses, exeEvictions;
  std::vector<uint64_t> invHits, invMisses;

  for (auto v : vec)
  {
    rdHits.push_back(v.rdHits);
    rdMisses.push_back(v.rdMisses);
    rdEvictions.push_back(v.rdEvictions);

    wrHits.push_back(v.wrHits);
    wrMisses.push_back(v.wrMisses);
    wrEvictions.push_back(v.wrEvictions);

    exeHits.push_back(v.execHits);
    exeMisses.push_back(v.execMisses);
    exeEvictions.push_back(v.execEvictions);

    invHits.push_back(v.invHits);
    invMisses.push_back(v.invMisses);
  }

  result.rdHits = computeStatistics(rdHits);
  result.rdMisses = computeStatistics(rdMisses);
  result.rdEvictions = computeStatistics(rdEvictions);

  result.wrHits = computeStatistics(wrHits);
  result.wrMisses = computeStatistics(wrMisses);
  result.wrEvictions = computeStatistics(wrEvictions);

  result.execHits = computeStatistics(exeHits);
  result.execMisses = computeStatistics(exeMisses);
  result.execEvictions = computeStatistics(exeEvictions);

  result.invHits = computeStatistics(invHits);
  result.invMisses = computeStatistics(invMisses);

  return result;
}

ProfilingEvaluationStatistic<StatisticalValue<double>,
                             StatisticalValue<uint32_t>,
                             StatisticalValue<uint64_t>>
ProfilingEvaluation::computeStatistics(
    std::vector<ProfilingEvaluationStatistic<double, uint32_t, uint64_t>>
        inputs)
{
  ProfilingEvaluationStatistic<StatisticalValue<double>,
                               StatisticalValue<uint32_t>,
                               StatisticalValue<uint64_t>>
      result;

  std::vector<double> srEviction;
  std::vector<double> srEvictionWFlush;
  std::vector<double> srEvictionWEvict;
  std::vector<uint32_t> szEvictionSet;
  std::vector<double> srEvictionTruePositives;
  std::vector<CacheStatistics<uint64_t>> victimStats;
  std::vector<CacheStatistics<uint64_t>> attackerStats;
  std::vector<uint64_t> szAttackMemory;

  for (auto v : inputs)
  {
    srEviction.push_back(v.evictionSR);
    srEvictionWFlush.push_back(v.evictionSRWFlush);
    srEvictionWEvict.push_back(v.evictionSRWEvict);
    szEvictionSet.push_back(v.evictionSetSize);
    srEvictionTruePositives.push_back(v.evictionSetTruePositiveRate);
    szAttackMemory.push_back(v.attackMemorySize);
    victimStats.push_back(v.cacheStatsVictim);
    attackerStats.push_back(v.cacheStatsAttacker);
  }

  // Compute median of the statistics gathered
  result.evictionSR = computeStatistics(srEviction);
  result.evictionSRWFlush = computeStatistics(srEvictionWFlush);
  result.evictionSRWEvict = computeStatistics(srEvictionWEvict);
  result.evictionSetSize = computeStatistics(szEvictionSet);
  result.evictionSetTruePositiveRate =
      computeStatistics(srEvictionTruePositives);
  result.cacheStatsVictim = computeStatistics(victimStats);
  result.cacheStatsAttacker = computeStatistics(attackerStats);
  result.attackMemorySize = computeStatistics(szAttackMemory);
  result.evictionSR.valid = result.evictionSRWFlush.valid = true;
  result.evictionSRWEvict.valid = result.evictionSetSize.valid = true;
  result.evictionSetTruePositiveRate.valid = true;

  if (inputs.size() > 0)
  {
    result.cacheCfg = inputs[0].cacheCfg;
  }
  else
  {
    result.cacheCfg.algorithm = REPL_RANDOM;
    result.cacheCfg.cacheType = "";
    result.cacheCfg.nSets = 0;
    result.cacheCfg.nWays = 0;
    result.cacheCfg.param[0] = result.cacheCfg.param[1] =
        result.cacheCfg.param[2] = result.cacheCfg.param[3] = 0;
  }

  return result;
}

Profiling* ProfilingEvaluation::createProfiling(std::string& profilingType,
                                                MMU* mmu, Cache* cache,
                                                bool useFlush)
{
  size_t cacheSize = cache->getNLines() * CACHE_LINE_SIZE;
  if (profilingType == PT_FILLED_CACHE)
  {
    return new FilledCacheProfiling(mmu, cacheSize);
  }
  else if (profilingType == PT_FILLED_CACHE_PROBABILISTIC)
  {
    return new FilledCacheProfilingProbabilistic(mmu, cacheSize, false);
  }
  else if (profilingType == PT_FILLED_CACHE_PROBABILISTIC_PROFILED)
  {
    return new FilledCacheProfilingProbabilistic(mmu, cacheSize, true);
  }
  else if (profilingType == PT_FILLED_CACHE_LRU)
  {
    return new FilledCacheProfilingLRU(mmu, cacheSize);
  }
  else if (profilingType == PT_FILLED_CACHE_PLRU)
  {
    return new FilledCacheProfilingLRU(mmu, cacheSize, true);
  }
  else if (profilingType == PT_FILLED_CACHE_SINGLE_HOLD)
  {
    return new FilledCacheProfilingSingleHold(mmu, cache);
  }
  else if (profilingType == PT_FILLED_CACHE_SINGLE_HOLD_P)
  {
    return new FilledCacheProfilingSingleHold(mmu, cache, true);
  }
  else if (profilingType == PT_FILLED_CACHE_GROUP_ELIM)
  {
    return new FilledCacheProfilingGroupElim(mmu, cache);
  }
  else if (profilingType == PT_FILLED_CACHE_GROUP_ELIM_P)
  {
    return new FilledCacheProfilingGroupElim(mmu, cache, true);
  }
  else if (profilingType == PT_EVSET_SIZE_P90)
  {
    return new EvictionSetSizeProfiling(mmu, cache);
  }
  return nullptr;
}

size_t ProfilingEvaluation::determineEvictionSetSize()
{
  std::unique_ptr<Cache> cache(
      CacheFactory::Instance()->loadConfigurationFromFile());
  std::unique_ptr<DirectMMU> mmu(new DirectMMU(std::move(cache), true));

  SingleAccessVictim victim(mmu.get(),
                            mmu->getCache()->getNLines() * CACHE_LINE_SIZE);
  victim.setAddress(std::rand());

  EvictionSetSizeProfiling evSetProf(mmu.get(), mmu->getCache());
  evSetProf.createEvictionSet(&victim, 0, 0);

  return evSetProf.getStatistics().numEvaluationTruePositives;
}

ProfilingEvaluationStatistic<StatisticalValue<double>,
                             StatisticalValue<uint32_t>,
                             StatisticalValue<uint64_t>>
ProfilingEvaluation::runProfiling(std::string& profilingType,
                                  uint32_t evictionSetSizeFactor,
                                  uint32_t numExperiments,
                                  uint32_t numProfilingsPerAddress,
                                  uint32_t numEvaluationsPerExperiment,
                                  bool useFlush)
{
  std::srand(time(NULL));

  std::vector<ProfilingEvaluationStatistic<double, uint32_t, uint64_t>> stats;

  for (uint32_t run = 0; run < numExperiments; run++)
  {
    std::unique_ptr<Cache> cache(
        CacheFactory::Instance()->loadConfigurationFromFile());
    std::unique_ptr<DirectMMU> mmu(new DirectMMU(std::move(cache), true));
    Cache* cachePtr = mmu->getCache();
    std::string cacheType = mmu->getCache()->getCacheType();
    if (cacheType == CacheHierarchy::CACHE_TYPESTR)
    {
      CacheHierarchy* ch = (CacheHierarchy*)cachePtr;
      if (ch->getCacheLevels() > 0)
      {
        cachePtr = ch->getCache(0);
        cacheType = cachePtr->getCacheType();
      }
    }
    std::unique_ptr<Profiling> profiling(
        createProfiling(profilingType, mmu.get(), mmu->getCache(), useFlush));
    SingleAccessVictim victim(mmu.get(),
                              mmu->getCache()->getNLines() * CACHE_LINE_SIZE);

    victim.setAddress(std::rand());

    size_t evictionSetSize = mmu->getCache()->getEvictionSetSize();
    // size_t evictionSetSize = determineEvictionSetSize();
    size_t numProfilingsPerExperiment =
        numProfilingsPerAddress * evictionSetSize;

    profiling->createEvictionSet(&victim, evictionSetSize,
                                 numProfilingsPerExperiment);

    ProfilingEvaluationStatistic<double, uint32_t, uint64_t> profStat;

    profStat.cacheStatsAttacker =
        mmu->getCache()->getCacheStatistics(DEFAULT_CACHE_CONTEXT);
    profStat.cacheStatsVictim =
        mmu->getCache()->getCacheStatistics(DEFAULT_CACHE_CONTEXT_VICTIM);

    profiling->evaluateEvictionSet(&victim, numEvaluationsPerExperiment);

    auto s = profiling->getStatistics();
    profStat.evictionSR = (double)s.numEvaluationMisses / s.numEvaluationRuns;
    profStat.evictionSRWFlush =
        (double)s.numEvaluationMissesWFlush / s.numEvaluationRuns;
    profStat.evictionSRWEvict =
        (double)s.numEvaluationMissesWEvict / s.numEvaluationRuns;
    profStat.evictionSetSize =
        s.numEvaluationFalsePositives + s.numEvaluationTruePositives;
    profStat.attackMemorySize = s.attackMemorySize;
    profStat.evictionSetTruePositiveRate =
        (profStat.evictionSetSize == 0)
            ? 0
            : (double)s.numEvaluationTruePositives / profStat.evictionSetSize;
    profStat.evaluationIterations = numEvaluationsPerExperiment;
    profStat.profilingIterations = numProfilingsPerExperiment;
    profStat.numStatisticalRuns = 1;
    profStat.cacheCfg.algorithm = mmu->getCache()->getAlgorithm();
    profStat.cacheCfg.cacheType = cacheType;
    profStat.cacheCfg.nSets = mmu->getCache()->getNSets();
    profStat.cacheCfg.nWays = mmu->getCache()->getNWays();
    profStat.cacheCfg.param[0] = mmu->getCache()->getParam(0);
    profStat.cacheCfg.param[1] = mmu->getCache()->getParam(1);
    profStat.cacheCfg.param[2] = mmu->getCache()->getParam(2);
    profStat.cacheCfg.param[3] = mmu->getCache()->getParam(3);

    stats.push_back(profStat);
  }

  return computeStatistics(stats);
}

void ProfilingEvaluation::printStatistics()
{
  for (size_t i = 0; i < _profilingConfigs.size(); i++)
  {
    auto stats = _profilingResults[i];
    auto cfg = _profilingConfigs[i];
    std::cout << "Statistics for " << std::get<0>(cfg) << std::endl;
    std::cout << "Config eviction set size factor: " << std::get<1>(cfg)
              << std::endl;
    std::cout << "Config num experiments: " << std::get<2>(cfg) << std::endl;
    std::cout << "Config num profiling iterations per address: "
              << std::get<3>(cfg) << std::endl;
    std::cout << "Config num evaluation runs: " << std::get<4>(cfg)
              << std::endl;
    std::cout << "Config use Flush " << std::get<5>(cfg) << std::endl;
    std::cout << "Addresses found: " << stats.evictionSetSize << std::endl;
    std::cout << "TPR: " << stats.evictionSetTruePositiveRate << std::endl;
    std::cout << "Eviction SR: " << stats.evictionSR << std::endl;
    std::cout << "Eviction SR (w/ evict): " << stats.evictionSRWEvict
              << std::endl;
    std::cout << "Eviction SR (w/ flush): " << stats.evictionSRWFlush
              << std::endl;
    std::cout << "Attacker Memory: " << stats.attackMemorySize << std::endl;
    std::cout << std::endl << "Cache Statistics - Attacker" << std::endl;
    std::cout << stats.cacheStatsAttacker;
    std::cout << std::endl << "Cache Statistics - Victim" << std::endl;
    std::cout << stats.cacheStatsVictim;
    std::cout << std::endl;
  }
}

void ProfilingEvaluation::printStatsMax(std::ofstream& outputFile,
                                        std::string& delimiter, size_t idx)
{
  auto stats = _profilingResults[idx];
  auto cfg = _profilingConfigs[idx];

  outputFile << std::get<0>(cfg) << "-MAX";

  outputFile << delimiter << stats.cacheCfg.cacheType << delimiter
             << stats.cacheCfg.nSets << delimiter << stats.cacheCfg.nWays
             << delimiter << repAlgStrMap[stats.cacheCfg.algorithm] << delimiter
             << stats.cacheCfg.param[0] << delimiter << stats.cacheCfg.param[1]
             << delimiter << stats.cacheCfg.param[2] << delimiter
             << stats.cacheCfg.param[3];

  outputFile << delimiter << std::get<1>(cfg) << delimiter << std::get<2>(cfg)
             << delimiter << std::get<3>(cfg) << delimiter << std::get<4>(cfg)
             << delimiter << std::get<5>(cfg) << delimiter;

  outputFile << stats.evictionSetSize.max << delimiter
             << stats.evictionSetTruePositiveRate.max << delimiter
             << stats.evictionSR.max << delimiter << stats.evictionSRWEvict.max
             << delimiter << stats.evictionSRWFlush.max << delimiter
             << stats.attackMemorySize.max << delimiter;

  outputFile << stats.cacheStatsAttacker.rdHits.max << delimiter
             << stats.cacheStatsAttacker.rdMisses.max << delimiter
             << stats.cacheStatsAttacker.rdEvictions.max << delimiter
             << stats.cacheStatsAttacker.wrHits.max << delimiter
             << stats.cacheStatsAttacker.wrMisses.max << delimiter
             << stats.cacheStatsAttacker.wrEvictions.max << delimiter
             << stats.cacheStatsAttacker.execHits.max << delimiter
             << stats.cacheStatsAttacker.execMisses.max << delimiter
             << stats.cacheStatsAttacker.execEvictions.max << delimiter
             << stats.cacheStatsAttacker.invHits.max << delimiter
             << stats.cacheStatsAttacker.invMisses.max << delimiter;

  outputFile << stats.cacheStatsVictim.rdHits.max << delimiter
             << stats.cacheStatsVictim.rdMisses.max << delimiter
             << stats.cacheStatsVictim.rdEvictions.max << delimiter
             << stats.cacheStatsVictim.wrHits.max << delimiter
             << stats.cacheStatsVictim.wrMisses.max << delimiter
             << stats.cacheStatsVictim.wrEvictions.max << delimiter
             << stats.cacheStatsVictim.execHits.max << delimiter
             << stats.cacheStatsVictim.execMisses.max << delimiter
             << stats.cacheStatsVictim.execEvictions.max << delimiter
             << stats.cacheStatsVictim.invHits.max << delimiter
             << stats.cacheStatsVictim.invMisses.max << std::endl;
}

void ProfilingEvaluation::printStatsMin(std::ofstream& outputFile,
                                        std::string& delimiter, size_t idx)
{
  auto stats = _profilingResults[idx];
  auto cfg = _profilingConfigs[idx];

  outputFile << std::get<0>(cfg) << "-MIN";

  outputFile << delimiter << stats.cacheCfg.cacheType << delimiter
             << stats.cacheCfg.nSets << delimiter << stats.cacheCfg.nWays
             << delimiter << repAlgStrMap[stats.cacheCfg.algorithm] << delimiter
             << stats.cacheCfg.param[0] << delimiter << stats.cacheCfg.param[1]
             << delimiter << stats.cacheCfg.param[2] << delimiter
             << stats.cacheCfg.param[3];

  outputFile << delimiter << std::get<1>(cfg) << delimiter << std::get<2>(cfg)
             << delimiter << std::get<3>(cfg) << delimiter << std::get<4>(cfg)
             << delimiter << std::get<5>(cfg) << delimiter;

  outputFile << stats.evictionSetSize.min << delimiter
             << stats.evictionSetTruePositiveRate.min << delimiter
             << stats.evictionSR.min << delimiter << stats.evictionSRWEvict.min
             << delimiter << stats.evictionSRWFlush.min << delimiter
             << stats.attackMemorySize.min << delimiter;

  outputFile << stats.cacheStatsAttacker.rdHits.min << delimiter
             << stats.cacheStatsAttacker.rdMisses.min << delimiter
             << stats.cacheStatsAttacker.rdEvictions.min << delimiter
             << stats.cacheStatsAttacker.wrHits.min << delimiter
             << stats.cacheStatsAttacker.wrMisses.min << delimiter
             << stats.cacheStatsAttacker.wrEvictions.min << delimiter
             << stats.cacheStatsAttacker.execHits.min << delimiter
             << stats.cacheStatsAttacker.execMisses.min << delimiter
             << stats.cacheStatsAttacker.execEvictions.min << delimiter
             << stats.cacheStatsAttacker.invHits.min << delimiter
             << stats.cacheStatsAttacker.invMisses.min << delimiter;

  outputFile << stats.cacheStatsVictim.rdHits.min << delimiter
             << stats.cacheStatsVictim.rdMisses.min << delimiter
             << stats.cacheStatsVictim.rdEvictions.min << delimiter
             << stats.cacheStatsVictim.wrHits.min << delimiter
             << stats.cacheStatsVictim.wrMisses.min << delimiter
             << stats.cacheStatsVictim.wrEvictions.min << delimiter
             << stats.cacheStatsVictim.execHits.min << delimiter
             << stats.cacheStatsVictim.execMisses.min << delimiter
             << stats.cacheStatsVictim.execEvictions.min << delimiter
             << stats.cacheStatsVictim.invHits.min << delimiter
             << stats.cacheStatsVictim.invMisses.min << std::endl;
}

void ProfilingEvaluation::printStatsAvg(std::ofstream& outputFile,
                                        std::string& delimiter, size_t idx)
{
  auto stats = _profilingResults[idx];
  auto cfg = _profilingConfigs[idx];

  outputFile << std::get<0>(cfg) << "-AVG";

  outputFile << delimiter << stats.cacheCfg.cacheType << delimiter
             << stats.cacheCfg.nSets << delimiter << stats.cacheCfg.nWays
             << delimiter << repAlgStrMap[stats.cacheCfg.algorithm] << delimiter
             << stats.cacheCfg.param[0] << delimiter << stats.cacheCfg.param[1]
             << delimiter << stats.cacheCfg.param[2] << delimiter
             << stats.cacheCfg.param[3];

  outputFile << delimiter << std::get<1>(cfg) << delimiter << std::get<2>(cfg)
             << delimiter << std::get<3>(cfg) << delimiter << std::get<4>(cfg)
             << delimiter << std::get<5>(cfg) << delimiter;

  outputFile << stats.evictionSetSize.avg << delimiter
             << stats.evictionSetTruePositiveRate.avg << delimiter
             << stats.evictionSR.avg << delimiter << stats.evictionSRWEvict.avg
             << delimiter << stats.evictionSRWFlush.avg << delimiter
             << stats.attackMemorySize.avg << delimiter;

  outputFile << stats.cacheStatsAttacker.rdHits.avg << delimiter
             << stats.cacheStatsAttacker.rdMisses.avg << delimiter
             << stats.cacheStatsAttacker.rdEvictions.avg << delimiter
             << stats.cacheStatsAttacker.wrHits.avg << delimiter
             << stats.cacheStatsAttacker.wrMisses.avg << delimiter
             << stats.cacheStatsAttacker.wrEvictions.avg << delimiter
             << stats.cacheStatsAttacker.execHits.avg << delimiter
             << stats.cacheStatsAttacker.execMisses.avg << delimiter
             << stats.cacheStatsAttacker.execEvictions.avg << delimiter
             << stats.cacheStatsAttacker.invHits.avg << delimiter
             << stats.cacheStatsAttacker.invMisses.avg << delimiter;

  outputFile << stats.cacheStatsVictim.rdHits.avg << delimiter
             << stats.cacheStatsVictim.rdMisses.avg << delimiter
             << stats.cacheStatsVictim.rdEvictions.avg << delimiter
             << stats.cacheStatsVictim.wrHits.avg << delimiter
             << stats.cacheStatsVictim.wrMisses.avg << delimiter
             << stats.cacheStatsVictim.wrEvictions.avg << delimiter
             << stats.cacheStatsVictim.execHits.avg << delimiter
             << stats.cacheStatsVictim.execMisses.avg << delimiter
             << stats.cacheStatsVictim.execEvictions.avg << delimiter
             << stats.cacheStatsVictim.invHits.avg << delimiter
             << stats.cacheStatsVictim.invMisses.avg << std::endl;
}

void ProfilingEvaluation::printStatsMedian(std::ofstream& outputFile,
                                           std::string& delimiter, size_t idx)
{
  auto stats = _profilingResults[idx];
  auto cfg = _profilingConfigs[idx];

  outputFile << std::get<0>(cfg) << "-MEDIAN";

  outputFile << delimiter << stats.cacheCfg.cacheType << delimiter
             << stats.cacheCfg.nSets << delimiter << stats.cacheCfg.nWays
             << delimiter << repAlgStrMap[stats.cacheCfg.algorithm] << delimiter
             << stats.cacheCfg.param[0] << delimiter << stats.cacheCfg.param[1]
             << delimiter << stats.cacheCfg.param[2] << delimiter
             << stats.cacheCfg.param[3];

  outputFile << delimiter << std::get<1>(cfg) << delimiter << std::get<2>(cfg)
             << delimiter << std::get<3>(cfg) << delimiter << std::get<4>(cfg)
             << delimiter << std::get<5>(cfg) << delimiter;

  outputFile << stats.evictionSetSize.median << delimiter
             << stats.evictionSetTruePositiveRate.median << delimiter
             << stats.evictionSR.median << delimiter
             << stats.evictionSRWEvict.median << delimiter
             << stats.evictionSRWFlush.median << delimiter
             << stats.attackMemorySize.median << delimiter;

  outputFile << stats.cacheStatsAttacker.rdHits.median << delimiter
             << stats.cacheStatsAttacker.rdMisses.median << delimiter
             << stats.cacheStatsAttacker.rdEvictions.median << delimiter
             << stats.cacheStatsAttacker.wrHits.median << delimiter
             << stats.cacheStatsAttacker.wrMisses.median << delimiter
             << stats.cacheStatsAttacker.wrEvictions.median << delimiter
             << stats.cacheStatsAttacker.execHits.median << delimiter
             << stats.cacheStatsAttacker.execMisses.median << delimiter
             << stats.cacheStatsAttacker.execEvictions.median << delimiter
             << stats.cacheStatsAttacker.invHits.median << delimiter
             << stats.cacheStatsAttacker.invMisses.median << delimiter;

  outputFile << stats.cacheStatsVictim.rdHits.median << delimiter
             << stats.cacheStatsVictim.rdMisses.median << delimiter
             << stats.cacheStatsVictim.rdEvictions.median << delimiter
             << stats.cacheStatsVictim.wrHits.median << delimiter
             << stats.cacheStatsVictim.wrMisses.median << delimiter
             << stats.cacheStatsVictim.wrEvictions.median << delimiter
             << stats.cacheStatsVictim.execHits.median << delimiter
             << stats.cacheStatsVictim.execMisses.median << delimiter
             << stats.cacheStatsVictim.execEvictions.median << delimiter
             << stats.cacheStatsVictim.invHits.median << delimiter
             << stats.cacheStatsVictim.invMisses.median << std::endl;
}

void ProfilingEvaluation::printStatsVar(std::ofstream& outputFile,
                                        std::string& delimiter, size_t idx)
{
  auto stats = _profilingResults[idx];
  auto cfg = _profilingConfigs[idx];

  outputFile << std::get<0>(cfg) << "-VAR";

  outputFile << delimiter << stats.cacheCfg.cacheType << delimiter
             << stats.cacheCfg.nSets << delimiter << stats.cacheCfg.nWays
             << delimiter << repAlgStrMap[stats.cacheCfg.algorithm] << delimiter
             << stats.cacheCfg.param[0] << delimiter << stats.cacheCfg.param[1]
             << delimiter << stats.cacheCfg.param[2] << delimiter
             << stats.cacheCfg.param[3];

  outputFile << delimiter << std::get<1>(cfg) << delimiter << std::get<2>(cfg)
             << delimiter << std::get<3>(cfg) << delimiter << std::get<4>(cfg)
             << delimiter << std::get<5>(cfg) << delimiter;

  outputFile << stats.evictionSetSize.var << delimiter
             << stats.evictionSetTruePositiveRate.var << delimiter
             << stats.evictionSR.var << delimiter << stats.evictionSRWEvict.var
             << delimiter << stats.evictionSRWFlush.var << delimiter
             << stats.attackMemorySize.var << delimiter;

  outputFile << stats.cacheStatsAttacker.rdHits.var << delimiter
             << stats.cacheStatsAttacker.rdMisses.var << delimiter
             << stats.cacheStatsAttacker.rdEvictions.var << delimiter
             << stats.cacheStatsAttacker.wrHits.var << delimiter
             << stats.cacheStatsAttacker.wrMisses.var << delimiter
             << stats.cacheStatsAttacker.wrEvictions.var << delimiter
             << stats.cacheStatsAttacker.execHits.var << delimiter
             << stats.cacheStatsAttacker.execMisses.var << delimiter
             << stats.cacheStatsAttacker.execEvictions.var << delimiter
             << stats.cacheStatsAttacker.invHits.var << delimiter
             << stats.cacheStatsAttacker.invMisses.var << delimiter;

  outputFile << stats.cacheStatsVictim.rdHits.var << delimiter
             << stats.cacheStatsVictim.rdMisses.var << delimiter
             << stats.cacheStatsVictim.rdEvictions.var << delimiter
             << stats.cacheStatsVictim.wrHits.var << delimiter
             << stats.cacheStatsVictim.wrMisses.var << delimiter
             << stats.cacheStatsVictim.wrEvictions.var << delimiter
             << stats.cacheStatsVictim.execHits.var << delimiter
             << stats.cacheStatsVictim.execMisses.var << delimiter
             << stats.cacheStatsVictim.execEvictions.var << delimiter
             << stats.cacheStatsVictim.invHits.var << delimiter
             << stats.cacheStatsVictim.invMisses.var << std::endl;
}

void ProfilingEvaluation::printStatistics(std::string& outputFilePath)
{
  std::string delimiter(";");
  ofstream outputFile;
  outputFile.open(outputFilePath);
  outputFile << "CfgEvaluationType";
  outputFile << delimiter << "CfgCacheType" << delimiter << "CfgNSets"
             << delimiter << "CfgNWays" << delimiter << "CfgReplAlg"
             << delimiter << "CfgCParam0" << delimiter << "CfgCParam1"
             << delimiter << "CfgCParam2" << delimiter << "CfgCParam3";
  outputFile << delimiter << "CfgEvSetFactor" << delimiter << "CfgNExperiments"
             << delimiter << "CfgNProfileRuns" << delimiter << "CfgNEvalRuns"
             << delimiter << "CfgUseFlush";
  outputFile << delimiter << "AddrFound" << delimiter << "TPR" << delimiter
             << "EvSR" << delimiter << "EvSRwEvict" << delimiter << "EvSRwFlush"
             << delimiter << "AttackMem";
  outputFile << delimiter << "Attacker-RdHits" << delimiter
             << "Attacker-RdMisses" << delimiter << "Attacker-RdEvictions"
             << delimiter << "Attacker-WrHits" << delimiter
             << "Attacker-WrMisses" << delimiter << "Attacker-WrEvictions"
             << delimiter << "Attacker-ExecHits" << delimiter
             << "Attacker-ExecMisses" << delimiter << "Attacker-ExecEvictions"
             << delimiter << "Attacker-InvHits" << delimiter
             << "Attacker-InvMisses";
  outputFile << delimiter << "Victim-RdHits" << delimiter << "Victim-RdMisses"
             << delimiter << "Victim-RdEvictions" << delimiter
             << "Victim-WrHits" << delimiter << "Victim-WrMisses" << delimiter
             << "Victim-WrEvictions" << delimiter << "Victim-ExecHits"
             << delimiter << "Victim-ExecMisses" << delimiter
             << "Victim-ExecEvictions" << delimiter << "Victim-InvHits"
             << delimiter << "Victim-InvMisses" << std::endl;

  for (size_t i = 0; i < _profilingConfigs.size(); i++)
  {
    printStatsMax(outputFile, delimiter, i);
    printStatsMin(outputFile, delimiter, i);
    printStatsAvg(outputFile, delimiter, i);
    printStatsMedian(outputFile, delimiter, i);
    printStatsVar(outputFile, delimiter, i);
  }
  outputFile.close();
}

void ProfilingEvaluation::evaluate(uint32_t numExperiments)
{
  _profilingConfigs.clear();
  _profilingResults.clear();

  // Config: profilingType, evictionSetSize, numExperiments,
  // numProfilingsPerAddress, numEvaluationIterations, useFlush
  _profilingConfigs.push_back(
      std::tuple<const char*, uint32_t, uint32_t, uint32_t, uint32_t, bool>(
          PT_EVSET_SIZE_P90, 0, 1, 0, 0, true));
  _profilingConfigs.push_back(
      std::tuple<const char*, uint32_t, uint32_t, uint32_t, uint32_t, bool>(
          PT_FILLED_CACHE_LRU, 4, numExperiments, 50, 1000, true));
  _profilingConfigs.push_back(
      std::tuple<const char*, uint32_t, uint32_t, uint32_t, uint32_t, bool>(
          PT_FILLED_CACHE_PLRU, 4, numExperiments, 50, 1000, true));
  _profilingConfigs.push_back(
      std::tuple<const char*, uint32_t, uint32_t, uint32_t, uint32_t, bool>(
          PT_FILLED_CACHE, 4, numExperiments, 50, 1000, true));
  /*_profilingConfigs.push_back(std::tuple<const char*, uint32_t, uint32_t,
  uint32_t, uint32_t, bool> (PT_FILLED_CACHE_PROBABILISTIC, 4, numExperiments,
  50, 1000, true)); _profilingConfigs.push_back(std::tuple<const char*,
  uint32_t, uint32_t, uint32_t, uint32_t, bool>
                          (PT_FILLED_CACHE_PROBABILISTIC_PROFILED, 4,
  numExperiments, 50, 1000, true));*/
  _profilingConfigs.push_back(
      std::tuple<const char*, uint32_t, uint32_t, uint32_t, uint32_t, bool>(
          PT_FILLED_CACHE_SINGLE_HOLD, 4, numExperiments, 50, 1000, true));
  /*_profilingConfigs.push_back(std::tuple<const char*, uint32_t, uint32_t,
     uint32_t, uint32_t, bool> (PT_FILLED_CACHE_SINGLE_HOLD_P, 4,
     numExperiments, 50, 1000, true));*/
  _profilingConfigs.push_back(
      std::tuple<const char*, uint32_t, uint32_t, uint32_t, uint32_t, bool>(
          PT_FILLED_CACHE_GROUP_ELIM, 4, numExperiments, 50, 1000, true));
  /*_profilingConfigs.push_back(std::tuple<const char*, uint32_t, uint32_t,
     uint32_t, uint32_t, bool>
                          (PT_FILLED_CACHE_GROUP_ELIM_P, 4, numExperiments, 50,
     1000, true));*/

  for (auto cfg : _profilingConfigs)
  {
    std::string profilingType(std::get<0>(cfg));
    std::cout << "Start profiling " << profilingType << std::endl;
    _profilingResults.push_back(
        runProfiling(profilingType, std::get<1>(cfg), std::get<2>(cfg),
                     std::get<3>(cfg), std::get<4>(cfg), std::get<5>(cfg)));
  }
}
