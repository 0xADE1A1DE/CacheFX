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

#ifndef PLAINTEXTKEYPAIRGENERATOR_H_
#define PLAINTEXTKEYPAIRGENERATOR_H_ 1

#include <random>
#include <utility>
#include <vector>

class PlaintextKeyPairGenerator
{
protected:
  std::vector<uint8_t> keyA;
  std::vector<uint8_t> keyB;

  std::vector<uint8_t> plaintext;

  uint64_t keyLength;
  uint64_t length;

  virtual void generateKey() = 0;
  virtual void generatePlaintext() = 0;

public:
  PlaintextKeyPairGenerator(const uint64_t keyLength, const uint64_t length)
      : keyLength(keyLength), length(length)
  {
  }
  const std::vector<uint8_t>& getKeyA() const { return keyA; }
  const std::vector<uint8_t>& getKeyB() const { return keyB; }

  virtual const std::vector<uint8_t>& getPlaintext() = 0;

  uint64_t getKeyLength() const { return keyLength; }
  uint64_t getLength() const { return length; }

  virtual ~PlaintextKeyPairGenerator(){};
};

#endif