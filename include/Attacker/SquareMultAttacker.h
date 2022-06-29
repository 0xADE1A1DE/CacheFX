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

#ifndef __SQUAREMULTATTACKER_H__
#define __SQUAREMULTATTACKER_H__ 1

#include "Attacker.h"

class MMU;
class MemHandle;

class SquareMultAttacker : public Attacker
{
private:
  MMU* mmu;
  const int32_t cacheSize;
  MemHandle* evictionSet;
  int32_t secbyte;

  int32_t roundcount;
  int32_t probes[8];
  int32_t threshold;

  void setup();
  void prime();
  int32_t probe();

public:
  SquareMultAttacker(MMU* mmu, int32_t cacheSize, int32_t secbyte);
  void train(Victim* victim, const KeyPair keys);
  int32_t attack(const Victim* victim, const KeyPair keys);
  void round();
  void access(){};
};

#endif // __SQUAREMULTATTACKER_H__
