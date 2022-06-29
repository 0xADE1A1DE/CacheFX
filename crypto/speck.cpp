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

#include <crypto/speck.h>

#define ROR64(x, r) (((x) >> (r)) | ((x) << (64 - (r))))
#define ROL64(x, r) (((x) << (r)) | ((x) >> (64 - (r))))
#define ROR32(x, r) (((x) >> (r)) | ((x) << (32 - (r))))
#define ROL32(x, r) (((x) << (r)) | ((x) >> (32 - (r))))
#define SPECK128R(x, y, k)                                                     \
  (x = ROR64(x, 8), x += y, x ^= k, y = ROL64(y, 3), y ^= x)
#define SPECK128RI(x, y, k)                                                    \
  (y ^= x, y = ROR64(y, 3), x ^= k, x -= y, x = ROL64(x, 8))
#define SPECK64R(x, y, k)                                                      \
  (x = ROR32(x, 8), x += y, x ^= k, y = ROL32(y, 3), y ^= x)
#define SPECK64RI(x, y, k)                                                     \
  (y ^= x, y = ROR32(y, 3), x ^= k, x -= y, x = ROL32(x, 8))

void speck128Encrypt(uint64_t* u, uint64_t* v, uint64_t key[])
{
  uint64_t i, x = *u, y = *v;

  for (i = 0; i < 32; i++)
    SPECK128R(x, y, key[i]);

  *u = x;
  *v = y;
}

void speck128Decrypt(uint64_t* u, uint64_t* v, uint64_t key[])
{
  int32_t i;
  uint64_t x = *u, y = *v;

  for (i = 31; i >= 0; i--)
    SPECK128RI(x, y, key[i]);

  *u = x;
  *v = y;
}

void speck128ExpandKey(uint64_t K[], uint64_t key[])
{
  uint64_t i, B = K[1], A = K[0];

  for (i = 0; i < 32; i++)
  {
    key[i] = A;
    SPECK128R(B, A, i);
  }
}

void speck64Encrypt(uint32_t* u, uint32_t* v, uint32_t key[])
{
  uint32_t i, x = *u, y = *v;

  for (i = 0; i < 27; i++)
    SPECK64R(x, y, key[i]);

  *u = x;
  *v = y;
}

void speck64Decrypt(uint32_t* u, uint32_t* v, uint32_t key[])
{
  int32_t i;
  uint32_t x = *u, y = *v;

  for (i = 26; i >= 0; i--)
    SPECK64RI(x, y, key[i]);

  *u = x;
  *v = y;
}

void speck64ExpandKey(uint32_t K[], uint32_t key[])
{
  // ASAN throws stack-buffer-overflow here
  // with argument ./cachefx -v SquareMult -m attacker -c
  // configs/cl256/w4/ceasers_2.xml similar problem for CEASER, phantom and
  // scatter
  uint32_t i, D = K[3], C = K[2], B = K[1], A = K[0];

  for (i = 0; i < 27; i += 3)
  {
    key[i] = A;
    SPECK64R(B, A, i);
    key[i + 1] = A;
    SPECK64R(C, A, i + 1);
    key[i + 2] = A;
    SPECK64R(D, A, i + 2);
  }
}
