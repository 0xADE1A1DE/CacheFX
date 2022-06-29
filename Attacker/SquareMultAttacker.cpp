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

#include <string>
using namespace std;

#include <Attacker/SquareMultAttacker.h>
#include <MMU/MMU.h>
#include <MemHandle/MemHandle.h>
#include <Victim/Victim.h>
#include <types.h>

SquareMultAttacker::SquareMultAttacker(MMU* mmu, int32_t cacheSize,
                                       int32_t secbyte)
    : mmu(mmu), secbyte(secbyte), cacheSize(cacheSize)
{
  evictionSet = mmu->allocate("EvictionSet", cacheSize, false);
}

void SquareMultAttacker::setup()
{
  roundcount = 0;
  prime();
}

void SquareMultAttacker::prime()
{
  for (int32_t i = 0; i < cacheSize; i += CACHE_LINE_SIZE)
    evictionSet->read(i);
}

int32_t SquareMultAttacker::probe()
{
  int32_t result = 0;
  for (int32_t i = 0; i < cacheSize; i += CACHE_LINE_SIZE)
    if (evictionSet->read(cacheSize - CACHE_LINE_SIZE - i) == 0)
      result++;

  return result;
}

void SquareMultAttacker::round()
{
  int32_t ind = roundcount - secbyte * 8;
  int32_t pc = probe();
  if (ind >= 0 && ind < 8)
    probes[ind] = pc;

  roundcount++;
  prime();
}

void SquareMultAttacker::train(Victim* victim, const KeyPair KeyPair)
{
  int32_t keysize = victim->getKeySize();
  uint8_t key[keysize];

  for (int32_t i = 0; i < keysize; i++)
    key[i] = 0;
  victim->setKey(key);
  setup();
  victim->cipher(NULL, NULL);
  int32_t zerocount = 0;
  for (int32_t i = 0; i < 8; i++)
  {
    zerocount += probes[i];
  }

  for (int32_t i = 0; i < keysize; i++)
    key[i] = 0xFF;
  victim->setKey(key);
  setup();
  victim->cipher(NULL, NULL);
  int32_t onecount = 0;
  for (int32_t i = 0; i < 8; i++)
  {
    onecount += probes[i];
  }

  threshold = (zerocount + onecount + 7) / 16;
}

int32_t SquareMultAttacker::attack(const Victim* victim, const KeyPair KeyPair)
{
  setup();

  victim->cipher(NULL, NULL); // Should add buffers for input/output

  int32_t result = 0;
  for (int32_t i = 0; i < 8; i++)
  {
    result <<= 1;
    if (probes[i] > threshold)
      result |= 1;
  }

  return result;
}
