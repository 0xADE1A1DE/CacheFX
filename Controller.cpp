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

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

using namespace std;

#include <Attacker/Attacker.h>
#include <Cache/Cache.h>
#include <Controller.h>
#include <MMU/MMU.h>
#include <MemArray.h>
#include <Victim/Victim.h>
#include <types.h>

#include <Random.h>

#include <Attacker/EvictionAttacker.h>
#include <Attacker/OccupancyAttacker.h>
#include <Attacker/SquareMultAttacker.h>
#include <Cache/SetAssocCache.h>
#include <Victim/AESVictim.h>
#include <Victim/SquareMultVictim.h>

#define AESVICTIM
//#define SQUAREMULTVICTIM

#include <CacheFactory/CacheFactory.h>

/* Todo:
 * Parametrise creation of Cache, MMU, Attacker and Victim
 * Better random number generator
 * Generate full key
 */

static unique_ptr<Attacker> newAttacker(MMU* mmu, int32_t cacheSize,
                                        int32_t secretRound, CLArgs& args)
{
  switch (args.get_attacker())
  {
  case AT_OCCUPANCY:
    return make_unique<OccupancyAttacker>(mmu, cacheSize, secretRound, args);
  case AT_EVICTION:
    return make_unique<EvictionAttacker>(mmu, cacheSize, secretRound, args);
  }
  return NULL;
}

// Currently hardcoded.
Controller::Controller(CLArgs& args)
{
  std::srand(time(NULL));
  std::unique_ptr<Cache> cache(
      CacheFactory::Instance()->loadConfigurationFromFile());
  int32_t nlines = cache.get()->getNLines();
  mmu = std::unique_ptr<DirectMMU>(new DirectMMU(std::move(cache), true));

  int32_t modsize = 128;
  uint16_t mod[modsize];
  for (int32_t i = 0; i < modsize; i++)
    mod[i] = 0xffff - i;

  switch (args.get_victim())
  {
  case VT_AES:
    attacker = newAttacker(mmu.get(), nlines * CACHE_LINE_SIZE, -1, args);
    victim = std::unique_ptr<Victim>(new AESVictim(mmu.get()));
    break;
  case VT_SQUAREMULT:
    int32_t secbit = 7;
    attacker = newAttacker(mmu.get(), nlines * CACHE_LINE_SIZE, secbit, args);
    victim = std::unique_ptr<Victim>(
        new SquareMultVictim(mmu.get(), attacker.get(), 2, 128, mod, secbit));
    break;
  }
}

void Controller::run(int32_t nattacks)
{
  const auto keys = victim->genKeyPair();

  cout << "--attack: " << nattacks << "\n";
  attacker->zeroStats();
  if (!attacker->warmup(victim.get()))
    cout << "--Failed Warmup\n";
  cout << "warmup-accesses: " << attacker->getMemAccesses() << "\n";
  attacker->zeroStats();
  attacker->train(victim.get(), keys);
  cout << "--accesses: " << attacker->getMemAccesses() << "\n";
  cout << "--encryptions: " << attacker->getVictimCalls() << "\n";
};
