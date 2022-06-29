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

#include <CacheContext.h>

CacheContext::CacheContext() : _coreId(0) {}

CacheContext::CacheContext(int32_t coreId) : _coreId(coreId) {}

CacheContext::CacheContext(const CacheContext& ctx) : _coreId(ctx._coreId) {}

CacheContext::~CacheContext() {}

bool CacheContext::operator==(const CacheContext& context) const
{
  return this->_coreId == context._coreId;
}

bool CacheContext::operator!=(const CacheContext& context) const
{
  return !(*this == context);
}
