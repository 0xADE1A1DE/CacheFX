/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Feb 19, 2020
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

#include <Cache/CacheHierarchy.h>
#include <Cache/WayPartitionCache.h>
#include <CacheFactory/CacheFactory.h>
#include <EntropyLoss.h>
#include <MMU/DirectMMU.h>
#include <MMU/MMU.h>
#include <Victim/SingleAccessVictim.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>

const char* EntropyLoss::INFORMATION_FLOW = "InformationFlow";
const char* EntropyLoss::PATTERN_LINEAR = "LinearPattern";
const char* EntropyLoss::PATTERN_INCREASING_STRIDE = "IncStridePattern";
const char* EntropyLoss::PATTERN_RANDOM = "RandomPattern";

EntropyLoss::EntropyLoss()
{
  // TODO Auto-generated constructor stub
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

EntropyLoss::~EntropyLoss()
{
  // TODO Auto-generated destructor stub
}

void EntropyLoss::promoteCacheLineFront(tag_t tag)
{
  auto itCl = std::find(_faCache.begin(), _faCache.end(), tag);
  _faCache.push_front(tag);
  _faCache.erase(itCl);
}

void EntropyLoss::insertCacheLineFront(tag_t tag) { _faCache.push_front(tag); }

void EntropyLoss::replaceAndPromoteCacheLineFront(tag_t oldTag, tag_t newTag)
{
  uint32_t rank = 0;
  auto itCl = _faCache.begin();
  while (itCl != _faCache.end())
  {
    if (*itCl == oldTag)
    {
      _faCache.erase(itCl);
      break;
    }
    itCl++;
    rank++;
  }
  assert(rank < _evictionCnt.size());
  _faCache.push_front(newTag);
  _evictionCnt[rank]++;
}

void EntropyLoss::accessAndTrackOrder(MemHandle* memHandle, address_t offset)
{
  std::list<CacheResponse> responses;
  tag_t tag = memHandle->translate(offset) / CACHE_LINE_SIZE;
  memHandle->read(offset, responses);
  for (auto resp : responses)
  {
    if (resp._level != CACHE_LEVEL_UNKNOWN)
    {
      if (resp._hit)
      {
        promoteCacheLineFront(tag);
      }
      else
      {
        if (resp._eviction)
        {
          replaceAndPromoteCacheLineFront(resp._evictedTag, tag);
        }
        else
        {
          insertCacheLineFront(tag);
        }
      }
    }
  }
}

void EntropyLoss::accessAndTrackContent(MemHandle* memHandle, address_t offset)
{
  std::list<CacheResponse> responses;
  tag_t tag = memHandle->translate(offset) / CACHE_LINE_SIZE;
  memHandle->read(offset, responses);
  for (auto resp : responses)
  {
    if (resp._level != CACHE_LEVEL_UNKNOWN)
    {
      if (!resp._hit)
      {
        if (resp._eviction)
        {
          auto itCl =
              std::find(_faCache.begin(), _faCache.end(), resp._evictedTag);
          if (itCl != _faCache.end())
          {
            _faCache.erase(itCl);
          }
          else
          {
            std::cout << "WARNING: line evicted by cache (tag: "
                      << resp._evictedTag << ") not found!" << std::endl;
          }
          _faCache.push_front(tag);
        }
        else
        {
          _faCache.push_front(tag);
        }
      }
    }
  }
}

void EntropyLoss::flushCache(DirectMMU* mmu)
{
  for (auto tag : _faCache)
  {
    mmu->getCache()->evict(tag);
  }

  _faCache.clear();
  _evictionCnt.clear();
  _usageCnt.clear();
}

void EntropyLoss::prefillCache(DirectMMU* mmu)
{
  uint32_t sizeMemRange = mmu->getCache()->getNLines() * CACHE_LINE_SIZE;
  MemHandle* flushHandle = mmu->allocate("FCFlushRange", sizeMemRange, false);
  for (uint32_t offset = 0; offset < sizeMemRange; offset += CACHE_LINE_SIZE)
  {
    accessAndTrackOrder(
        flushHandle, rand() % (mmu->getCache()->getNLines() * CACHE_LINE_SIZE));
  }
  mmu->free(flushHandle);
}

double EntropyLoss::randomPattern(DirectMMU* mmu, MemHandle* memHandle,
                                  int32_t rep, int32_t len)
{
  flushCache(mmu);
  _evictionCnt.resize(mmu->getCache()->getNLines(), 0);

  prefillCache(mmu);

  srand(time(0));

  for (uint32_t r = 0; r < rep; r++)
  {
    for (uint32_t i = 0; i < len; i += 1)
    {
      address_t offset = (rand() % len) * CACHE_LINE_SIZE;
      accessAndTrackOrder(memHandle, offset);
    }
  }

  return calcInformationLoss();
}

double EntropyLoss::linearPattern(DirectMMU* mmu, MemHandle* memHandle,
                                  int32_t rep, int32_t stride, int32_t len)
{
  flushCache(mmu);
  _evictionCnt.resize(mmu->getCache()->getNLines(), 0);

  prefillCache(mmu);

  for (uint32_t r = 0; r < rep; r++)
  {
    for (uint32_t i = 0; i < len; i += stride)
    {
      accessAndTrackOrder(memHandle, i * CACHE_LINE_SIZE);
    }
  }

  return calcInformationLoss();
}

double EntropyLoss::increasingStridePattern(DirectMMU* mmu,
                                            MemHandle* memHandle, int32_t rep,
                                            int32_t stride, int32_t len)
{
  flushCache(mmu);
  _evictionCnt.resize(mmu->getCache()->getNLines(), 0);

  prefillCache(mmu);

  for (uint32_t r = 0; r <= rep; r++)
  {
    for (uint32_t s = 1; s <= stride; s++)
    {
      int32_t i = 0;
      for (i = 0; i < len / s; i += s)
      {
        accessAndTrackOrder(memHandle, i * CACHE_LINE_SIZE);
      }
    }
  }

  return calcInformationLoss();
}

double EntropyLoss::measureInformationFlow(uint32_t numExperiments,
                                           uint32_t ASMult)
{
  std::unique_ptr<DirectMMU> mmu(
      new DirectMMU(std::move(std::unique_ptr<Cache>(
                        CacheFactory::Instance()->loadConfigurationFromFile())),
                    true));

  int32_t numCacheLines = ASMult * mmu->getCache()->getNLines();
  MemHandle* memHandle = mmu->allocate(std::string("IFlowMemoryChunk"),
                                       numCacheLines * CACHE_LINE_SIZE, false);
  SingleAccessVictim victim(mmu.get(), numCacheLines * CACHE_LINE_SIZE);
  victim.setAddress(rand() % (numCacheLines * CACHE_LINE_SIZE));

  flushCache(mmu.get());
  _evictionCnt.resize(numCacheLines, 0);
  _usageCnt.resize(numCacheLines, 0);

  // Initially prime cache entirely
  uint32_t i = 0;
  for (;
       _faCache.size() < mmu->getCache()->getNLines() && i < 50 * numCacheLines;
       i++)
  {
    address_t offset = (rand() % numCacheLines) * CACHE_LINE_SIZE;
    accessAndTrackContent(memHandle, offset);
  }

  std::string cacheType(mmu->getCache()->getCacheType());
  bool partitionedCache = false;
  if (cacheType == CacheHierarchy::CACHE_TYPESTR)
  {
    CacheHierarchy* hier = (CacheHierarchy*)mmu->getCache();
    for (int32_t i = 0; i < hier->getCacheLevels(); i++)
    {
      cacheType = hier->getCache(i)->getCacheType();
      partitionedCache |= (cacheType == WayPartitionCache::CACHE_TYPESTR);
    }
  }
  else if (cacheType == WayPartitionCache::CACHE_TYPESTR)
  {
    partitionedCache = true;
  }
  assert(partitionedCache || _faCache.size() == mmu->getCache()->getNLines());

  for (int32_t rep = 0; rep < numExperiments; rep++)
  {
    // Prime cache with different set of addresses
    for (uint32_t i = 0; i < (mmu->getCache()->getNLines() / 4); i++)
    {
      address_t offset = (rand() % numCacheLines) * CACHE_LINE_SIZE;
      accessAndTrackContent(memHandle, offset);
    }

    std::list<tag_t> originalCacheState(_faCache);
    int32_t numEvictions = 0;

    // Let victim access and learn about eviction
    std::list<CacheResponse> responses;
    victim.accessAddress(responses);
    for (auto resp : responses)
    {
      if (resp._eviction)
      {
        auto itCl =
            std::find(_faCache.begin(), _faCache.end(), resp._evictedTag);
        if (itCl != _faCache.end())
        {
          auto relativeTag =
              resp._evictedTag % (ASMult * mmu->getCache()->getNLines());
          // Track evicted tags, which mark information flow
          _evictionCnt[relativeTag]++;
          _faCache.remove(resp._evictedTag);
          numEvictions++;
        }
        else
        {
          std::cout << "WARNING: line evicted by cache (tag: "
                    << resp._evictedTag << ") not found!" << std::endl;
        }
      }
    }

    // Keep track of which addresses are in the cache
    for (auto tag : originalCacheState)
    {
      tag = tag % (ASMult * mmu->getCache()->getNLines());
      _usageCnt[tag] += numEvictions;
    }

    victim.invalidateAddress();
  }

  // Compute entropy loss
  double flow = calcInformationFlow(mmu->getCache()->getNLines());

  mmu->free(memHandle);

  return flow;
}

void EntropyLoss::evaluate()
{
  _evaluationConfigs.clear();
  _evaluationResults.clear();

  std::unique_ptr<DirectMMU> mmu(
      new DirectMMU(std::move(std::unique_ptr<Cache>(
                        CacheFactory::Instance()->loadConfigurationFromFile())),
                    true));

  MemHandle* memHandle = mmu->allocate(
      std::string("MemoryChunk"),
      (mmu->getCache()->getNLines() * mmu->getCache()->getNLines()) *
          CACHE_LINE_SIZE,
      false);

  // Config: evaluationType, numExperiments, stride, addressSpaceMultiplier
  /*_evaluationConfigs.push_back(std::tuple<const char*, uint32_t, uint32_t,
  uint32_t> (PATTERN_LINEAR, 10, 1, 8));
  _evaluationConfigs.push_back(std::tuple<const char*, uint32_t, uint32_t,
  uint32_t> (PATTERN_LINEAR, 10, 1, 16));
  _evaluationConfigs.push_back(std::tuple<const char*, uint32_t, uint32_t,
  uint32_t> (PATTERN_LINEAR, 10, 1, 32));
  _evaluationConfigs.push_back(std::tuple<const char*, uint32_t, uint32_t,
  uint32_t> (PATTERN_LINEAR, 10, 4, 8));
  _evaluationConfigs.push_back(std::tuple<const char*, uint32_t, uint32_t,
  uint32_t> (PATTERN_LINEAR, 10, 4, 16));
  _evaluationConfigs.push_back(std::tuple<const char*, uint32_t, uint32_t,
  uint32_t> (PATTERN_LINEAR, 10, 4, 32));
  _evaluationConfigs.push_back(std::tuple<const char*, uint32_t, uint32_t,
  uint32_t> (PATTERN_LINEAR, 10, mmu->getCache()->getNLines(),
  mmu->getCache()->getNLines())); _evaluationConfigs.push_back(std::tuple<const
  char*, uint32_t, uint32_t, uint32_t> (PATTERN_INCREASING_STRIDE, 10, 4, 16));
  _evaluationConfigs.push_back(std::tuple<const char*, uint32_t, uint32_t,
  uint32_t> (PATTERN_INCREASING_STRIDE, 10, 8, 16));
  _evaluationConfigs.push_back(std::tuple<const char*, uint32_t, uint32_t,
  uint32_t> (PATTERN_INCREASING_STRIDE, 10, 16, 16));
  _evaluationConfigs.push_back(std::tuple<const char*, uint32_t, uint32_t,
  uint32_t> (PATTERN_RANDOM, 10, 0, 4));*/
  _evaluationConfigs.push_back(
      std::tuple<const char*, uint32_t, uint32_t, uint32_t>(INFORMATION_FLOW,
                                                            100000, 0, 16));

  _evaluationCacheCfg.algorithm = mmu->getCache()->getAlgorithm();
  _evaluationCacheCfg.cacheType = mmu->getCache()->getCacheType();
  if (_evaluationCacheCfg.cacheType == CacheHierarchy::CACHE_TYPESTR)
  {
    CacheHierarchy* ch = (CacheHierarchy*)mmu->getCache();
    if (ch->getCacheLevels() > 0)
    {
      _evaluationCacheCfg.cacheType = ch->getCache(0)->getCacheType();
    }
  }
  _evaluationCacheCfg.nSets = mmu->getCache()->getNSets();
  _evaluationCacheCfg.nWays = mmu->getCache()->getNWays();
  _evaluationCacheCfg.param[0] = mmu->getCache()->getParam(0);
  _evaluationCacheCfg.param[1] = mmu->getCache()->getParam(1);
  _evaluationCacheCfg.param[2] = mmu->getCache()->getParam(2);
  _evaluationCacheCfg.param[3] = mmu->getCache()->getParam(3);

  for (auto cfg : _evaluationConfigs)
  {
    std::string evaluationType(std::get<0>(cfg));
    std::cout << "Start evaluation " << evaluationType << " - "
              << std::get<1>(cfg) << " - " << std::get<2>(cfg) << " - "
              << std::get<3>(cfg) << std::endl;
    if (evaluationType == PATTERN_LINEAR)
    {
      _evaluationResults.push_back(linearPattern(
          mmu.get(), memHandle, std::get<1>(cfg), std::get<2>(cfg),
          std::get<3>(cfg) * mmu->getCache()->getNLines()));
    }
    else if (evaluationType == PATTERN_INCREASING_STRIDE)
    {
      _evaluationResults.push_back(increasingStridePattern(
          mmu.get(), memHandle, std::get<1>(cfg), std::get<2>(cfg),
          std::get<3>(cfg) * mmu->getCache()->getNLines()));
    }
    else if (evaluationType == PATTERN_RANDOM)
    {
      _evaluationResults.push_back(
          randomPattern(mmu.get(), memHandle, std::get<1>(cfg),
                        std::get<3>(cfg) * mmu->getCache()->getNLines()));
    }
    else if (evaluationType == INFORMATION_FLOW)
    {
      size_t iterations = mmu->getCache()->getNLines() * std::get<1>(cfg) / 256;
      _evaluationResults.push_back(
          measureInformationFlow(iterations, std::get<3>(cfg)));
    }
  }
}

void EntropyLoss::printStatistics(std::string& outputFilePath)
{
  std::string delimiter(";");
  std::ofstream outputFile;
  outputFile.open(outputFilePath);
  outputFile << "CfgEvaluationType";
  outputFile << delimiter << "CfgCacheType" << delimiter << "CfgNSets"
             << delimiter << "CfgNWays" << delimiter << "CfgReplAlg"
             << delimiter << "CfgCParam0" << delimiter << "CfgCParam1"
             << delimiter << "CfgCParam2" << delimiter << "CfgCParam3";
  outputFile << delimiter << "CfgNExperiments" << delimiter << "CfgStride"
             << delimiter << "CfgASMultiplier";
  outputFile << delimiter << "Entropy" << std::endl;

  for (size_t i = 0; i < _evaluationConfigs.size(); i++)
  {
    auto cfg = _evaluationConfigs[i];
    auto result = _evaluationResults[i];
    outputFile << std::get<0>(cfg);

    outputFile << delimiter << _evaluationCacheCfg.cacheType << delimiter
               << _evaluationCacheCfg.nSets << delimiter
               << _evaluationCacheCfg.nWays << delimiter
               << repAlgStrMap[_evaluationCacheCfg.algorithm] << delimiter
               << _evaluationCacheCfg.param[0] << delimiter
               << _evaluationCacheCfg.param[1] << delimiter
               << _evaluationCacheCfg.param[2] << delimiter
               << _evaluationCacheCfg.param[2];

    outputFile << delimiter << std::get<1>(cfg) << delimiter << std::get<2>(cfg)
               << delimiter << std::get<2>(cfg);

    outputFile << delimiter << result << std::endl;
  }
  outputFile.close();
}

void EntropyLoss::printStatistics()
{
  for (size_t i = 0; i < _evaluationConfigs.size(); i++)
  {
    auto stats = _evaluationResults[i];
    auto cfg = _evaluationConfigs[i];
    std::cout << std::get<0>(cfg) << " w/ config " << std::get<1>(cfg) << "/"
              << std::get<2>(cfg) << "/" << std::get<3>(cfg) << ": ";
    std::cout << stats << std::endl;
  }
}

/*void EntropyLoss::measureEvictionDistribution() {
  std::unique_ptr<DirectMMU> mmu(
      new
DirectMMU(std::move(std::unique_ptr<Cache>(CacheFactory::Instance()->loadConfigurationFromFile())),
          true));

  MemHandle *memHandle = mmu->allocate(std::string("MemoryChunk"),
(mmu->getCache()->getNLines()*mmu->getCache()->getNLines()) * CACHE_LINE_SIZE,
false);

  int32_t rep = 10;

  std::cout << "Linear Pattern " << rep << "/1/8x - Entropy Loss: " <<
      linearPattern(mmu.get(), memHandle, rep, 1,
8*mmu->getCache()->getNLines()) << " bits " << std::endl; printDistribution();
  std::cout << "Linear Pattern " << rep << "/1/16x - Entropy Loss: " <<
      linearPattern(mmu.get(), memHandle, rep, 1,
16*mmu->getCache()->getNLines()) << " bits " << std::endl; printDistribution();
  std::cout << "Linear Pattern " << rep << "/1/32x - Entropy Loss: " <<
      linearPattern(mmu.get(), memHandle, rep, 1,
32*mmu->getCache()->getNLines()) << " bits " << std::endl; printDistribution();
  std::cout << "Linear Pattern " << rep << "/4/8x - Entropy Loss: " <<
      linearPattern(mmu.get(), memHandle, rep, 4,
8*mmu->getCache()->getNLines()) << " bits " << std::endl; printDistribution();
  std::cout << "Linear Pattern " << rep << "/4/16x - Entropy Loss: " <<
      linearPattern(mmu.get(), memHandle, rep, 4,
16*mmu->getCache()->getNLines()) << " bits " << std::endl; printDistribution();
  std::cout << "Linear Pattern " << rep << "/4/32x - Entropy Loss: " <<
      linearPattern(mmu.get(), memHandle, rep, 4,
32*mmu->getCache()->getNLines()) << " bits " << std::endl; printDistribution();
  std::cout << "Linear Pattern " << rep << "/1x/CS^2 - Entropy Loss: " <<
      linearPattern(mmu.get(), memHandle, rep, 1*mmu->getCache()->getNLines(),
mmu->getCache()->getNLines()*mmu->getCache()->getNLines()) << " bits " <<
std::endl; printDistribution(); std::cout << "Increasing Stride Pattern " << rep
<< "/1-4/16x - Entropy Loss: " << increasingStridePattern(mmu.get(), memHandle,
rep, 4, 16*mmu->getCache()->getNLines()) << " bits " << std::endl;
  printDistribution();
  std::cout << "Increasing Stride Pattern " << rep << "/1-8/16x - Entropy Loss:
" << increasingStridePattern(mmu.get(), memHandle, rep, 8,
16*mmu->getCache()->getNLines()) << " bits " << std::endl; printDistribution();
  std::cout << "Increasing Stride Pattern " << rep << "/1-16/16x - Entropy Loss:
" << increasingStridePattern(mmu.get(), memHandle, rep, 16,
16*mmu->getCache()->getNLines()) << " bits " << std::endl; printDistribution();
  std::cout << "Random Pattern " << rep << "/4x - Entropy Loss: " <<
      randomPattern(mmu.get(), memHandle, rep, 4*mmu->getCache()->getNLines())
<< " bits " << std::endl; printDistribution();

  auto stats = mmu->getCache()->getCacheStatistics(DEFAULT_CACHE_CONTEXT);
  std::cout << stats;

  mmu->free(memHandle);
}*/

double EntropyLoss::calcInformationLoss()
{
  double entropyLoss = 0;
  auto sumEvictions =
      std::accumulate(_evictionCnt.begin(), _evictionCnt.end(), 0);
  for (int32_t i = 0; i < _evictionCnt.size(); i++)
  {
    if (_evictionCnt[i] != 0)
    {
      double q = (double)_evictionCnt[i] / sumEvictions;
      double p = (double)1 / _evictionCnt.size();
      entropyLoss += (double)_evictionCnt[i] / sumEvictions *
                     std::log2((double)(_evictionCnt.size() * _evictionCnt[i]) /
                               sumEvictions); // q * log (q/p)
      // entropyLoss -= p * std::log2(q/p)    // p * log (p/q) = - p * log (q/p)
    }
  }
  return entropyLoss;
}

double EntropyLoss::calcInformationFlow(size_t cacheSize)
{
  double avgUsages = 0;
  uint32_t evictedAddresses = 0;
  for (int32_t i = 0; i < _evictionCnt.size(); i++)
  {
    if (_evictionCnt[i] != 0)
    {
      evictedAddresses++;
      avgUsages += _usageCnt[i];
    }
  }
  avgUsages /= evictedAddresses;

  double sumEvictions = 0;
  double sumWeightedEvictions = 0;
  std::vector<double> weightedEvictionCnt(_evictionCnt.size(), 0);

  for (int32_t i = 0; i < _evictionCnt.size(); i++)
  {
    weightedEvictionCnt[i] = (double)_evictionCnt[i] * _usageCnt[i] / avgUsages;
    sumWeightedEvictions += weightedEvictionCnt[i];
    sumEvictions += _evictionCnt[i];
  }

  double entropyLoss = 0;
  double entropyP = 0;
  double entropyQ = 0;

  for (int32_t i = 0; i < _evictionCnt.size(); i++)
  {
    double p = (double)_usageCnt[i] / (sumEvictions * cacheSize);
    double q = weightedEvictionCnt[i] / sumWeightedEvictions;
    if (_evictionCnt[i] != 0)
    {
      entropyLoss += q * std::log2(q / p);
    }
    entropyP -= p * std::log2(p);
    if (_evictionCnt[i] != 0)
      entropyQ -= q * std::log2(q);
  }

  return entropyLoss;
}

void EntropyLoss::printDistribution()
{
  const bool MUTE = true;
  if (!MUTE)
  {
    for (int32_t r = 0; r < _evictionCnt.size(); r++)
    {
      std::cout << _evictionCnt[r] << " ";
    }
    std::cout << std::endl;
  }
}
