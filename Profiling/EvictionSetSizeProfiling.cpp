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

#include <Cache/Cache.h>
#include <CacheFactory/CacheFactory.h>
#include <MMU/DirectMMU.h>
#include <Profiling/EvictionSetSizeProfiling.h>

#include <numeric>

using namespace std;

EvictionSetSizeProfiling::EvictionSetSizeProfiling(MMU* mmu, Cache* cache)
    : _statAttackMemorySize(0), _statEvictionSetSize(0), _mmu(mmu),
      _cache(cache)
{
}

EvictionSetSizeProfiling::~EvictionSetSizeProfiling() {}

double EvictionSetSizeProfiling::testEvictionSet(Victim* victim,
                                                 MemHandle* memHandle,
                                                 std::vector<size_t> evSet)
{
  uint32_t misses = 0;
  const uint32_t runs = 500;

  // tag_t t = victim->getMemHandle()->translate(victim->getAddress()) /
  // CACHE_LINE_SIZE;

  for (uint32_t r = 0; r < runs; r++)
  {
    victim->invalidateAddress();

    for (auto addr : evSet)
    {
      memHandle->flush(addr);
    }

    victim->accessAddress();

    for (auto addr : evSet)
    {
      memHandle->read(addr);
    }

    if (victim->accessAddress() == 0)
    { // MISS
      misses++;
    }
  }
  return ((double)misses) / runs;
}

void EvictionSetSizeProfiling::createEvictionSet(Victim* victim,
                                                 uint32_t numAddresses,
                                                 uint32_t maxIterations)
{
  uint32_t nLines = 10000 * _cache->getNLines();
  MemHandle* memHandle =
      _mmu->allocate("EVSizeTestHandle", nLines * CACHE_LINE_SIZE, false);
  std::vector<size_t> evSet;

  createEvictionSet(victim, numAddresses, maxIterations, memHandle, evSet,
                    false, TAG_NONE, 0.9);

  _mmu->free(memHandle);

  _statEvictionSetSize = evSet.size();
}

bool EvictionSetSizeProfiling::createEvictionSet(
    Victim* victim, uint32_t numAddresses, uint32_t maxIterations,
    MemHandle* memHandle, std::vector<size_t>& evSet, bool output,
    const bool efficiencyTest, const uint64_t targetEvictionSetSize,
    const double targetEvictionSetEffectiveness)
{
  uint32_t nLines = memHandle->getSize() / CACHE_LINE_SIZE;

  _statAttackMemorySize = memHandle->getSize();

  double successRate = 0;
  size_t index = 0;
  size_t testInterval = _cache->getEvictionSetSize() / 10;

  size_t testCnt = 0;
  while (successRate <= (targetEvictionSetEffectiveness -
                         numeric_limits<double>::epsilon()) &&
         index < nLines && evSet.size() < targetEvictionSetSize)
  {
    bool collisionFound = false;
    while (!collisionFound && index < nLines)
    {
      size_t address = index * CACHE_LINE_SIZE;
      if (victim->hasCollision(memHandle, address))
      {
        evSet.push_back(address);
        collisionFound = true;
        testCnt++;
      }
      index++;
    }
    if ((_cache->getEvictionSetSize() <= evSet.size() &&
         (testCnt >= testInterval)) ||
        efficiencyTest)
    {
      successRate = testEvictionSet(victim, memHandle, evSet);
      testCnt = 0;
      if (output || efficiencyTest)
        std::cout << "Current Success Rate: " << successRate
                  << " for eviction set size " << evSet.size() << std::endl;
    }
  }
  if (output)
  {
    if (index == nLines)
    {
      std::cout << "WARNING: determined the eviction set size " << evSet.size()
                << " with success rate " << successRate
                << " as no more addresses were available!" << std::endl;
    }
    else
    {
      std::cout << "Eviction set size was determined with " << evSet.size()
                << " addresses" << std::endl;
    }
  }

  return index != nLines;
}

void EvictionSetSizeProfiling::evaluateEvictionSet(Victim* victim,
                                                   uint32_t numEvaluationRuns)
{
}

ProfilingStatistic EvictionSetSizeProfiling::getStatistics() const
{
  ProfilingStatistic stats;
  stats.numEvaluationMisses = 0;
  stats.numEvaluationMissesWEvict = 0;
  stats.numEvaluationMissesWFlush = 0;
  stats.numEvaluationFalsePositives = 0;
  stats.numEvaluationTruePositives = _statEvictionSetSize;
  stats.numEvaluationRuns = 0;
  stats.numProfilingRuns = 1;
  stats.attackMemorySize = _statAttackMemorySize;

  return stats;
}
