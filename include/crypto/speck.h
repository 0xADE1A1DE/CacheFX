/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Feb 27, 2020
 *     Author: thomas
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

#ifndef CRYPTO_SPECK_H_
#define CRYPTO_SPECK_H_

#include <cstdint>

void speck128Encrypt(uint64_t* u, uint64_t* v, uint64_t* key);

void speck128Decrypt(uint64_t* u, uint64_t* v, uint64_t* key);

void speck128ExpandKey(uint64_t* K, uint64_t* key);

void speck64Encrypt(uint32_t* u, uint32_t* v, uint32_t* key);

void speck64Decrypt(uint32_t* u, uint32_t* v, uint32_t* key);

void speck64ExpandKey(uint32_t* K, uint32_t* key);

#endif /* CRYPTO_SPECK_H_ */
