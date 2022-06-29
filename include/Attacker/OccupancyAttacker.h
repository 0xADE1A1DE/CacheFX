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

#ifndef __OCCUPANCYATTACKER_H__
#define __OCCUPANCYATTACKER_H__ 1

#include <Attacker/Attacker.h>

class MMU;
class MemHandle;
class CLArgs;

class OccupancyAttacker : public Attacker
{
private:
  MMU* mmu;
  const int32_t cacheSize;
  MemHandle* evictionSet;

  virtual void prime();
  virtual int32_t probe();

  virtual bool trainPostlude(const int32_t i, int32_t a, double& aSum,
                             double& aSSum, int32_t b, double& bSum,
                             double& bSSum, int32_t& count,
                             const int32_t aLastProbe = 0,
                             const int32_t bLastProbe = 0);

public:
  OccupancyAttacker(MMU* mmu, int32_t cacheSize, int32_t secretRound,
                    CLArgs& args);
  virtual ~OccupancyAttacker(){};
  bool warmup();

  void access(){};
};

#endif // __OCCUPANCYATTACKER_H__
