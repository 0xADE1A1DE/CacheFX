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

#ifndef __CRYPTO_QARMA64_H__
#define __CRYPTO_QARMA64_H__


typedef unsigned char cell_t;

#ifdef __cplusplus
#include <cstdint>
extern "C"
{
#else
#include <stdint.h>
#endif
  void qarma64Encrypt(cell_t* input, const cell_t W[16], const cell_t W_p[16],
                      const cell_t core_key[16], const cell_t middle_key[16],
                      const cell_t tweak[16], int32_t R);

  void qarma64KeySpecialisation(const cell_t k0[16], const cell_t w0[16],
                                cell_t* k1, cell_t* k1_M, cell_t* w1);

#ifdef __cplusplus
}
#endif

#endif //__CRYPTO_QARMA64_H__