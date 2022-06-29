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

#include <cstdlib>
#include <iostream>
#include <vector>

#include <MMU/MMU.h>
#include <MemHandle/MemHandle.h>
#include <Profiling/FilledCacheProfiling/FilledCacheProfiling.h>
#include <Victim/Victim.h>
#include <types.h>

FilledCacheProfiling::FilledCacheProfiling(MMU* mmu, size_t cacheSize)
    : _mmu(mmu), _memHandle(nullptr), _cacheSize(cacheSize), _evictionSet(0),
      _statNumProfilingRuns(0), _statNumEvaluationRuns(0),
      _statAvgCandidateCnt(0), _statNumEvaluationMisses(0),
      _statNumEvaluationMissesWFlush(0), _statNumEvaluationMissesWEvict(0),
      _statNumEvaluationTruePositives(0), _statNumEvaluationFalsePositives(0),
      _statAttackMemorySize(0)
{
}

FilledCacheProfiling::~FilledCacheProfiling()
{
  if (_memHandle != nullptr)
  {
    _mmu->free(_memHandle);
  }
}

void FilledCacheProfiling::flushCacheLines(MemHandle* memHandle,
                                           address_t startAddress,
                                           size_t numElements)
{
  for (size_t i = 0; i < numElements; i++)
  {
    memHandle->flush(startAddress + i * CACHE_LINE_SIZE);
  }
}

void FilledCacheProfiling::primeCache(std::vector<char> candidateSet,
                                      address_t startAddress)
{
  for (size_t i = 0; i < candidateSet.size(); i++)
  {
    size_t offset = i;
    address_t address = startAddress + offset * CACHE_LINE_SIZE;
    if (candidateSet[offset])
    {
      _memHandle->read(address);
      //      printf("Prime: Read: %ld\n", address);
    }
  }
}

void FilledCacheProfiling::probeCache(std::vector<char> candidateSet,
                                      std::vector<int32_t>& candidateCount,
                                      address_t startAddress)
{
  for (size_t i = 0; i < candidateSet.size(); i++)
  {
    size_t index = i;
    size_t address = startAddress + index * CACHE_LINE_SIZE;
    if (candidateSet[index])
    {
      if (_memHandle->read(address) == 0)
      {
        if (candidateCount[index] == 0)
        {
          _evictionSet.push_back(address);
        }
        candidateCount[index]++;
        break;
      }
    }
  }
}

int32_t FilledCacheProfiling::pruneCandidateSet(std::vector<char>& candidateSet,
                                                address_t startAddress)
{
  int32_t missObserved;
  int32_t candidateCnt;

  do
  {
    missObserved = 0;
    candidateCnt = 0;
    for (size_t i = 0; i < candidateSet.size(); i++)
    {
      size_t index = candidateSet.size() - i - 1;
      size_t address = startAddress + index * CACHE_LINE_SIZE;
      if (candidateSet[index] == 1)
      {
        if (_memHandle->read(address) == 0)
        {
          candidateSet[index] = 0;
          missObserved++;
        }
        else
        {
          candidateCnt++;
        }
      }
    }
  } while (missObserved);

  return candidateCnt;
}

void FilledCacheProfiling::selectCandidates(std::vector<char>& candidateSet)
{
  size_t cacheLines = _cacheSize / CACHE_LINE_SIZE;
  int32_t selectionFactor = candidateSet.size() / cacheLines;
  for (size_t i = 0; i < candidateSet.size(); i++)
  {
    candidateSet[i] = ((std::rand() % selectionFactor) == 0);
  }
}

void FilledCacheProfiling::createEvictionSet(Victim* victim,
                                             uint32_t numAddresses,
                                             uint32_t maxIterations)
{
  size_t cacheLines = _cacheSize / CACHE_LINE_SIZE;
  size_t evSetCacheRatio = (size_t)(numAddresses / _cacheSize);
  size_t candidateMult = (evSetCacheRatio + 1) * 8;
  size_t numCandidates = candidateMult * cacheLines;
  std::vector<char> candidateSet(numCandidates, 0);
  std::vector<int32_t> candidateCount(numCandidates, 0);

  _evictionSet.clear();

  if (_memHandle != nullptr)
  {
    _mmu->free(_memHandle);
  }
  _memHandle = _mmu->allocate("FCProfilerEvictionSet",
                              candidateMult * _cacheSize, false);

  address_t startAddress = 0;

  _statAttackMemorySize = candidateMult * _cacheSize;
  _statNumProfilingRuns = 0;
  _statAvgCandidateCnt = 0;

  while (_evictionSet.size() < numAddresses &&
         _statNumProfilingRuns < maxIterations)
  {
    _statNumProfilingRuns++;

    selectCandidates(candidateSet);

    primeCache(candidateSet, startAddress);

    _statAvgCandidateCnt += pruneCandidateSet(candidateSet, startAddress);

    victim->accessAddress();

    probeCache(candidateSet, candidateCount, startAddress);

    flushCacheLines(_memHandle, startAddress, numCandidates);
  }

  _statAvgCandidateCnt /= _statNumProfilingRuns;
}

void FilledCacheProfiling::flushCacheRandAcc()
{
  uint32_t sizeMemRange = _cacheSize;
  MemHandle* flushHandle = _mmu->allocate("FCFlushRange", sizeMemRange, false);
  for (uint32_t offset = 0; offset < sizeMemRange; offset += CACHE_LINE_SIZE)
  {
    flushHandle->read(offset);
  }
  _mmu->free(flushHandle);
}

void FilledCacheProfiling::flushSet(std::vector<address_t> addressSet,
                                    bool useFlush)
{
  if (useFlush)
  {
    for (auto addr : addressSet)
    {
      _memHandle->flush(addr);
    }
  }
  else
  {
    flushCacheRandAcc();
  }
}

uint32_t FilledCacheProfiling::testEvictionSet(Victim* victim, uint32_t numRuns,
                                               bool evictAddressesBeforeRun,
                                               bool evictWithFlush)
{
  uint32_t misses = 0;

  for (uint32_t r = 0; r < numRuns; r++)
  {
    if (evictAddressesBeforeRun)
    {
      flushSet(_evictionSet, evictWithFlush);
    }

    victim->accessAddress();

    for (auto addr : _evictionSet)
    {
      _memHandle->read(addr);
    }

    if (victim->accessAddress() == 0)
    { // MISS
      misses++;
    }
  }

  return misses;
}

void FilledCacheProfiling::evaluateEvictionSet(Victim* victim,
                                               uint32_t numEvaluationRuns)
{
  _statNumEvaluationRuns = numEvaluationRuns;

  if (_memHandle == nullptr)
  {
    return;
  }

  _statNumEvaluationMisses = testEvictionSet(victim, _statNumEvaluationRuns);
  _statNumEvaluationMissesWEvict =
      testEvictionSet(victim, _statNumEvaluationRuns, true);
  _statNumEvaluationMissesWFlush =
      testEvictionSet(victim, _statNumEvaluationRuns, true, true);

  _statNumEvaluationTruePositives = 0;
  _statNumEvaluationFalsePositives = 0;
  for (auto addr : _evictionSet)
  {
    if (victim->hasCollision(_memHandle, addr))
    {
      _statNumEvaluationTruePositives++;
    }
    else
    {
      _statNumEvaluationFalsePositives++;
    }
  }
}

ProfilingStatistic FilledCacheProfiling::getStatistics() const
{
  ProfilingStatistic stats;
  stats.numEvaluationMisses = _statNumEvaluationMisses;
  stats.numEvaluationMissesWEvict = _statNumEvaluationMissesWEvict;
  stats.numEvaluationMissesWFlush = _statNumEvaluationMissesWFlush;
  stats.numEvaluationFalsePositives = _statNumEvaluationFalsePositives;
  stats.numEvaluationTruePositives = _statNumEvaluationTruePositives;
  stats.numEvaluationRuns = _statNumEvaluationRuns;
  stats.numProfilingRuns = _statNumProfilingRuns;
  stats.attackMemorySize = _statAttackMemorySize;

  return stats;
}
