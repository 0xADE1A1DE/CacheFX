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

#ifndef AESPLAINTEXTKEYPAIRGENERATOR_H_
#define AESPLAINTEXTKEYPAIRGENERATOR_H_ 1

#include <cstdint>
#include <vector>

#include <PlaintextKeyPairGenerator/PlaintextKeyPairGenerator.h>
#include <Random.h>

class AESPlaintextKeyPairGenerator : public PlaintextKeyPairGenerator
{
protected:
  void generateKey();
  void generatePlaintext();

public:
  AESPlaintextKeyPairGenerator(const uint64_t keyLength, const uint64_t length);
  virtual const std::vector<uint8_t>& getPlaintext();

  virtual ~AESPlaintextKeyPairGenerator(){};
};

#endif