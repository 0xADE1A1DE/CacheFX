/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Aug 6, 2019
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

#ifndef CACHECONTEXT_H_
#define CACHECONTEXT_H_

#include <stdint.h>

class CacheContext
{
private:
  int32_t _coreId;

public:
  CacheContext();
  CacheContext(int32_t coreId);
  CacheContext(const CacheContext& ctx);
  virtual ~CacheContext();

  int32_t getCoreId() const { return _coreId; }
  void setCoreId(int32_t coreId) { _coreId = coreId; }

  bool operator==(const CacheContext& context) const;
  bool operator!=(const CacheContext& context) const;
};

class CacheContextCoreIdComparator
{
public:
  bool operator()(const CacheContext& lhs, const CacheContext& rhs) const
  {
    return lhs.getCoreId() < rhs.getCoreId();
  }
};

static const CacheContext DEFAULT_CONTEXT(0);

#endif /* CACHECONTEXT_H_ */
