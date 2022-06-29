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

#include <PlaintextKeyPairGenerator/BinaryPlaintextKeyPairGenerator.h>
#include <PlaintextKeyPairGenerator/PlaintextKeyPairGenerator.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include <Random.h>

using namespace std;

BinaryPlaintextKeyPairGenerator::BinaryPlaintextKeyPairGenerator()
    : PlaintextKeyPairGenerator(1, 0)
{
  generateKey();
  plaintext.push_back(0);
}

void BinaryPlaintextKeyPairGenerator::generateKey()
{
  keyA.push_back(0);
  keyB.push_back(1);

  return;
}

void BinaryPlaintextKeyPairGenerator::generatePlaintext() {}

const std::vector<uint8_t>& BinaryPlaintextKeyPairGenerator::getPlaintext()
{
  return plaintext;
}