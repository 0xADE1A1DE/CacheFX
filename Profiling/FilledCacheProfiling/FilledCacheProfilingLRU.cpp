/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Feb 28, 2020
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

#include <Profiling/FilledCacheProfiling/FilledCacheProfilingLRU.h>

#include <cstdlib>
#include <iostream>
#include <vector>

#include <MMU/MMU.h>
#include <MemHandle/MemHandle.h>
#include <Profiling/FilledCacheProfiling/FilledCacheProfiling.h>
#include <Victim/Victim.h>
#include <types.h>

FilledCacheProfilingLRU::FilledCacheProfilingLRU(MMU* mmu, size_t cacheSize)
    : FilledCacheProfilingLRU(mmu, cacheSize, false)
{
}

FilledCacheProfilingLRU::FilledCacheProfilingLRU(MMU* mmu, size_t cacheSize,
                                                 bool plru)
    : FilledCacheProfiling(mmu, cacheSize), _plru(plru)
{
}

FilledCacheProfilingLRU::~FilledCacheProfilingLRU() {}

void FilledCacheProfilingLRU::selectCandidates(std::vector<char>& candidateSet)
{
  for (size_t i = 0; i < candidateSet.size(); i++)
  {
    candidateSet[i] = 1;
  }
}

void FilledCacheProfilingLRU::selectCandidatesPLRU(
    std::vector<char>& candidateSet)
{
  size_t cacheLines = _cacheSize / CACHE_LINE_SIZE;
  size_t selectionFactor = candidateSet.size() / cacheLines / 2;
  for (size_t i = 0; i < candidateSet.size(); i++)
  {
    candidateSet[i] = !(rand() % selectionFactor);
  }
}

int32_t
FilledCacheProfilingLRU::pruneCandidateSet(std::vector<char>& candidateSet,
                                           address_t startAddress)
{
  int32_t miss_observed;
  int32_t candidateCnt;

  do
  {
    // sc_flush_cache();

    primeCache(candidateSet, startAddress);

    miss_observed = 0;
    candidateCnt = 0;

    for (size_t i = 0; i < candidateSet.size(); i++)
    {
      size_t index = candidateSet.size() - 1 - i;
      size_t address = startAddress + index * CACHE_LINE_SIZE;
      if (candidateSet[index] == 1)
      {
        //        printf("Prune: Read: %ld\n", address);
        if (_memHandle->read(address) == 0)
        {
          candidateSet[index] = 0;
          miss_observed++;
        }
        else
        {
          candidateCnt++;
        }
      }
    }
    // std::cout << "misses: " << miss_observed << std::endl;
  } while (miss_observed && _plru);

  if (!_plru)
  {
    // std::cout << "CandidateCnt: " << candidateCnt << " PLRU: " << _plru << "
    // Misses: " << miss_observed << " (" << candidateSet.size() << ")" <<
    // std::endl;
    primeCache(candidateSet, startAddress);
    miss_observed = 0;
  }

  //  printf("Pruned Candidate Cnt: %d\n", candidateCnt);
  return candidateCnt;
}

void FilledCacheProfilingLRU::probeCache(std::vector<char> candidateSet,
                                         std::vector<int32_t>& candidateCount,
                                         address_t startAddress)
{
  for (size_t i = 0; i < candidateSet.size(); i++)
  {
    size_t index = candidateSet.size() - 1 - i;
    size_t address = startAddress + index * CACHE_LINE_SIZE;
    if (candidateSet[index])
    {
      if (_memHandle->read(address) == 0)
      {
        std::cout << "miss on " << address << std::endl;
        if (candidateCount[index] == 0)
        {
          _evictionSet.push_back(address);
        }
        candidateCount[index]++;
      }
    }
  }
}

void FilledCacheProfilingLRU::probeCacheFwd(
    std::vector<char> candidateSet, std::vector<int32_t>& candidateCount,
    address_t startAddress)
{
  for (size_t i = 0; i < candidateSet.size(); i++)
  {
    size_t index = i;
    size_t address = startAddress + index * CACHE_LINE_SIZE;
    if (candidateSet[index])
    {
      //        printf("ProbeFW: Read: %ld\n", address);
      if (_memHandle->read(address) == 0)
      {
        if (candidateCount[index] == 0)
        {
          _evictionSet.push_back(address);
        }
        candidateCount[index]++;
      }
    }
  }
}

void FilledCacheProfilingLRU::createEvictionSet(Victim* victim,
                                                uint32_t numAddresses,
                                                uint32_t maxIterations)
{
  size_t cacheLines = _cacheSize / CACHE_LINE_SIZE;
  size_t candidateMult = _plru ? 8 : 2;
  size_t numCandidates = candidateMult * cacheLines;
  std::vector<char> candidateSet(numCandidates, 0);
  std::vector<int32_t> candidateCount(numCandidates, 0);

  auto probeCacheFunc = &FilledCacheProfilingLRU::probeCache;
  auto selectCandidatesFunc = &FilledCacheProfilingLRU::selectCandidates;
  if (_plru)
  {
    probeCacheFunc = &FilledCacheProfilingLRU::probeCacheFwd;
    selectCandidatesFunc = &FilledCacheProfilingLRU::selectCandidatesPLRU;
  }

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

    (this->*selectCandidatesFunc)(candidateSet);

    // primeCache(candidateSet, startAddress);

    _statAvgCandidateCnt += pruneCandidateSet(candidateSet, startAddress);

    if (_plru)
      primeCache(candidateSet, startAddress);

    victim->accessAddress();

    //(this->*probeCacheFunc)(candidateSet, candidateCount, startAddress);
    //    printf("Prior probe: _evictionSet.size(): %ld\n",
    //    _evictionSet.size());
    probeCacheFwd(candidateSet, candidateCount, startAddress);
    //    printf("After probe: _evictionSet.size(): %ld\n",
    //    _evictionSet.size());

    flushCacheLines(_memHandle, startAddress, numCandidates);
  }
}
