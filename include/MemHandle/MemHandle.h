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

#ifndef __MEMHANDLE_H__
#define __MEMHANDLE_H__ 1

#include <Cache/Cache.h>
#include <iostream>
#include <string>
#include <unordered_set>

#include <CLArgs.h>

class MemHandle
{
protected:
  std::unordered_set<tag_t> tagSet;

  std::unordered_set<tag_t>* evSet;

  MemHandle* collisionCompareMemHandle;

  uint64_t victimTargetAddress;

private:
  bool pub;
  const std::string name;
  address_t size;

  uint64_t attackerAddressesEvicted;

  uint64_t correctEvictions;
  uint64_t incorrectEvictions;

  AccessType accessType;

  void evictionCheck(const address_t offset,
                     const std::list<CacheResponse>& cacheRespList)
  {
    if (cacheRespList.empty())
      return;
    const CacheResponse cacheResp = cacheRespList.back();

    if (evSet && cacheResp._eviction)
    {
      if (evSet->find(cacheResp._evictedTag) != evSet->end())
      {
        if ((offset / CACHE_LINE_SIZE) ==
            (victimTargetAddress / CACHE_LINE_SIZE))
        {
          correctEvictions++;
        }
        else
        {
          incorrectEvictions++;
        }
        tagSet.insert(offset / CACHE_LINE_SIZE);
        attackerAddressesEvicted++;
      }
      // else if (offset == 0)
      //   incorrectEvictions++;
      // else if (offset != 0)
      //   correctEvictions++;

      // else {
      //   if (offset != 0)
      //     correctEvictions++;
      //   else
      //     incorrectEvictions++;
      // }
    }
  }

public:
  MemHandle(std::string name, address_t size, bool pub)
      : pub(pub), name(name), size(size), collisionCompareMemHandle(NULL),
        evSet(NULL), attackerAddressesEvicted(0), correctEvictions(0),
        incorrectEvictions(0), accessType(ACT_ALL), victimTargetAddress(0){};
  virtual ~MemHandle(){};

  bool isPublic() const { return pub; };
  const std::string& getName() { return name; };
  address_t getSize() const { return size; };
  virtual CacheContext getCacheContext() const
  {
    return DEFAULT_CACHE_CONTEXT;
  };

  int32_t read(address_t offset, std::list<CacheResponse>* resp = nullptr)
  {

    std::list<CacheResponse> temp;
    int32_t hit = 0;

    if (accessType == ACT_ALL ||
        (accessType == ACT_TARGET && (offset / CACHE_LINE_SIZE) == 0) ||
        (accessType == ACT_FIVE && (offset / CACHE_LINE_SIZE) < 5) ||
        (accessType == ACT_TEN && (offset / CACHE_LINE_SIZE) < 10) ||
        (accessType == ACT_FIFTEEN && (offset / CACHE_LINE_SIZE) < 15))
      hit = read(offset, temp);

    if (resp)
      *resp = temp;

    evictionCheck(offset, temp);

    return hit;
  };
  int32_t write(address_t offset)
  {

    std::list<CacheResponse> resp = std::list<CacheResponse>();

    int32_t hit = 0;

    if (accessType == ACT_ALL ||
        (accessType == ACT_TARGET && (offset / CACHE_LINE_SIZE) == 0) ||
        (accessType == ACT_FIVE && (offset / CACHE_LINE_SIZE) < 5) ||
        (accessType == ACT_TEN && (offset / CACHE_LINE_SIZE) < 10) ||
        (accessType == ACT_FIFTEEN && (offset / CACHE_LINE_SIZE) < 15))
      hit = write(offset, resp);

    evictionCheck(offset, resp);

    return hit;
  };
  int32_t exec(address_t offset)
  {
    std::list<CacheResponse> resp = std::list<CacheResponse>();
    int32_t hit = 0;

    if (accessType == ACT_ALL ||
        (accessType == ACT_TARGET && (offset / CACHE_LINE_SIZE) == 0) ||
        (accessType == ACT_FIVE && (offset / CACHE_LINE_SIZE) < 5) ||
        (accessType == ACT_TEN && (offset / CACHE_LINE_SIZE) < 10) ||
        (accessType == ACT_FIFTEEN && (offset / CACHE_LINE_SIZE) < 15))
      hit = exec(offset, resp);

    evictionCheck(offset, resp);

    return hit;
  };
  int32_t flush(address_t offset)
  {
    std::list<CacheResponse> resp = std::list<CacheResponse>();

    int32_t hit = 0;

    if (accessType == ACT_ALL ||
        (accessType == ACT_TARGET && (offset / CACHE_LINE_SIZE) == 0) ||
        (accessType == ACT_FIVE && (offset / CACHE_LINE_SIZE) < 5) ||
        (accessType == ACT_TEN && (offset / CACHE_LINE_SIZE) < 10) ||
        (accessType == ACT_FIFTEEN && (offset / CACHE_LINE_SIZE) < 15))
      hit = flush(offset, resp);

    evictionCheck(offset, resp);
    return hit;
  };

  void setComparatorMemHandle(MemHandle* const memhandle)
  {
    collisionCompareMemHandle = memhandle;
  }

  void setEvSet(std::unordered_set<tag_t>* evSet) { this->evSet = evSet; }

  uint64_t getAttackerAddressesEvicted() { return attackerAddressesEvicted; };
  void resetAttackerAddressesEvicted() { attackerAddressesEvicted = 0; };

  uint64_t getUniqueTags() { return tagSet.size(); };
  void clearUniqueTags() { tagSet.clear(); };

  uint64_t getCorrectEvictions() { return correctEvictions; };
  void clearCorrectEvictions() { correctEvictions = 0; };

  uint64_t getIncorrectEvictions() { return incorrectEvictions; };
  void clearIncorrectEvictions() { incorrectEvictions = 0; };

  void setAccessType(const AccessType accessType)
  {
    this->accessType = accessType;
  }

  void setVictimTargetAddress(const uint64_t targetAddress)
  {
    victimTargetAddress = targetAddress;
  }

  virtual int32_t read(address_t offset,
                       std::list<CacheResponse>& response) = 0;
  virtual int32_t write(address_t offset,
                        std::list<CacheResponse>& response) = 0;
  virtual int32_t exec(address_t offset,
                       std::list<CacheResponse>& response) = 0;
  virtual int32_t flush(address_t offset,
                        std::list<CacheResponse>& response) = 0;
  virtual address_t translate(address_t offset) const = 0;
  virtual int32_t hasCollision(address_t offset, const MemHandle* handle,
                               address_t handleOffset) = 0;
};

#endif // __MEMHANDLE_H__
