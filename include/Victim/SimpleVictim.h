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

#ifndef __SIMPLEVICTIM_H__
#define __SIMPLEVICTIM_H__ 1

#include "victim.h"

class SimpleVictim : public Victim
{
private:
  uint8_t secret;
  uint8_t mask = 0xff;
  MMU* mmu;
  MemHandle* secretHandle;

public:
  int32_t getKeySize(void) const { return 1; };
  const uint8_t* getKeyMask(void) const { return &mask; };
  int32_t getInputSize(void) const { return 0; };
  int32_t getOutputSize(void) const { return 0; };

  SimpleVictim(MMU* mmu) : mmu(mmu), secret(0)
  {
    secretHandle = mmu->allocate("Secret", 256 * CACHE_LINE_SIZE,
                                 DEFAULT_CACHE_CONTEXT_VICTIM, false);
  };

  void setKey(uint8_t* secret) { this->secret = *secret; };

  uint8_t getSecret() { return secret; };

  void cipher(uint8_t* input, uint8_t* output) const
  {
    for (int32_t i = 0; i < secret; i++)
      secretHandle->read(i * CACHE_LINE_SIZE);
  };
};

#endif // __SIMPLEVICTIM_H__
