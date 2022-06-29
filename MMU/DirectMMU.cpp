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

#include <iostream>
#include <map>
#include <string>

using namespace std;

#include <MMU/DirectMMU.h>
#include <MemHandle/DirectMemHandle.h>
#include <types.h>

DirectMMU::~DirectMMU()
{
  for (std::map<string, DirectMemHandle*>::iterator it = handles.begin();
       it != handles.end(); it++)
  {
    delete it->second;
  }
}

MemHandle* DirectMMU::allocate(string name, address_t size, address_t align,
                               address_t fix, CacheContext context, bool pub)
{
  int32_t count = handles.count(name);
  if (count != 0)
  {
    DirectMemHandle* dmh = handles[name];
    if (!pub || !dmh->isPublic())
      return NULL;
    while (dmh && dmh->context != context)
      dmh = dmh->next;
    if (dmh)
      return dmh;
    dmh = handles[name];
    DirectMemHandle* newdmh =
        new DirectMemHandle(this, name, dmh->base, size, context, true);
    newdmh->next = dmh;
    handles[name] = newdmh;
    return NULL;
  }
  address_t base = last + align - 1;
  base = (base / align) * align;
  base += fix;
  DirectMemHandle* dmh =
      new DirectMemHandle(this, name, base, size, context, pub);
  last = base + size;
  handles[name] = dmh;
  return dmh;
}

// Does not really work.
void DirectMMU::free(MemHandle* handle)
{
  string name = ((DirectMemHandle*)handle)->getName();
  DirectMemHandle* dmh = handles[name];

  if (dmh != nullptr)
  {
    handles.erase(name);
    delete handle;
  }
}
