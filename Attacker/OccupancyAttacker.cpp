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

#include <cmath>
#include <stdlib.h>
#include <string>

using namespace std;

#include <Attacker/OccupancyAttacker.h>
#include <CLArgs.h>
#include <MMU/MMU.h>
#include <MemHandle/MemHandle.h>
#include <Victim/Victim.h>
#include <types.h>

OccupancyAttacker::OccupancyAttacker(MMU* mmu, int32_t cacheSize,
                                     int32_t secretRound, CLArgs& args)
    : Attacker(secretRound, args.get_giveup()), mmu(mmu), cacheSize(cacheSize)
{
  evictionSet = mmu->allocate("EvictionSet", cacheSize, false);
}

void OccupancyAttacker::prime()
{
  for (int32_t i = 0; i < cacheSize; i += CACHE_LINE_SIZE)
  {
    read(evictionSet, i);
  }
}

int32_t OccupancyAttacker::probe()
{
  int32_t result = 0;
  for (int32_t i = CACHE_LINE_SIZE; i <= cacheSize; i += CACHE_LINE_SIZE)
  {
    if (read(evictionSet, cacheSize - i) == 0)
      result++;
  }

  return result;
}

bool OccupancyAttacker::warmup()
{
  int32_t min = cacheSize;
  int32_t count = 0;
  for (int32_t i = 0; i < cacheSize; i++)
  {
    int32_t p = probe();
    // printf("Populate: %3d: miss on %5d\n", i, p);
    if (p == 0)
      break;
    if (p >= min)
      count++;
    else
    {
      count = 0;
      min = p;
    }
    if (count == 30)
      break;
  }
  if (min > cacheSize / 3)
    return false;
  return true;
}

bool OccupancyAttacker::trainPostlude(const int32_t i, int32_t a, double& aSum,
                                      double& aSSum, int32_t b, double& bSum,
                                      double& bSSum, int32_t& count,
                                      const int32_t aLastProbe,
                                      const int32_t bLastProbe)
{
  if (i >= 0)
  {
    aSum += a;
    aSSum += a * a;

    bSum += b;
    bSSum += b * b;

    ABDiff += (a - b);

    if (setStats(i + 1, aSum, aSSum, bSum, bSSum))
    {
      printf("%d [%6.3f, %6.3f] [%6.3f, %6.3f]\n", i + 1, aavg - acint,
             aavg + acint, bavg - bcint, bavg + bcint);
      return true;
    }
  }
  return false;
}