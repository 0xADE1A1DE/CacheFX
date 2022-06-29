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

#include <Profiling/FilledCacheProfiling/FilledCacheProfilingSingleHold.h>

#include <cstdlib>
#include <iostream>
#include <vector>

#include <MMU/MMU.h>
#include <MemHandle/MemHandle.h>
#include <Profiling/FilledCacheProfiling/FilledCacheProfiling.h>
#include <Victim/Victim.h>
#include <cmath>
#include <types.h>

//#define REPL_RANDOM_HANDLING

FilledCacheProfilingSingleHold::FilledCacheProfilingSingleHold(MMU* mmu,
                                                               Cache* cache)
    : FilledCacheProfilingSingleHold(mmu, cache, false)
{
}

FilledCacheProfilingSingleHold::FilledCacheProfilingSingleHold(MMU* mmu,
                                                               Cache* cache,
                                                               bool plru)
    : FilledCacheProfiling(mmu, cache->getNLines() * CACHE_LINE_SIZE),
      _plru(plru), _cache(cache)
{
}

FilledCacheProfilingSingleHold::~FilledCacheProfilingSingleHold() {}

size_t FilledCacheProfilingSingleHold::selectCandidates(
    std::vector<char>& candidateSet)
{
  size_t selectedCandidates = 0;

  for (size_t i = 0; i < candidateSet.size(); i++)
  {
    candidateSet[i] = 1;
    selectedCandidates++;
  }
  return selectedCandidates;
}

/* Selects ~1 out of 4 candidateSet lines; selected lines should be equal to 2 *
 * cachelines  */
size_t FilledCacheProfilingSingleHold::selectCandidatesPLRU(
    std::vector<char>& candidateSet)
{
  size_t cacheLines = _cacheSize / CACHE_LINE_SIZE;
  size_t selectionFactor = candidateSet.size() / cacheLines / 2;
  size_t selectedCandidates = 0;

  for (size_t i = 0; i < candidateSet.size(); i++)
  {
    candidateSet[i] = !(rand() % selectionFactor);
    if (candidateSet[i])
      selectedCandidates++;
  }
  return selectedCandidates;
}

/* Pick an address from candidate set and prune */
size_t FilledCacheProfilingSingleHold::pruneCandidateSet(
    std::vector<char>& candidateSet, size_t nextPruneIndx)
{

  for (; nextPruneIndx < candidateSet.size(); nextPruneIndx++)
  {
    if (candidateSet[nextPruneIndx] == 1)
    {
      candidateSet[nextPruneIndx] = 0;
      break;
    }
  }

  return nextPruneIndx;
}

bool FilledCacheProfilingSingleHold::hasConflict(
    Victim* victim, std::vector<char>& candidateSet, address_t startAddress)
{
  bool isHit, tmp;

  tmp = victim->accessAddress();
  tmp = victim->accessAddress();

  primeCache(candidateSet, startAddress);

  isHit = victim->accessAddress();

  flushCacheLines(_memHandle, startAddress, candidateSet.size());
  victim->invalidateAddress();

  return !isHit;
}
// void FilledCacheProfilingSingleHold::probeCache(std::vector<char>
// candidateSet, address_t startAddress) {
//    for (size_t i = 0; i < candidateSet.size(); i++) {
//        size_t index = candidateSet.size()-1-i;
//        size_t address = startAddress + index*CACHE_LINE_SIZE;
//        if (candidateSet[index]) {
//            if (_memHandle->read(address) == 0) {
//                _evictionSet.push_back(address);
//            }
//        }
//    }
//}

// void FilledCacheProfilingSingleHold::probeCacheFwd(std::vector<char>
// candidateSet, address_t startAddress) {
//    for (size_t i = 0; i < candidateSet.size(); i++) {
//        size_t index = i;
//        size_t address = startAddress + index*CACHE_LINE_SIZE;
//        if (candidateSet[index]) {
//            if (_memHandle->read(address) == 0) {
//                _evictionSet.push_back(address);
//            }
//        }
//    }
//}

double FilledCacheProfilingSingleHold::determineEvictionProbability(
    Victim* victim, std::vector<char>& candidateSet, address_t startAddress,
    int32_t totalTests)
{
  double evictionProbability = 0;
  int32_t conflicts = 0;

  for (int32_t i = 0; i < totalTests; i++)
  {
    if (hasConflict(victim, candidateSet, startAddress))
    {
      conflicts++;
    }
  }

  return ((double)conflicts) / totalTests;
}

void FilledCacheProfilingSingleHold::createEvictionSet(Victim* victim,
                                                       uint32_t numAddresses,
                                                       uint32_t maxIterations)
{
  size_t cacheLines = _cacheSize / CACHE_LINE_SIZE;
  size_t candidateMult = 8;
  size_t numCandidates = candidateMult * cacheLines;
  size_t activeCandidates;
  size_t candidatesPrunedPrev;
  size_t candidatesPrunedCur;
  std::vector<char> candidateSet(numCandidates, 0);
  uint32_t maxRestarts = 0; // 2*cacheLines;
  uint32_t statRestartCnt = 0;
  uint32_t conflictSetTries = 0;
  const uint32_t maxConflictSetTries = 50;

  auto selectCandidatesFunc = &FilledCacheProfilingSingleHold::selectCandidates;
  if (_plru)
  {
    selectCandidatesFunc =
        &FilledCacheProfilingSingleHold::selectCandidatesPLRU;
  }

  _evictionSet.clear();

  if (_memHandle != nullptr)
  {
    _mmu->free(_memHandle);
  }
  _memHandle = _mmu->allocate("FCProfilerEvictionSetSingleHold",
                              candidateMult * _cacheSize, false);

  address_t startAddress = 0;

  _statAttackMemorySize = candidateMult * _cacheSize;
  _statNumProfilingRuns = 0;
  _statAvgCandidateCnt = 0;

  size_t pruneIndx = 0;

  statRestartCnt = 0;

  do
  {
    activeCandidates = (this->*selectCandidatesFunc)(candidateSet);
    conflictSetTries++;
  } while (!hasConflict(victim, candidateSet, startAddress) &&
           conflictSetTries < maxConflictSetTries);

  if (conflictSetTries >= maxConflictSetTries)
  {
    printf("Did not find conflict set.\n");
    _statAvgCandidateCnt += 0;
    return;
  }

#ifdef REPL_RANDOM_HANDLING
  double initEvictionProbability = 0;
  if (_cache->getAlgorithm() != REPL_LRU)
  {
    initEvictionProbability =
        determineEvictionProbability(victim, candidateSet, startAddress, 1000);
    maxRestarts = 10000;
  }
#endif

  candidatesPrunedPrev = 1;
  candidatesPrunedCur = 0;

  /* Loop through each line in candidateSet */
  while ((activeCandidates > numAddresses) &&
         (candidatesPrunedPrev > 0 || statRestartCnt < maxRestarts))
  {
    _statNumProfilingRuns++;
    /* Find next address to prune from Candidate Set */
    pruneIndx = pruneCandidateSet(candidateSet, pruneIndx);

    /* If we reached the end of candidate set, restart */
    if (pruneIndx == numCandidates)
    {
      pruneIndx = 0;
      candidatesPrunedPrev = candidatesPrunedCur;
      candidatesPrunedCur = 0;
      std::cout << "Next full iteration - " << activeCandidates
                << " candidates left " << std::endl;
      if (candidatesPrunedPrev == 0)
      {
        statRestartCnt++;
      }
      else
      {
        statRestartCnt = 0;
      }
      continue;
    }

    activeCandidates--;
    candidatesPrunedCur++;

    int32_t hasCollisionflag = victim->hasCollision(_memHandle, pruneIndx);

#ifdef REPL_RANDOM_HANDLING
    if (_cache->getAlgorithm() == REPL_LRU)
    {
      if (!hasConflict(victim, candidateSet, startAddress))
      {
        candidateSet[pruneIndx] = 1; // Re-insert line in candidate set
        activeCandidates++;
        candidatesPrunedCur--;
      }
    }
    else
    {
      double curEvictionProbability =
          determineEvictionProbability(victim, candidateSet, startAddress, 100);
      if (curEvictionProbability < 0.99 * initEvictionProbability)
      {
        candidateSet[pruneIndx] = 1;
        activeCandidates++;
        candidatesPrunedCur--;
      }
    }
#else
    if (!hasConflict(victim, candidateSet, startAddress))
    {
      candidateSet[pruneIndx] = 1; // Re-insert line in candidate set
      activeCandidates++;
      candidatesPrunedCur--;
    }
#endif

    pruneIndx++; // next index to be checked for pruning
  }

  for (size_t indx = 0; indx < numCandidates; indx++)
  {
    if (candidateSet[indx])
    {
      _evictionSet.push_back(startAddress + indx * CACHE_LINE_SIZE);
    }
  }

  printf("Retry runs: %d, Eviction set size: %ld\n", statRestartCnt,
         _evictionSet.size());
  _statAvgCandidateCnt += activeCandidates;
}
