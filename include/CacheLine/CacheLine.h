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

#ifndef __CACHELINE_H__
#define __CACHELINE_H__ 1

class CacheLine
{
private:
  tag_t tag;

public:
  tag_t getTag(void);
  void setTag(tag_t line);
};

inline tag_t CacheLine::getTag(void) { return tag; }
inline void CacheLine::setTag(tag_t tag) { this->tag = tag; }

#endif //__CACHELINE_H__
