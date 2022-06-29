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

#include <PlaintextKeyPairGenerator/AESPlaintextKeyPairGenerator.h>
#include <PlaintextKeyPairGenerator/PlaintextKeyPairGenerator.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include <Random.h>

using namespace std;

static uint8_t mix(uint8_t a, uint8_t b) { return (a & 0xf) | (b & 0xf0); }

AESPlaintextKeyPairGenerator::AESPlaintextKeyPairGenerator(
    const uint64_t keyLength, const uint64_t length)
    : PlaintextKeyPairGenerator(keyLength, length)
{
  generateKey();
}

void AESPlaintextKeyPairGenerator::generateKey()
{
  keyA.resize(keyLength);
  keyB.resize(keyLength);

  for (int32_t i = 0; i < 16; i++)
  {
    keyA[i] = Random::get()->rand();
    keyB[i] = Random::get()->rand();
  }
  keyB[0] = mix(keyB[0], keyA[0] ^ 0x10);
  keyB[4] = mix(keyB[4], keyA[4] ^ 0x20);
  keyB[8] = mix(keyB[8], keyA[8] ^ 0x30);
  keyB[12] = mix(keyB[12], keyA[12] ^ 0x40);

  return;
}

void AESPlaintextKeyPairGenerator::generatePlaintext()
{
  plaintext.clear();
  uint8_t byte = 0;
  transform(keyA.begin(), keyA.end(), back_inserter(plaintext),
            [&byte](const uint8_t key) {
              uint8_t randKey = Random::get()->rand();
              switch (byte)
              {
              case 0:
              case 4:
              case 8:
              case 12:
                randKey = mix(randKey, key);
                break;
              }
              byte++;
              return randKey;
            });
}

const std::vector<uint8_t>& AESPlaintextKeyPairGenerator::getPlaintext()
{
  generatePlaintext();
  return plaintext;
}