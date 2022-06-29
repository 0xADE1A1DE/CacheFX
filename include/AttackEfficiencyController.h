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

#ifndef __ATTACK_EFFICIENCY_CONTROLLER_H__
#define __ATTACK_EFFICIENCY_CONTROLLER_H__ 1

#include <Attacker/Attacker.h>
#include <Attacker/EvictionAttacker.h>
#include <CLArgs.h>
#include <MMU/DirectMMU.h>
#include <Victim/Victim.h>
#include <memory>
#include <utility>

class AttackEfficiencyController
{
private:
  std::unique_ptr<DirectMMU> mmu;
  std::unique_ptr<Victim> victim;
  std::unique_ptr<EvictionAttacker> attacker;

  CLArgs args;

public:
  AttackEfficiencyController(CLArgs args);

  void run(int32_t nattacks);
};

#endif // __ATTACK_EFFICIENCY_CONTROLLER_H__
