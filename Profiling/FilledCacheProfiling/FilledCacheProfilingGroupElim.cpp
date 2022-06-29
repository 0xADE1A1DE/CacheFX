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

#include <Profiling/FilledCacheProfiling/FilledCacheProfilingGroupElim.h>

#include <cstdlib>
#include <iostream>
#include <vector>

#include <MMU/DirectMMU.h>
#include <MMU/MMU.h>
#include <MemHandle/MemHandle.h>
#include <Profiling/FilledCacheProfiling/FilledCacheProfiling.h>
#include <Victim/Victim.h>
#include <cmath>
#include <types.h>

#define DEBUG_GEM
//#define REPL_RANDOM_HANDLING

FilledCacheProfilingGroupElim::FilledCacheProfilingGroupElim(MMU* mmu,
                                                             Cache* cache)
    : FilledCacheProfilingGroupElim(mmu, cache, false)
{
}

FilledCacheProfilingGroupElim::FilledCacheProfilingGroupElim(MMU* mmu,
                                                             Cache* cache,
                                                             bool plru)
    : FilledCacheProfiling(mmu, cache->getNLines() * CACHE_LINE_SIZE),
      _plru(plru), _cache(cache)
{
}

FilledCacheProfilingGroupElim::~FilledCacheProfilingGroupElim() {}

size_t
FilledCacheProfilingGroupElim::selectCandidates(std::vector<char>& candidateSet)
{
  for (size_t i = 0; i < candidateSet.size(); i++)
  {
    candidateSet[i] = 1;
  }
  return candidateSet.size();
}

/* Selects ~1 out of 4 candidateSet lines; selected lines should be equal to 2 *
 * cachelines  */
size_t FilledCacheProfilingGroupElim::selectCandidatesPLRU(
    std::vector<char>& candidateSet)
{
  size_t cacheLines = _cacheSize / CACHE_LINE_SIZE;
  size_t selectionFactor = candidateSet.size() / cacheLines / 2;
  size_t candidateSetSelected = 0;

  for (size_t i = 0; i < candidateSet.size(); i++)
  {
    candidateSet[i] = !(rand() % selectionFactor);
    candidateSetSelected++;
  }

  return candidateSetSelected;
}

/* Assign selected candidate lines to <numOfGroups> groups */
void FilledCacheProfilingGroupElim::assignToGroups(
    std::vector<char>& candidateSet, size_t candidateSetSelected,
    std::vector<int32_t>& candidateSetGroupIndx, uint32_t numOfGroups)
{
  size_t candidateCnt = 0;
  size_t groupSize = candidateSetSelected / numOfGroups;

  /* Assign each selected candidate address to a group */
  for (size_t index = 0; index < candidateSet.size(); index++)
  {
    if (candidateSet[index] == 1)
    {
      candidateSetGroupIndx[index] =
          (candidateCnt / groupSize > numOfGroups - 1)
              ? (numOfGroups - 1)
              : (candidateCnt / groupSize); // If missaligned, last group will
                                            // contain leftover entries
      candidateCnt++;
    }
  }
}

/* Find next groupIndx and remove all lines that belong to that group */
size_t FilledCacheProfilingGroupElim::pruneCandidateSet(
    std::vector<char>& candidateSet,
    std::vector<int32_t>& candidateSetGroupIndx, size_t nextPruneIndx)
{
  bool foundGroupIndx = false;
  int32_t groupPruneIdx = -1;

  for (; nextPruneIndx < candidateSet.size(); nextPruneIndx++)
  {
    if (candidateSet[nextPruneIndx] == 1)
    {
      if (!foundGroupIndx)
      {
        groupPruneIdx = candidateSetGroupIndx[nextPruneIndx];
        candidateSet[nextPruneIndx] = 0;
        foundGroupIndx = true;
      }
      else if (candidateSetGroupIndx[nextPruneIndx] == groupPruneIdx)
      {
        candidateSet[nextPruneIndx] = 0;
      }
      else
        break;
    }
  }

  return nextPruneIndx;
}

void FilledCacheProfilingGroupElim::unpruneCandidateSet(
    std::vector<char>& candidateSet,
    std::vector<int32_t>& candidateSetGroupIndx, size_t pruneIndx)
{
  bool foundGroupIndx = false;
  int32_t groupPruneIdx = -1;

  for (; pruneIndx < candidateSet.size(); pruneIndx++)
  {
    if (candidateSetGroupIndx[pruneIndx] != -1)
    {
      if (!foundGroupIndx)
      {
        groupPruneIdx = candidateSetGroupIndx[pruneIndx];
        candidateSet[pruneIndx] = 1;
        foundGroupIndx = true;
      }
      else if (candidateSetGroupIndx[pruneIndx] == groupPruneIdx)
      {
        candidateSet[pruneIndx] = 1;
      }
      else
        break;
    }
  }
}

void FilledCacheProfilingGroupElim::cleanCandidateGroup(
    std::vector<char>& candidateSet,
    std::vector<int32_t>& candidateSetGroupIndx, size_t pruneIndx)
{
  bool foundGroupIndx = false;
  int32_t groupPruneIdx = -1;

  for (; pruneIndx < candidateSet.size(); pruneIndx++)
  {
    if (candidateSetGroupIndx[pruneIndx] != -1)
    {
      if (!foundGroupIndx)
      {
        groupPruneIdx = candidateSetGroupIndx[pruneIndx];
        candidateSetGroupIndx[pruneIndx] = -1;
        foundGroupIndx = true;
      }
      else if (candidateSetGroupIndx[pruneIndx] == groupPruneIdx)
      {
        candidateSetGroupIndx[pruneIndx] = -1;
      }
      else
        break;
    }
  }
}

void FilledCacheProfilingGroupElim::resetCandidateGroup(
    std::vector<int32_t>& candidateSetGroupIndx)
{
  for (size_t i = 0; i < candidateSetGroupIndx.size(); i++)
    candidateSetGroupIndx[i] = -1;
}

bool FilledCacheProfilingGroupElim::hasConflict(Victim* victim,
                                                std::vector<char>& candidateSet,
                                                address_t startAddress)
{
  bool isHit;

  victim->accessAddress();
  primeCache(candidateSet, startAddress);

  isHit = victim->accessAddress();

  flushCacheLines(_memHandle, startAddress, candidateSet.size());
  victim->invalidateAddress();

  return !isHit;
}

// void FilledCacheProfilingGroupElim::probeCache(std::vector<char>
// candidateSet, std::vector<int32_t> &candidateCount, address_t startAddress) {
//  for (size_t i = 0; i < candidateSet.size(); i++) {
//    size_t index = candidateSet.size()-1-i;
//    size_t address = startAddress + index*CACHE_LINE_SIZE;
//    if (candidateSet[index]) {
//      if (_memHandle->read(address) == 0) {
//        if (candidateCount[index] == 0) {
//          _evictionSet.push_back(address);
//        }
//        candidateCount[index]++;
//      }
//    }
//  }
//}

// void FilledCacheProfilingGroupElim::probeCacheFwd(std::vector<char>
// candidateSet, std::vector<int32_t> &candidateCount, address_t startAddress) {
//  for (size_t i = 0; i < candidateSet.size(); i++) {
//    size_t index = i;
//    size_t address = startAddress + index*CACHE_LINE_SIZE;
//    if (candidateSet[index]) {
//      if (_memHandle->read(address) == 0) {
//        if (candidateCount[index] == 0) {
//          _evictionSet.push_back(address);
//        }
//        candidateCount[index]++;
//      }
//    }
//  }
//}

double FilledCacheProfilingGroupElim::determineEvictionProbability(
    Victim* victim, std::vector<char>& candidateSet, address_t startAddress,
    int32_t totalTests)
{
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

void FilledCacheProfilingGroupElim::createEvictionSet(Victim* victim,
                                                      uint32_t numAddresses,
                                                      uint32_t maxIterations)
{
  size_t cacheLines = _cacheSize / CACHE_LINE_SIZE;
  size_t candidateMult = 8;
  size_t numCandidates = candidateMult * cacheLines;
  size_t activeCandidates = 0, activeCandidOfIter = 0;
  std::vector<char> candidateSet(numCandidates, 0);
  std::vector<int32_t> candidateSetGroupIndx(numCandidates, -1);
  size_t numOfGroups = ((DirectMMU*)_mmu)->getCache()->getEvictionSetSize();
  size_t groupSize = 0;
  uint32_t maxRetries = 0;
  uint32_t conflictSetTries = 0;
  const uint32_t maxConflictSetTries = 50;

  auto selectCandidatesFunc = &FilledCacheProfilingGroupElim::selectCandidates;
  if (_plru)
  {
    selectCandidatesFunc = &FilledCacheProfilingGroupElim::selectCandidatesPLRU;
  }

  _evictionSet.clear();

  if (_memHandle != nullptr)
  {
    _mmu->free(_memHandle);
  }
  _memHandle = _mmu->allocate("FCProfilerEvictionSetGroupElim",
                              candidateMult * _cacheSize, false);

  address_t startAddress = 0;

  _statAttackMemorySize = candidateMult * _cacheSize;
  _statNumProfilingRuns = 0;
  _statAvgCandidateCnt = 0;

  size_t pruneIndx = 0, nextPruneIndx = 0, groupIndx = 0;
  bool conflictSetFound = false;

  while (!conflictSetFound && conflictSetTries < maxConflictSetTries)
  {
    resetCandidateGroup(candidateSetGroupIndx);
    activeCandidates = (this->*selectCandidatesFunc)(candidateSet);

    if (!hasConflict(victim, candidateSet, startAddress))
    {
      if (!_plru)
      {
        _mmu->free(_memHandle);
        _memHandle = _mmu->allocate("FCProfilerEvictionSetGroupElim",
                                    candidateMult * _cacheSize, false);
      }
    }
    else
    {
      conflictSetFound = true;
    }
    conflictSetTries++;
  }

  if (conflictSetTries >= maxConflictSetTries)
  {
    printf("Did not find conflict set.\n");
    _statAvgCandidateCnt += 0;
    return;
  }

#ifdef REPL_RANDOM_HANDLING
  double initEvictionProbability = 0;
  if (!(_cache->getAlgorithm() == REPL_LRU ||
        _cache->getAlgorithm() == REPL_TREE_PLRU))
  {
    initEvictionProbability =
        determineEvictionProbability(victim, candidateSet, startAddress, 1000);
    maxRetries = 0;
  }
#endif

  double threshold = 0.99;

  numOfGroups = sqrt(activeCandidates); // Initialization of GEM group size

  uint32_t retryCnt = 0;
  while ((activeCandidates > numAddresses) &&
         ((numOfGroups < activeCandidates) // group size is larger than 1
          || !(activeCandidOfIter ==
               activeCandidates) // group was removed in last iteration
          || (retryCnt < maxRetries)))
  {
    _statNumProfilingRuns++;
    /* If no group was removed in the previous iteration */
    if (activeCandidOfIter == activeCandidates)
    {
      numOfGroups *= 2; // double the number of groups
#ifdef DEBUG_GEM
      std::cout << "Reduced threshold to " << threshold << std::endl;
#endif
      if (numOfGroups >= activeCandidates)
      {
        retryCnt++;
      }
    }

    activeCandidOfIter = activeCandidates;
    pruneIndx = 0;

    /* Find new group size */
    groupSize = activeCandidates / numOfGroups;

    if (groupSize < 1)
    {
      groupSize = 1;
      numOfGroups = activeCandidates;
    }

#ifdef DEBUG_GEM
    std::cout << "Trying group size " << groupSize << " (or: " << numOfGroups
              << " groups)" << std::endl;
#endif

    /* Assign candidate lines to groups */
    assignToGroups(candidateSet, activeCandidates, candidateSetGroupIndx,
                   numOfGroups);

#ifdef DEBUG_GEM
    int32_t cnt0 = 0;
    for (size_t i = 0; i < candidateSet.size(); i++)
    {
      if (candidateSet[i])
        cnt0++;
    }

    std::cout << "Starting with " << cnt0 << " candidates " << std::endl;
#endif

    for (groupIndx = 0; groupIndx < numOfGroups; groupIndx++)
    {
      /* Find next group to prune from Candidate Set */
      nextPruneIndx =
          pruneCandidateSet(candidateSet, candidateSetGroupIndx, pruneIndx);

      if (groupIndx != numOfGroups - 1)
        activeCandidates -= groupSize;
      else
        activeCandidates -= activeCandidOfIter - groupIndx * groupSize;

      bool removeGroup = true;
#ifdef REPL_RANDOM_HANDLING
      if (_cache->getAlgorithm() == REPL_LRU ||
          _cache->getAlgorithm() == REPL_TREE_PLRU)
      {
        if (!hasConflict(victim, candidateSet, startAddress))
        {
          removeGroup = false;
        }
      }
      else
      {
        double curEvictionProbability = determineEvictionProbability(
            victim, candidateSet, startAddress, 100);
        if (curEvictionProbability < threshold * initEvictionProbability)
        {
          removeGroup = false;
        }
      }
#else
      if (!hasConflict(victim, candidateSet, startAddress))
      {
        removeGroup = false;
      }
#endif

      if (!removeGroup)
      {
        unpruneCandidateSet(candidateSet, candidateSetGroupIndx,
                            pruneIndx); // Re-insert line in candidate set

        if (groupIndx != numOfGroups - 1)
          activeCandidates += groupSize;
        else
          activeCandidates += activeCandidOfIter - groupIndx * groupSize;
      }
      else
      {
        cleanCandidateGroup(candidateSet, candidateSetGroupIndx,
                            pruneIndx); // Reset groupIndx for pruned lines

#ifdef DEBUG_GEM
        int32_t cnt = 0;
        int32_t collisionCnt = 0;
        for (size_t i = 0; i < candidateSet.size(); i++)
        {
          if (candidateSet[i])
          {
            cnt++;
            if (victim->hasCollision(_memHandle,
                                     startAddress + i * CACHE_LINE_SIZE))
              collisionCnt++;
          }
        }
        std::cout << groupSize << " Removed line(s)... " << cnt << " ( "
                  << collisionCnt << " collisions) candidates left"
                  << std::endl;
#endif
      }

      pruneIndx = nextPruneIndx;
    }
#ifdef DEBUG_GEM
    std::cout << activeCandidates << " / " << numAddresses << " / "
              << numOfGroups << " / " << retryCnt << " / " << activeCandidOfIter
              << std::endl;
#endif
  }

  for (size_t indx = 0; indx < numCandidates; indx++)
  {
    if (candidateSet[indx])
    {
      _evictionSet.push_back(startAddress + indx * CACHE_LINE_SIZE);
    }
  }

#ifdef DEBUG_GEM
  std::cout << "Pushed " << _evictionSet.size() << " lines " << activeCandidates
            << std::endl;
  printf("Candidate Set Size: %ld, eviction set size: %ld\n", activeCandidates,
         _evictionSet.size());
#endif
}
