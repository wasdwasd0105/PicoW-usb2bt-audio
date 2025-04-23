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

#include "ldac.h"


#define LDAC_Q_LOWENERGY    11
#define LDAC_Q_ADD_LOWENERGY 4

#define LDAC_TH_LOWENERGY_L (0x070bc28f>>LDAC_Q_ADD_LOWENERGY) /* Q15, _scalar(225.47)(Q19)>>4 */
#define LDAC_TH_LOWENERGY_M (0x1c0ce148>>LDAC_Q_ADD_LOWENERGY) /* Q15, _scalar(897.61)(Q19)>>4 */
#define LDAC_TH_LOWENERGY_H (0x6fab851f>>LDAC_Q_ADD_LOWENERGY) /* Q15, _scalar(3573.44)(Q19)>>4 */

#define LDAC_TH_CENTROID 0x00168000 /* Q15, _scalar(45.0) */

/***************************************************************************************************
    Lookup Table for Calculating Square Root Value
***************************************************************************************************/
static INT16 sa_sqrt_ldac[97] = { /* Q14 */
    0x2d41, 0x2df4, 0x2ea5, 0x2f54, 0x3000, 0x30a9, 0x3150, 0x31f5,
    0x3298, 0x3339, 0x33d8, 0x3475, 0x3510, 0x35aa, 0x3642, 0x36d8,
    0x376c, 0x3800, 0x3891, 0x3921, 0x39b0, 0x3a3d, 0x3ac9, 0x3b54,
    0x3bdd, 0x3c66, 0x3ced, 0x3d72, 0x3df7, 0x3e7b, 0x3efd, 0x3f7f,
    0x4000, 0x407f, 0x40fe, 0x417b, 0x41f8, 0x4273, 0x42ee, 0x4368,
    0x43e1, 0x445a, 0x44d1, 0x4548, 0x45be, 0x4633, 0x46a7, 0x471b,
    0x478d, 0x4800, 0x4871, 0x48e2, 0x4952, 0x49c1, 0x4a30, 0x4a9e,
    0x4b0b, 0x4b78, 0x4be5, 0x4c50, 0x4cbb, 0x4d26, 0x4d90, 0x4df9,
    0x4e62, 0x4eca, 0x4f32, 0x4f99, 0x5000, 0x5066, 0x50cb, 0x5130,
    0x5195, 0x51f9, 0x525d, 0x52c0, 0x5323, 0x5385, 0x53e7, 0x5449,
    0x54a9, 0x550a, 0x556a, 0x55ca, 0x5629, 0x5688, 0x56e6, 0x5745,
    0x57a2, 0x5800, 0x585c, 0x58b9, 0x5915, 0x5971, 0x59cc, 0x5a27,
    0x5a82,
};

/***************************************************************************************************
    Subfunction: Multiply
***************************************************************************************************/
__inline static INT32 mul_ldac(
INT32 in1,
INT32 in2)
{
    INT32 out;
    INT64 acc;

    /* Q30 <- Q30 * Q30 */
    acc = (INT64)in1 * in2;
    acc >>= 30;

    if (acc > LDAC_MAX_32BIT) {
        out = LDAC_MAX_32BIT;
    }
    else if (acc < LDAC_MIN_32BIT) {
        out = LDAC_MIN_32BIT;
    }
    else {
        out = (INT32)acc;
    }

    return out;
}

/***************************************************************************************************
    Subfunction: Subtract
***************************************************************************************************/
__inline static INT32 sub_ldac(
INT32 in1,
INT32 in2)
{
    INT32 out;

    out = in1 - in2;

    return out;
}

/***************************************************************************************************
    Subfunction: Add
***************************************************************************************************/
__inline static INT32 add_ldac(
INT32 in1,
INT32 in2)
{
    INT32 out;

    out = in1 + in2;

    return out;
}

/***************************************************************************************************
    Subfunction: Multiply and Add
***************************************************************************************************/
__inline static INT32 mad_ldac(
INT32 in1,
INT32 in2,
INT32 in3)
{
    INT32 out;

    out = mul_ldac(in2, in3);
    out = add_ldac(in1, out);

    return out;
}

/***************************************************************************************************
    Subfunction: Normalize
***************************************************************************************************/
__inline static INT16 norm_ldac(
UINT32 val)
{
    INT16 len;

    len = 0;
    while (val > 0) {
        val >>= 1;
        len++;
    }

    return len;
}

/***************************************************************************************************
    Subfunction: Calculate Exponential
***************************************************************************************************/
__inline static INT16 calc_exp_ldac(
INT32 in_h,
UINT32 in_l)
{
    INT16 e;

    if (in_h) {
        e = norm_ldac((UINT32)in_h) + 32;
    }
    else {
        e = norm_ldac(in_l);
    }
    e = (63 - e) & 0xfffe;

    return e;
}

/***************************************************************************************************
    Subfunction: Calculate Square Root
***************************************************************************************************/
__inline static INT32 calc_sqrt_ldac(
INT32 in,
INT16 e)
{
    INT16 i;
    INT32 val, dif, a;

    if (in <= 0) {
        return 0;
    }

    i = (INT16)(in >> 24);
    a = in & 0x00ffffffL;

    i = i - 32;
    val = sa_sqrt_ldac[i] << 16; /* Q30 <- Q14 << 16 */
    dif = sub_ldac(sa_sqrt_ldac[i+1]<<16, val); /* Q30 */
    a = (INT32)(((INT64)a << 30) >> 24); /* Q30 a = a / 0x1000000 */
    val = mad_ldac(val, dif, a);
    val = val >> (e >> 1);

    return val;
}

/***************************************************************************************************
    Calculate Pseudo Spectrum and Low Band Energy
***************************************************************************************************/
static INT32 calc_mdct_pseudo_spectrum_ldac(
INT32 *p_spec,
INT32 *p_psd,
UINT32 nsp)
{
    UINT32 isp;
    INT16 e;
    INT32 y0, y1, y2;
    INT32 tmp;
    INT64 low_energy;
    INT64 acc1, acc2;

    {
        y1 = p_spec[0];
        y2 = p_spec[1];
        acc1 = (INT64)y1 * (INT64)y1;
        acc2 = (INT64)y2 * (INT64)y2;
        acc1 = acc1 + acc2;
        low_energy = acc1 >> LDAC_Q_ADD_LOWENERGY; /* Q26 <- (Q15 * Q15) >> 4 */
        e = calc_exp_ldac((INT32)(acc1>>32), (UINT32)(acc1&0xffffffff));
        tmp = (INT32)((acc1 << e) >> 32);
        *p_psd++ = calc_sqrt_ldac(tmp, e);
    }

    for (isp = 1; isp < LDAC_NSP_LOWENERGY; isp++) {
        y0 = y1;
        y1 = y2;
        y2 = p_spec[isp+1];
        acc1 = (INT64)y1 * (INT64)y1;
        acc2 = (INT64)(y0-y2) * (INT64)(y0-y2);
        acc1 = acc1 + acc2;
        low_energy += acc1 >> LDAC_Q_ADD_LOWENERGY; /* Q26 <- (Q15 * Q15) >> 4 */
        e = calc_exp_ldac((INT32)(acc1>>32), (UINT32)(acc1&0xffffffff));
        tmp = (INT32)((acc1 << e) >> 32);
        *p_psd++ = calc_sqrt_ldac(tmp, e);
    }

    for (isp = LDAC_NSP_LOWENERGY; isp < nsp-1; isp++) {
        y0 = y1;
        y1 = y2;
        y2 = p_spec[isp+1];
        acc1 = (INT64)y1 * (INT64)y1;
        acc2 = (INT64)(y0-y2) * (INT64)(y0-y2);
        acc1 = acc1 + acc2;
        e = calc_exp_ldac((INT32)(acc1 >> 32), (UINT32)(acc1&0xffffffff));
        tmp = (INT32) ((acc1 << e) >> 32);
        *p_psd++ = calc_sqrt_ldac(tmp, e);
    }

    {
        acc1 = (INT64)y1 * (INT64)y1;
        acc2 = (INT64)y2 * (INT64)y2;
        acc1 = acc1 + acc2;
        e = calc_exp_ldac((INT32)(acc1 >> 32), (UINT32)(acc1&0xffffffff));
        tmp = (INT32)((acc1 << e) >> 32);
        *p_psd++ = calc_sqrt_ldac(tmp, e);
    }

    low_energy >>= LDAC_Q_LOWENERGY; /* Q15 <- Q26 >> 11 */
    if (low_energy > LDAC_MAX_32BIT) {
        low_energy = LDAC_MAX_32BIT;
    }

    return (INT32)low_energy;
}

/***************************************************************************************************
    Calculate Pseudo Spectrum Centroid
***************************************************************************************************/
static INT32 calc_spectral_centroid_ldac(
INT32 *p_spec,
UINT32 nsp)
{
    UINT32 isp;
    INT32 centroid = 0;
    INT64 s1, s2;

    s1 = s2 = 0;
    for (isp = 0; isp < nsp; isp++) {
        s1 += ((INT64)isp * (INT64)*p_spec); /* Q15 <- Q00 * Q15 */
        s2 += (INT64)*p_spec++; /* Q15 */
    }

    if (s2 != 0) {
        centroid = (INT32)((s1<<15) / s2); /* Q15 <- (Q15<<15) / Q15 */
    }

    return centroid;
}

/***************************************************************************************************
    Calculate Number of Zero Cross
***************************************************************************************************/
static UINT32 calc_zero_cross_number_ldac(
INT32 *p_time,
UINT32 n)
{
    UINT32 i;
    UINT32 zero_cross = 0;
    INT32 prev, tmp;

    prev = 0;
    for (i = 0; i < n; i++) {
        if ((prev == 0) || (*p_time == 0)) {
            tmp = 0;
        }
        else {
            tmp = prev ^ (*p_time);
        }

        if (tmp < 0) {
            zero_cross++;
        }
        prev = *p_time++;
    }

    return zero_cross;
}

/***************************************************************************************************
    Analyze Frame Status
***************************************************************************************************/
DECLSPEC int ana_frame_status_ldac(
SFINFO *p_sfinfo,
int nlnn)
{
    AC *p_ac;
    int ich;
    int nchs = p_sfinfo->cfg.ch;
    int nsmpl = npow2_ldac(nlnn+1);
    int cnt;
    int a_status[LDAC_PRCNCH];
    UINT32 zero_cross;
    INT32 low_energy, centroid;
    INT32 a_psd_spec[LDAC_NSP_PSEUDOANA];


    for (ich = 0; ich < nchs; ich++) {
        p_ac = p_sfinfo->ap_ac[ich];

        low_energy = calc_mdct_pseudo_spectrum_ldac(p_ac->p_acsub->a_spec, a_psd_spec, LDAC_NSP_PSEUDOANA);

        centroid = calc_spectral_centroid_ldac(a_psd_spec, LDAC_NSP_PSEUDOANA);

        zero_cross = calc_zero_cross_number_ldac(p_ac->p_acsub->a_time, nsmpl);

        a_status[ich] = LDAC_FRMSTAT_LEV_0;
        if (low_energy < LDAC_TH_LOWENERGY_L) {
            a_status[ich] = LDAC_FRMSTAT_LEV_3;
        }
        else {
            if (low_energy < LDAC_TH_LOWENERGY_M) {
                a_status[ich] = LDAC_FRMSTAT_LEV_2;
            }
            else if (low_energy < LDAC_TH_LOWENERGY_H) {
                a_status[ich] = LDAC_FRMSTAT_LEV_1;
            }

            cnt = p_ac->frmana_cnt;
            if ((centroid > LDAC_TH_CENTROID) && (zero_cross >= LDAC_TH_ZCROSNUM)) {
                cnt++;

                if (cnt >= LDAC_MAXCNT_FRMANA) {
                    cnt = LDAC_MAXCNT_FRMANA;
                    a_status[ich] = LDAC_FRMSTAT_LEV_2;
                }
                else if (a_status[ich] <= LDAC_FRMSTAT_LEV_1) {
                    a_status[ich]++;
                }
            }
            else {
                cnt = 0;
            }
            p_ac->frmana_cnt = cnt;
        }
    }

    if (nchs == LDAC_CHANNEL_1CH) {
        return a_status[0];
    } else {
        return min_ldac(a_status[0], a_status[1]);
    }
}

