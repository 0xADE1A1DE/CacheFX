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

#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__ 1

#include <Attacker/Attacker.h>
#include <CLArgs.h>
#include <MMU/DirectMMU.h>
#include <Victim/Victim.h>
#include <memory>
#include <utility>

class Controller
{
private:
  std::unique_ptr<DirectMMU> mmu;
  std::unique_ptr<Victim> victim;
  std::unique_ptr<Attacker> attacker;

public:
  // Controller(VictimType victimType/* Specify everything */);
  Controller(CLArgs& args);

  void run(int32_t nattacks);
};

#endif // __CONTROLLER_H__
