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

// Implementation of QARMA-64

#include <crypto/qarma64.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// #define DEBUG

// Rotation amounts

#define ROTATION_A 1
#define ROTATION_B 2
#define ROTATION_C 1

#define ROTATION_A_INV 1
#define ROTATION_B_INV 2
#define ROTATION_C_INV 1

#define ROTATION_A_MID 1
#define ROTATION_B_MID 2
#define ROTATION_C_MID 1

// In the internal state matrix the data is stored as follows
//
//  0  1  2  3
//  4  5  6  7
//  8  9 10 11
// 12 13 14 15

// Definition of the 4-bit Sbox(es)

const cell_t sigma_0[16] = {0, 14, 2, 10, 9,  15, 8, 11,
                            6, 4,  3, 7,  13, 12, 1, 5};
const cell_t sigma_0_inv[16] = {
    0, 14, 2, 10, 9,  15, 8, 11,
    6, 4,  3, 7,  13, 12, 1, 5}; // Sbox is an involution

const cell_t sigma_1[16] = {10, 13, 14, 6,  15, 7, 3, 5,
                            9,  8,  0,  12, 11, 1, 2, 4};
const cell_t sigma_1_inv[16] = {10, 13, 14, 6,  15, 7, 3, 5,
                                9,  8,  0,  12, 11, 1, 2, 4};

const cell_t sigma_2[16] = {11, 6, 8, 15, 12, 0, 9, 14,
                            3,  7, 4, 5,  13, 2, 1, 10};
const cell_t sigma_2_inv[16] = {5, 14, 13, 8, 10, 11, 1, 9,
                                2, 6,  15, 0, 4,  12, 7, 3};

const cell_t* sbox_4 = sigma_0;
const cell_t* sbox_4_inv = sigma_0_inv;

// ShuffleCells permutation (we use the INVERSE MIDORI permutation)

//                             0, 1, 2, 3,   4, 5, 6, 7,   8, 9,10,11,
//                             12,13,14,15
const int32_t SHUFFLE_P[16] = {0, 11, 6, 13, 10, 1, 12, 7,
                               5, 14, 3, 8,  15, 4, 9,  2};
const int32_t SHUFFLE_P_inv[16] = {0,  5,  15, 10, 13, 8, 2, 7,
                                   11, 14, 4,  1,  6,  3, 9, 12};
//                             0, 1, 2, 3,   4, 5, 6, 7,   8, 9,10,11,
//                             12,13,14,15

// INVERSE Tweak permutation

const int32_t TWEAKEY_P[16] = {6, 5,  14, 15, 0, 1, 2,  3,
                               7, 12, 13, 4,  8, 9, 10, 11};
const int32_t TWEAKEY_P_inv[16] = {4,  5,  6,  7,  11, 1,  0, 8,
                                   12, 13, 14, 15, 9,  10, 2, 3};

//// OMEGA4
//
// const cell_t nlfsr[16]      = { 8, 0, 9, 1, 2, 10, 11, 3, 4, 12, 13, 5, 6,
// 14, 15, 7 }; const cell_t nlfsr_inv[16]  = { 1, 3, 4, 7, 8, 11, 12, 15, 0, 2,
// 5, 6, 9, 10, 13, 14 };

// round constants

const cell_t RC[16][16] = {{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                            0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
                           {0x1, 0x3, 0x1, 0x9, 0x8, 0xA, 0x2, 0xE, 0x0, 0x3,
                            0x7, 0x0, 0x7, 0x3, 0x4, 0x4},
                           {0xA, 0x4, 0x0, 0x9, 0x3, 0x8, 0x2, 0x2, 0x2, 0x9,
                            0x9, 0xF, 0x3, 0x1, 0xD, 0x0},
                           {0x0, 0x8, 0x2, 0xE, 0xF, 0xA, 0x9, 0x8, 0xE, 0xC,
                            0x4, 0xE, 0x6, 0xC, 0x8, 0x9},
                           {0x4, 0x5, 0x2, 0x8, 0x2, 0x1, 0xE, 0x6, 0x3, 0x8,
                            0xD, 0x0, 0x1, 0x3, 0x7, 0x7},
                           {0xB, 0xE, 0x5, 0x4, 0x6, 0x6, 0xC, 0xF, 0x3, 0x4,
                            0xE, 0x9, 0x0, 0xC, 0x6, 0xC},
                           {0x3, 0xF, 0x8, 0x4, 0xD, 0x5, 0xB, 0x5, 0xB, 0x5,
                            0x4, 0x7, 0x0, 0x9, 0x1, 0x7},
                           {0x9, 0x2, 0x1, 0x6, 0xD, 0x5, 0xD, 0x9, 0x8, 0x9,
                            0x7, 0x9, 0xF, 0xB, 0x1, 0xB},
                           {0xD, 0x1, 0x3, 0x1, 0x0, 0xB, 0xA, 0x6, 0x9, 0x8,
                            0xD, 0xF, 0xB, 0x5, 0xA, 0xC},
                           {0x2, 0xF, 0xF, 0xD, 0x7, 0x2, 0xD, 0xB, 0xD, 0x0,
                            0x1, 0xA, 0xD, 0xF, 0xB, 0x7},
                           {0xB, 0x8, 0xE, 0x1, 0xA, 0xF, 0xE, 0xD, 0x6, 0xA,
                            0x2, 0x6, 0x7, 0xE, 0x9, 0x6},
                           {0xB, 0xA, 0x7, 0xC, 0x9, 0x0, 0x4, 0x5, 0xF, 0x1,
                            0x2, 0xC, 0x7, 0xF, 0x9, 0x9},
                           {0x2, 0x4, 0xA, 0x1, 0x9, 0x9, 0x4, 0x7, 0xB, 0x3,
                            0x9, 0x1, 0x6, 0xC, 0xF, 0x7},
                           {0x0, 0x8, 0x0, 0x1, 0xF, 0x2, 0xE, 0x2, 0x8, 0x5,
                            0x8, 0xE, 0xF, 0xC, 0x1, 0x6},
                           {0x6, 0x3, 0x6, 0x9, 0x2, 0x0, 0xD, 0x8, 0x7, 0x1,
                            0x5, 0x7, 0x4, 0xE, 0x6, 0x9},
                           {0xA, 0x4, 0x5, 0x8, 0xF, 0xE, 0xA, 0x3, 0xF, 0x4,
                            0x9, 0x3, 0x3, 0xD, 0x7, 0xE}};

const cell_t ALPHA[16] = {0xC, 0x0, 0xA, 0xC, 0x2, 0x9, 0xB, 0x7,
                          0xC, 0x9, 0x7, 0xC, 0x5, 0x0, 0xD, 0xD};

// XOR Cells to the state

void Add(cell_t state[16], const cell_t Cells[16])
{
  for (int32_t i = 0; i < 16; i++)
    state[i] ^= Cells[i];
}

// Apply a permutation

void apply_perm(cell_t cells[16], const int32_t permutation[16])
{
  int32_t i;
  cell_t tmp[16];

  // Apply the TWEAK permutation, store in temporary array

  for (i = 0; i < 16; i++)
    tmp[i] = cells[permutation[i]];

  // copy back, destructing input

  for (i = 0; i < 16; i++)
    cells[i] = tmp[i];
}

// Apply the tweak update, its inverse

// maps (b_3,b_2,b_1,b_0) to ( b_0 + b_1 ,b_3,b_2,b_1)

int32_t lfsr(int32_t v) { return (v >> 1) ^ (((v & 1) ^ ((v >> 1) & 1)) << 3); }

// maps  ( b_4 = b_0 + b_1 ,b_3,b_2,b_1) to (b_3,b_2,b_1,b_1 + b_4)

int32_t lfsr_inv(int32_t v)
{
  return ((v << 1) & 0xF) ^ ((v & 1) ^ ((v >> 3) & 1));
}

void updateTweak(cell_t cells[16])
{
  apply_perm(cells, TWEAKEY_P);

  cells[0] = lfsr(cells[0]);
  cells[1] = lfsr(cells[1]);
  cells[3] = lfsr(cells[3]);
  cells[4] = lfsr(cells[4]);
  cells[8] = lfsr(cells[8]);
  cells[11] = lfsr(cells[11]);
  cells[13] = lfsr(cells[13]);
}

void updateTweak_inv(cell_t cells[16])
{
  cells[0] = lfsr_inv(cells[0]);
  cells[1] = lfsr_inv(cells[1]);
  cells[3] = lfsr_inv(cells[3]);
  cells[4] = lfsr_inv(cells[4]);
  cells[8] = lfsr_inv(cells[8]);
  cells[11] = lfsr_inv(cells[11]);
  cells[13] = lfsr_inv(cells[13]);

  apply_perm(cells, TWEAKEY_P_inv);
}

// Apply the ShuffleCells function, its inverse

void ShuffleCells(cell_t cells[16]) { apply_perm(cells, SHUFFLE_P); }

void ShuffleCells_inv(cell_t cells[16]) { apply_perm(cells, SHUFFLE_P_inv); }

// Add a round constant

void AddConstants(cell_t cells[16], int32_t r)
{
  for (int32_t i = 0; i < 16; i++)
    cells[i] ^= RC[r][i];
}

// Apply alpha to cells state

void AddAlpha(cell_t cells[16])
{
  for (int32_t i = 0; i < 16; i++)
    cells[i] ^= ALPHA[i];
}

// Apply the 4-bit Sbox

void SubCell4(cell_t cells[16])
{
  for (int32_t i = 0; i < 16; i++)
    cells[i] = sbox_4[cells[i]];
}

// Apply the 4-bit inverse Sbox

void SubCell4_inv(cell_t cells[16])
{
  for (int32_t i = 0; i < 16; i++)
    cells[i] = sbox_4_inv[cells[i]];
}

// Apply the linear diffusion matrix of MIDORI (involution)
//
// M =
// 0 a b c  a,b,c means rotate by a,b,c
// c 0 a b
// b c 0 a
// a b c 0
//
// 0 a b c     s 0 s 1 s 2 s 3
// c 0 a b     s 4 s 5 s 6 s 7
// b c 0 a  X  s 8 s 9 s10 s11
// a b c 0     s12 s13 s14 s15

cell_t RotCell(cell_t val, int32_t amount)
{
  return (((val << amount) | (val >> (4 - amount))) & 0xF);
}

void MixColumns(cell_t state[16], int32_t a, int32_t b, int32_t c)
{
  int32_t j;
  cell_t temp0, temp1, temp2, temp3;

  for (j = 0; j < 4; j++) // for each column, that has 0,1,2,3 at the top
  {
    temp0 = RotCell(state[j + 4], a) ^ RotCell(state[j + 8], b) ^
            RotCell(state[j + 12], c);
    temp1 = RotCell(state[j], c) ^ RotCell(state[j + 8], a) ^
            RotCell(state[j + 12], b);
    temp2 = RotCell(state[j], b) ^ RotCell(state[j + 4], c) ^
            RotCell(state[j + 12], a);
    temp3 = RotCell(state[j], a) ^ RotCell(state[j + 4], b) ^
            RotCell(state[j + 8], c);

    state[j] = temp0;
    state[j + 4] = temp1;
    state[j + 8] = temp2;
    state[j + 12] = temp3;
  }
}

#define COMMA ,

#ifdef DEBUG

#define DEBUG_TRACE(_txt_)

#else

#define DEBUG_TRACE(_txt_)

#endif

void qarma64Encrypt(cell_t* input, const cell_t W[16], const cell_t W_p[16],
                    const cell_t core_key[16], const cell_t middle_key[16],
                    const cell_t tweak[16], int32_t R)
{
  cell_t state[16];
  cell_t T[16];
  cell_t K[16];

  int32_t i;

  for (i = 0; i < 16; i++)
  {
    state[i] = input[i];
    T[i] = tweak[i];
    K[i] = core_key[i];
  }

  DEBUG_TRACE("Q64 - initial state:                           ");

  // Initial whitening

  Add(state, W);
  DEBUG_TRACE("Q64 - after whitening with w:                  ");

  // The R forward rounds

  for (i = 0; i < R; i++)
  {
    // A round is Add(constant+key+tweak), then S-Box, MixColumns, and Shuffle

    AddConstants(state, i);
    Add(state, K);
    Add(state, T);
    DEBUG_TRACE("Q64 - round %.2i - after AddRoundTK:             " COMMA i);
    if (i != 0)
    {
      ShuffleCells(state);
      DEBUG_TRACE("Q64 - round %.2i - after ShuffleCells:           " COMMA i);
      MixColumns(state, ROTATION_A, ROTATION_B, ROTATION_C);
      DEBUG_TRACE("Q64 - round %.2i - after MixColumns:             " COMMA i);
    }
    SubCell4(state);
    DEBUG_TRACE("Q64 - round %.2i - after SubCell:                " COMMA i);
    updateTweak(T);
  }

  // The pseudo-reflector

  // first whitening key addition

  Add(state, W_p);
  Add(state, T);
  DEBUG_TRACE("Q64 - reflector - after AddRoundTK:            ");

  // full forward diffusion layer

  ShuffleCells(state);
  DEBUG_TRACE("Q64 - reflector - after ShuffleCells:          ");
  MixColumns(state, ROTATION_A, ROTATION_B, ROTATION_C);
  DEBUG_TRACE("Q64 - reflector - after MixColumns:            ");
  SubCell4(state);

  // bridge

  ShuffleCells(state);

  MixColumns(state, ROTATION_A_MID, ROTATION_B_MID, ROTATION_C_MID);
  Add(state, middle_key);

  ShuffleCells_inv(state);

  // backward round with whitening key in place of core key

  SubCell4_inv(state);
  DEBUG_TRACE("Q64 - reflector - after middle part:           ");

  MixColumns(state, ROTATION_A_INV, ROTATION_B_INV, ROTATION_C_INV);
  DEBUG_TRACE("Q64 - reflector - after MixColumns:            ");
  ShuffleCells_inv(state);
  DEBUG_TRACE("Q64 - reflector - after ShuffleCells:          ");

  // second whitening key addition

  Add(state, W);
  Add(state, T);
  DEBUG_TRACE("Q64 - reflector - after AddRoundTK:            ");

  // The R backward rounds

  for (i = 0; i < R; i++)
  {
    updateTweak_inv(T);

    SubCell4_inv(state);
    DEBUG_TRACE("Q64 - inverse round %.2i - after SubCell:        " COMMA R -
                1 - i);

    if (i != R - 1)
    {
      MixColumns(state, ROTATION_A_INV, ROTATION_B_INV, ROTATION_C_INV);
      DEBUG_TRACE("Q64 - inverse round %.2i - after MixColumns:     " COMMA R -
                  1 - i);
      ShuffleCells_inv(state);
      DEBUG_TRACE("Q64 - inverse round %.2i - after ShuffleCells:   " COMMA R -
                  1 - i);
    }
    Add(state, T);
    Add(state, K);
    AddAlpha(state); // inverse has + alpha
    AddConstants(state, R - 1 - i);
    DEBUG_TRACE("Q64 - inverse round %.2i - after AddRoundTK:     " COMMA R -
                1 - i);
  }

  // Final whitening with W_p

  Add(state, W_p);
  DEBUG_TRACE("Q64 - final state (after whitening with w'):   ");

  for (i = 0; i < 16; i++)
    input[i] = state[i] & 0x0F;
}

// in: core and whitening keys w0 and k0
// out: center core keys k1 and k1_M, second whitening key w1

void PRINCEExpansion(const cell_t w0[16], cell_t* w1)
{
  cell_t oldtemp;
  cell_t newtemp;
  int32_t i;

  // create w1 by expansion
  // first w0 >>> 1

  oldtemp = 0;
  for (i = 0; i < 16; i++)
  {
    newtemp = w0[i] & 0x1;
    w1[i] = (w0[i] >> 1) & 0xf;
    w1[i] = w1[i] ^ oldtemp;
    oldtemp = (newtemp << 3);
  }
  w1[0] ^= oldtemp;

  // then add w0 >> 63

  w1[15] ^= (w0[0] >> 3);
}

void W2Expansion(const cell_t w0[16], cell_t* w1)
{
  cell_t oldtemp;
  cell_t newtemp;
  int32_t i, c;

  // create w1 by expansion
  // first w0 >>> 1

  for (c = 0; c < 4; c++)
  {
    oldtemp = 0;
    for (i = 0; i < 4; i++)
    {
      newtemp = w0[i * 4 + c] & 0x1;
      w1[i * 4 + c] = (w0[i * 4 + c] >> 1) & 0xf;
      w1[i * 4 + c] = w1[i * 4 + c] ^ oldtemp;
      oldtemp = (newtemp << 3);
    }
    w1[c] ^= oldtemp;

    // then add w0 >> 15

    w1[c + 12] ^= (w0[c] >> 3);
  }
}

void qarma64KeySpecialisation(const cell_t k0[16], const cell_t w0[16],
                              cell_t* k1, cell_t* k1_M, cell_t* w1)
{
  int32_t i;

  PRINCEExpansion(w0, w1);
  // W2Expansion(w0, w1);

#if 0
	oldtemp = 0;
	for (i = 15; i>=0; i--)
	{
		newtemp = k0[i] & 0x8;
		k1[i] = (k0[i] << 1) & 0xf;
		k1[i] = k1[i] ^ oldtemp;
		oldtemp = (newtemp >> 3);
	}
	k1[15]  ^= oldtemp;
	// then add k0 << 63
	k1[0] ^= ((k0[15] << 3) & 0xF);
#else
  for (i = 0; i < 16; i++)
    k1[i] = k0[i];
#endif

  for (i = 0; i < 16; i++)
    k1_M[i] = k1[i];

  MixColumns(k1_M, ROTATION_A_MID, ROTATION_B_MID, ROTATION_C_MID);
}
