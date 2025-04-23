/*
 * Copyright (C) 2003 - 2016 Sony Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _FIXP_LDAC_H
#define _FIXP_LDAC_H

/***************************************************************************************************
    Macro Definitions
***************************************************************************************************/

#define LDAC_MAX_32BIT ((INT32)0x7fffffffL)
#define LDAC_MIN_32BIT ((INT32)0x80000000L)

#define LDAC_C_BLKFLT   31

#define LDAC_Q_SETPCM   15

#define LDAC_Q_MDCT_WIN 30
#define LDAC_Q_MDCT_COS 31
#define LDAC_Q_MDCT_SIN 31

#define LDAC_Q_NORM1    15
#define LDAC_Q_NORM2    31

#define LDAC_Q_QUANT1   47
#define LDAC_Q_QUANT2    0
#define LDAC_Q_QUANT3   15
#define LDAC_Q_QUANT4   47

#define LDAC_Q_DEQUANT1  0
#define LDAC_Q_DEQUANT2  0
#define LDAC_Q_DEQUANT3 31

#define LDAC_Q_NORM (15+(LDAC_Q_NORM2-LDAC_Q_NORM1))

/***************************************************************************************************
    Function Declarations
***************************************************************************************************/
/* func_fixp_ldac.c */
DECLFUNC INT32 sftrnd_ldac(INT32, int);
#define lsft_ldac(x, n)    ((x) << (n))
#define rsft_ldac(x, n)    ((x) >> (n))
#define rsft_ro_ldac(x, n) (((x) + (1 << (n-1))) >> (n))

#define lsftrnd_ldac(x, n)        (INT32)((INT64)(x) << (-(n)))
#define rsftrnd_ldac(x, n)        (INT32)(((INT64)(x) + ((INT64)1 << ((n)-1))) >> (n))
#define mul_lsftrnd_ldac(x, y, n) (INT32)(((INT64)(x) * (INT64)(y)) << (-(n)))
#define mul_rsftrnd_ldac(x, y, n) (INT32)((((INT64)(x) * (INT64)(y)) + ((INT64)1 << ((n)-1))) >> (n))

DECLFUNC int get_bit_length_ldac(INT32);
DECLFUNC INT32 get_absmax_ldac(INT32 *, int);

#endif /* _FIXP_LDAC_H */

