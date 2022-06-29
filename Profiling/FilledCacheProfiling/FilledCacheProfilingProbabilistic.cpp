/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Feb 5, 2020
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

#include <MMU/MMU.h>
#include <MemHandle/MemHandle.h>
#include <Profiling/FilledCacheProfiling/FilledCacheProfilingProbabilistic.h>

#include <cstdlib>
#include <iostream>
#include <limits>

FilledCacheProfilingProbabilistic::FilledCacheProfilingProbabilistic(
    MMU* mmu, size_t cacheSize, bool profiledThreshold)
    : FilledCacheProfiling(mmu, cacheSize), _pruningStopCondition(0.01),
      _numCandidateTestRuns(100), _profiledThreshold(profiledThreshold),
      _useFlush(false)
{
}

FilledCacheProfilingProbabilistic::~FilledCacheProfilingProbabilistic() {}

void FilledCacheProfilingProbabilistic::probeCache(
    std::vector<char> candidateSet, std::vector<int32_t>& candidateCount,
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
        candidateCount[index]++;
      }
    }
  }
}

int32_t FilledCacheProfilingProbabilistic::pruneCandidateSet(
    std::vector<char>& candidateSet, address_t startAddress)
{
  int32_t missObserved;
  int32_t candidateCnt;
  do
  {
    missObserved = 0;
    candidateCnt = 0;
    for (size_t i = 0; i < candidateSet.size(); i++)
    {
      size_t index = i;
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
  } while ((double)missObserved / (missObserved + candidateCnt) >
           _pruningStopCondition);
  return candidateCnt;
}

void FilledCacheProfilingProbabilistic::testCandidateCollisions(
    Victim* victim, std::vector<bool>& candidateCollisions,
    address_t startAddress)
{
  for (size_t i = 0; i < candidateCollisions.size(); i++)
  {
    size_t address = startAddress + i * CACHE_LINE_SIZE;
    candidateCollisions[i] = victim->hasCollision(_memHandle, address);
  }
}

void FilledCacheProfilingProbabilistic::testCandidateMisses(
    Victim* victim, std::vector<int32_t> candidateCount,
    std::vector<int32_t>& candidateMisses, address_t startAddress)
{
  for (size_t i = 0; i < candidateCount.size(); i++)
  {
    size_t address = startAddress + i * CACHE_LINE_SIZE;
    if (candidateCount[i] > 0)
    {
      candidateMisses[i] = 0;

      for (uint32_t run = 0; run < _numCandidateTestRuns; run++)
      {
        _memHandle->read(address);
        victim->accessAddress();
        if (_memHandle->read(address) == 0)
        {
          candidateMisses[i]++;
        }

        if (_useFlush)
        {
          _memHandle->flush(address);
          victim->invalidateAddress();
        }
        else
        {
          flushCacheRandAcc();
        }
      }
      // std::cout << "Testing candidate " << address << ": " <<
      // candidateMisses[i] << " misses." << std::endl;
    }
  }
}

double FilledCacheProfilingProbabilistic::getCandidateMissThresholdSimple(
    std::vector<int32_t> candidateCount, std::vector<int32_t> candidateMisses)
{
  double sumMisses = 0;
  int32_t numCandidates = 0;

  for (size_t i = 0; i < candidateCount.size(); i++)
  {
    if (candidateCount[i] > 0)
    {
      sumMisses += candidateMisses[i];
      numCandidates++;
    }
  }

  return (sumMisses / numCandidates);
}

double FilledCacheProfilingProbabilistic::getCandidateMissThresholdProfiled(
    std::vector<int32_t> candidateCount, std::vector<int32_t> candidateMisses,
    std::vector<bool> candidateCollisions)
{
  double sumMissesWCollision = 0;
  double sumMissesWOCollision = 0;
  int32_t numCandidatesWCollision = 0;
  int32_t numCandidatesWOCollision = 0;

  for (size_t i = 0; i < candidateCount.size(); i++)
  {
    if (candidateCount[i] > 0)
    {
      if (candidateCollisions[i])
      {
        sumMissesWCollision += candidateMisses[i];
        numCandidatesWCollision++;
      }
      else
      {
        sumMissesWOCollision += candidateMisses[i];
        numCandidatesWOCollision++;
      }
    }
  }

  double avgMissesWCollision =
      (numCandidatesWCollision > 0)
          ? (double)sumMissesWCollision / numCandidatesWCollision
          : std::numeric_limits<double>::max();
  double avgMissesWOCollision =
      (numCandidatesWOCollision > 0)
          ? (double)sumMissesWOCollision / numCandidatesWOCollision
          : 0;

  return (avgMissesWOCollision + avgMissesWCollision) / 2;
}

void FilledCacheProfilingProbabilistic::createEvictionSet(
    Victim* victim, uint32_t numAddresses, uint32_t maxIterations)
{
  size_t cacheLines = _cacheSize / CACHE_LINE_SIZE;
  size_t evSetCacheRatio = (size_t)(numAddresses / _cacheSize);
  size_t candidateMult = (evSetCacheRatio + 1) * 8;
  size_t numCandidates = candidateMult * cacheLines;
  std::vector<char> candidateSet(numCandidates, 0);
  std::vector<int32_t> candidateCount(numCandidates, 0);
  std::vector<int32_t> candidateMisses(numCandidates, 0);

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

  while (_statNumProfilingRuns < maxIterations)
  {
    _statNumProfilingRuns++;

    selectCandidates(candidateSet);

    primeCache(candidateSet, startAddress);

    _statAvgCandidateCnt += pruneCandidateSet(candidateSet, startAddress);

    victim->accessAddress();

    probeCache(candidateSet, candidateCount, startAddress);

    flushCacheLines(_memHandle, startAddress, cacheLines);
  }

  testCandidateMisses(victim, candidateCount, candidateMisses, startAddress);

  double threshold = 0;
  if (_profiledThreshold)
  {
    std::vector<bool> candidateCollisions(cacheLines, 0);
    testCandidateCollisions(victim, candidateCollisions, startAddress);
    threshold = getCandidateMissThresholdProfiled(
        candidateCount, candidateMisses, candidateCollisions);
  }
  else
  {
    threshold =
        getCandidateMissThresholdSimple(candidateCount, candidateMisses);
  }

  std::cout << "Threshold: " << threshold << std::endl;

  size_t index = 0;
  while (index < candidateCount.size() && _evictionSet.size() < numAddresses)
  {
    size_t address = startAddress + index * CACHE_LINE_SIZE;
    if (candidateCount[index] > 0)
    {
      if (candidateMisses[index] > threshold)
      {
        _evictionSet.push_back(address);
      }
    }
    index++;
  }
}
