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

#include <PlaintextKeyPairGenerator/PlaintextKeyPairGenerator.h>
#include <PlaintextKeyPairGenerator/SquareMultPlaintextKeyPairGenerator.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include <Random.h>

using namespace std;

SquareMultPlaintextKeyPairGenerator::SquareMultPlaintextKeyPairGenerator(
    const uint64_t PkeyLength, const uint64_t Plength, const uint8_t secBit)
    : PlaintextKeyPairGenerator(PkeyLength * 2, Plength * 2), secBit(secBit)
{
  // Generate Mask.
  keyMask.resize(keyLength);
  fill(keyMask.begin(), keyMask.end(), 0);
  keyMask[(keyLength * 8 - secBit) / 8] = 1 << ((keyLength * 8 - secBit) % 8);

  generateKey();
}

void SquareMultPlaintextKeyPairGenerator::generateKey()
{
  keyA.resize(keyLength);
  keyB.resize(keyLength);
  for (uint64_t i = 0; i < keyLength; i++)
  {
    keyA[i] = Random::get()->rand() & ~keyMask[i];
    keyB[i] = Random::get()->rand() | keyMask[i];
  }

  return;
}

void SquareMultPlaintextKeyPairGenerator::generatePlaintext()
{
  plaintext.clear();

  plaintext.resize(length);
  fill(plaintext.begin(), plaintext.end(), randomUint8());

  return;
}

const std::vector<uint8_t>& SquareMultPlaintextKeyPairGenerator::getPlaintext()
{
  generatePlaintext();
  return plaintext;
}