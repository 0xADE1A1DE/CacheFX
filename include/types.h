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

#ifndef __TYPES_H__
#define __TYPES_H__ 1
#include <cstdint>

typedef uint64_t address_t;
typedef uint64_t tag_t;

static const tag_t TAG_NONE = ~0ULL;
static const tag_t TAG_INIT = ~0ULL - 1;
static const int32_t CACHE_LINE_SIZE = 64;

#endif //__TYPES_H__
