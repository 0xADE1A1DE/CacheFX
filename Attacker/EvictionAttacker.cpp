/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
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
#include <cmath>
#include <set>
#include <stdlib.h>
#include <string>

#include <fstream>
using namespace std;

#include <Attacker/Attacker.h>
#include <Attacker/EvictionAttacker.h>
#include <CLArgs.h>
#include <MMU/MMU.h>
#include <MemHandle/MemHandle.h>
#include <Profiling/EvictionSetSizeProfiling.h>
#include <Victim/Victim.h>
#include <types.h>

EvictionAttacker::EvictionAttacker(MMU* mmu, int32_t cacheSize,
                                   int32_t secretRound, CLArgs& args,
                                   const double testNoiseSize,
                                   const NoiseType noiseType)
    : Attacker(secretRound, args.get_giveup()), mmu(mmu), cacheSize(cacheSize),
      evSet(NULL), testNoiseSize(testNoiseSize), noiseAccesses(0),
      probeType(args.get_probeType()), outputStats(nullptr),
      noiseType(noiseType), attackEfficacyType(args.get_attackEfficacyType()),
      alwaysNoise(args.get_alwaysNoise())
{
  bufSize = 10000 * cacheSize;
  evSetHandle = mmu->allocate("EvictionSet", bufSize, false);
  noiseptr = 0;
}

EvictionAttacker::~EvictionAttacker()
{
  mmu->free(evSetHandle);
  if (evSet != NULL)
    delete[] evSet;
}

void EvictionAttacker::prime()
{
  // Prime for evSize addresses, in case of NT_SAME,
  // the eviction set size is a ratio of the whole address range.
  uint32_t target = evSize;
  if (noiseType == NT_SAME)
    target = testNoiseSize * evSize;

  for (int32_t i = 0; i < target; i++)
    read(evSetHandle, evSet[i]);
}

int32_t EvictionAttacker::probe()
{
  // If the "probe" is done from the victim side (i.e. measuring
  // the actual number of attacker's line evicted, free from self-eviction)
  // then return as the result is ignored.
  if (probeType == PT_VICTIM)
    return 1;

  int32_t result = 0;
  int32_t selfEvictionCount = 0;

  int32_t target = 0;
  int32_t i = evSize - 1;


  // Divide the eviction set for probing and to evict stuck victim lines.
  if (noiseType == NT_SAME)
  {
    i = testNoiseSize * (evSize - 1);
    target = ((i - 256) > 0) ? (i - 256) : 0;
  }
  else if (noiseType == NT_PROBE_SIZE)
    target = evSize - 1 - testNoiseSize;

  for (; i >= target; i--)
  {
    list<CacheResponse> cacheResp;
    if (read(evSetHandle, evSet[i], &cacheResp) == 0)
    {
      if (i == evSize - 1)
        lastProbe = 1;
      result++;
    }
    else if (i == evSize - 1)
      lastProbe = 0;

    const tag_t evictedTag = cacheResp.back()._evictedTag;
    if (cacheResp.back()._eviction)
    {
      if ((evTagSet.find(evictedTag) != evTagSet.end()))
        selfEvictionCount++;
      else
        uniqueVictimLines++;
    }
  }
  selfEvictionRate += (double(selfEvictionCount) / double(evSize));
  selfEvictions += selfEvictionCount;

  return result;
}

bool EvictionAttacker::warmup(Victim* victim, const bool efficiencyTest,
                              const uint64_t targetEvictionSetSize,
                              const double targetEvictionSetEffectiveness,
                              uint64_t* const constructedEvictionSetSize,
                              double* const constructedEvictionSetProbability)
{
  EvictionSetSizeProfiling evSetProf(mmu, mmu->getCache());
  std::vector<size_t> evSetVector;

  victim->setAttackerEvictionSet(nullptr);

  evSetProf.createEvictionSet(victim, 0, 0, evSetHandle, evSetVector,
                              efficiencyTest, targetEvictionSetSize,
                              targetEvictionSetEffectiveness);

  evSize = evSetVector.size();
  evTagSet.clear();

  if (evSet != NULL)
    delete[] evSet;
  evSet = new size_t[evSize];

  int32_t i = 0;
  for (auto addr : evSetVector)
  {
    evTagSet.insert(evSetHandle->translate(addr) / CACHE_LINE_SIZE);
    evSet[i++] = addr;
  }
  victim->setAttackerEvictionSet(&evTagSet);

  if (constructedEvictionSetSize)
    *constructedEvictionSetSize = evSize;
  if (constructedEvictionSetProbability)
    *constructedEvictionSetProbability =
        evSetProf.testEvictionSet(victim, evSetHandle, evSetVector);

  return true;
}

bool EvictionAttacker::trainPostlude(const int32_t i, int32_t a, double& aSum,
                                     double& aSSum, int32_t b, double& bSum,
                                     double& bSSum, int32_t& count,
                                     const int32_t aLastProbe,
                                     const int32_t bLastProbe)
{
  if (probeType == PT_VICTIM)
  {
    a = areal;
    b = breal;
  }
  
  // If no probe was activated, we make some noise in the cache.
  // If the option alwaysNoise is activated, then a set of memory addresses
  // are accessed to dislodge stuck victim addresses
  // Otherwise, only do this when no probe is activated.
  if ((alwaysNoise || (a == 0) && (b == 0)) && noiseType == NT_SEPARATE)
  {
    int64_t target = cacheSize * testNoiseSize;
    if (attackEfficacyType == AET_HEATMAP)
      target = int64_t(testNoiseSize) * CACHE_LINE_SIZE;
    for (int32_t p = 0; p < target; p += CACHE_LINE_SIZE)
    {
      evSetHandle->read(noiseptr);
      noiseAccesses++;
      noiseptr += CACHE_LINE_SIZE;
      noiseptr %= bufSize;
    }
    prime();
  }
  
  // Use only last index to probe.
  if (probeType == PT_LAST)
  {
    a = aLastProbe;
    b = bLastProbe;
  }

  aSum += a;
  aSSum += a * a;

  bSum += b;
  bSSum += b * b;

  arealsum += areal;
  arealssum += areal * areal;

  brealsum += breal;
  brealssum += breal * breal;

  // Self eviction is defined as the probe that attacker reads
  // subtracted by what the victim actually evicts.
  const double aselfeviction = (a - areal);
  const double bselfeviction = (b - breal);

  aselfevictionsum += aselfeviction;
  aselfevictionssum += aselfeviction * aselfeviction;

  bselfevictionsum += bselfeviction;
  bselfevictionssum += bselfeviction * bselfeviction;

  count++;

  if (setStats(count, aSum, aSSum, bSum, bSSum, arealsum, arealssum, brealsum,
               brealssum, aselfevictionsum, aselfevictionssum, bselfevictionsum,
               bselfevictionssum))
  {
    printf("%d [%6.3f, %6.3f] [%6.3f, %6.3f] -- %d\n", count, aavg - acint,
           aavg + acint, bavg - bcint, bavg + bcint, i);
    if (i >= 0)
      success = true;
  }
  if (success || i == giveup - 1)
  {
    ABDiff = abs(aSum - bSum);
    if (outputStats)
    {
      (*outputStats) << victimCalls << "\t" << aavg << "\t" << asavg << "\t"
                     << asd << "\t" << bavg << "\t" << bsavg << "\t" << bsd
                     << "\t" << arealavg << "\t" << arealsavg << "\t" << arealsd
                     << "\t" << brealavg << "\t" << brealsavg << "\t" << brealsd
                     << "\t" << aselfevictionavg << "\t" << aselfevictionsavg
                     << "\t" << aselfevictionsd << "\t" << bselfevictionavg
                     << "\t" << bselfevictionsavg << "\t" << bselfevictionsd
                     << endl;
    }
  }
  return success;
}