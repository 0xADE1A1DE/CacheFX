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

#ifndef __AESVICTIM_H__
#define __AESVICTIM_H__ 1

#include <Victim/Victim.h>

class AESVictim : public Victim
{
private:
  uint32_t encKey[4 * 11];
  uint8_t secret;
  MemHandle* ttables;
  int32_t secbyte;

  void tabaccess(int32_t index0, int32_t index1, int32_t index2,
                 int32_t index3) const
  {
    tabaccess(index0, index1, index2, index3, false);
  };

  void tabaccess(int32_t index0, int32_t index1, int32_t index2, int32_t index3,
                 bool print) const
  {
    ttables->read(index0 * 4);
    ttables->read(index1 * 4 + 1024);
    ttables->read(index2 * 4 + 2048);
    ttables->read(index3 * 4 + 3072);
    // if (print)
    // printf("%02x:%02x:%02x:%02x ", index0, index1, index2, index3);
  };

public:
  AESVictim(MMU* mmu);
  AESVictim(MMU* mmu, CLArgs clargs);

  virtual int32_t getKeySize(void) const { return 16; };

  virtual int32_t getInputSize(void) const { return 16; };
  virtual int32_t getOutputSize(void) const { return 16; };

  virtual void setKey(const uint8_t* secret);
  virtual uint8_t getSecret() { return secret; };
  virtual void cipher(const uint8_t* input, uint8_t* output);

  int32_t accessAddress();
  void invalidateAddress() const;
  int32_t hasCollision(MemHandle* memHandle, address_t address) const;

  virtual uint64_t getUniqueVictimTags();
  virtual void clearUniqueVictimTags();

  virtual void setAttackerEvictionSet(unordered_set<tag_t>* evSet);
  virtual uint64_t getAttackerAddressesEvicted() const;
  virtual void resetAttackerAddressesEvicted();

  virtual uint64_t getCorrectEvictions();
  virtual void clearCorrectEvictions();

  virtual uint64_t getIncorrectEvictions();
  virtual void clearIncorrectEvictions();

  virtual ~AESVictim(){};
};

#endif // __AESVICTIM_H__
