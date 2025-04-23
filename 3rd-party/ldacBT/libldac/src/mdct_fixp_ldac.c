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

/***************************************************************************************************
    Subfunction: Process MDCT Core
***************************************************************************************************/
static void proc_mdct_core_ldac(
INT32 *p_x,
INT32 *p_y,
int nlnn)
{
    INT32 i, j, k;
    INT32 loop1, loop2;
    INT32 coef, index0, index1, offset;
    int nsmpl = npow2_ldac(nlnn);
    int shift;
    const int *p_p;
    const INT32 *p_w, *p_c, *p_s;
    INT32 a_work[LDAC_MAXLSU];
    INT32 g0, g1, g2, g3;

    i = nlnn - LDAC_1FSLNN;
    p_w = gaa_fwin_ldac[i];
    p_c = gaa_wcos_ldac[i];
    p_s = gaa_wsin_ldac[i];
    p_p = gaa_perm_ldac[i];

    /* Block Floating */
    shift = LDAC_C_BLKFLT - get_bit_length_ldac(get_absmax_ldac(p_x, nsmpl<<1)) - 1;
    if (shift < 0) {
        shift = 0;
    }

    /* Windowing */
    if (LDAC_Q_MDCT_WIN-shift > 0){
        for (i = 0; i < nsmpl>>1; i++) {
            g0 = mul_rsftrnd_ldac(-p_x[3*nsmpl/2-1-i], p_w[nsmpl/2+i], LDAC_Q_MDCT_WIN-shift);
            g1 = mul_rsftrnd_ldac(-p_x[3*nsmpl/2+i], p_w[nsmpl/2-1-i], LDAC_Q_MDCT_WIN-shift);
            a_work[p_p[i]] = g0 + g1;

            g0 = mul_rsftrnd_ldac(p_x[i], p_w[i], LDAC_Q_MDCT_WIN-shift);
            g1 = mul_rsftrnd_ldac(-p_x[nsmpl-1-i], p_w[nsmpl-1-i], LDAC_Q_MDCT_WIN-shift);
            a_work[p_p[nsmpl/2+i]] = g0 + g1;
        }
    }
    else{
        for (i = 0; i < nsmpl>>1; i++) {
            g0 = mul_lsftrnd_ldac(-p_x[3*nsmpl/2-1-i], p_w[nsmpl/2+i], LDAC_Q_MDCT_WIN-shift);
            g1 = mul_lsftrnd_ldac(-p_x[3*nsmpl/2+i], p_w[nsmpl/2-1-i], LDAC_Q_MDCT_WIN-shift);
            a_work[p_p[i]] = g0 + g1;

            g0 = mul_lsftrnd_ldac(p_x[i], p_w[i], LDAC_Q_MDCT_WIN-shift);
            g1 = mul_lsftrnd_ldac(-p_x[nsmpl-1-i], p_w[nsmpl-1-i], LDAC_Q_MDCT_WIN-shift);
            a_work[p_p[nsmpl/2+i]] = g0 + g1;
        }
    }

    /* Butterfly */
    coef = 0;
    for (i = 0; i < nlnn-1; i++) {
        loop1 = 1 << (nlnn-2-i);
        loop2 = 1 << i;
        index0 = 0;
        index1 = 1 << (i+1);
        offset = 1 << (i+1);

        for (j = 0; j < loop1; j++) {
            for (k = 0; k < loop2; k++) {
                g0 = mul_rsftrnd_ldac(a_work[index1], p_c[coef], LDAC_Q_MDCT_COS+1);
                g1 = mul_rsftrnd_ldac(a_work[index1+1], p_s[coef], LDAC_Q_MDCT_SIN+1);
                g2 = g0 + g1;

                g0 = mul_rsftrnd_ldac(a_work[index1], p_s[coef], LDAC_Q_MDCT_SIN+1);
                g1 = mul_rsftrnd_ldac(a_work[index1+1], p_c[coef], LDAC_Q_MDCT_COS+1);
                g3 = g0 - g1;

                g0 = a_work[index0] >> 1;
                g1 = a_work[index0+1] >> 1;

                a_work[index0] = g0 + g2;
                a_work[index0+1] = g1 + g3;
                a_work[index1] = g0 - g2;
                a_work[index1+1] = g1 - g3;

                index0 += 2;
                index1 += 2;
                coef++;
            }
            index0 += offset;
            index1 += offset;
            coef -= loop2;
        }
        coef += loop2;
    }

    for (i = 0; i < nsmpl>>1; i++) {
        index0 = i << 1;

        g0 = mul_rsftrnd_ldac(a_work[index0], p_c[coef], LDAC_Q_MDCT_COS+shift);
        g1 = mul_rsftrnd_ldac(a_work[index0+1], p_s[coef], LDAC_Q_MDCT_SIN+shift);
        p_y[index0] = g0 + g1;

        g0 = mul_rsftrnd_ldac(a_work[index0], p_s[coef], LDAC_Q_MDCT_SIN+shift);
        g1 = mul_rsftrnd_ldac(a_work[index0+1], p_c[coef], LDAC_Q_MDCT_COS+shift);
        p_y[nsmpl-index0-1] = g0 - g1;

        coef++;
    }


    return;
}

/***************************************************************************************************
    Process MDCT
***************************************************************************************************/
DECLFUNC void proc_mdct_ldac(
SFINFO *p_sfinfo,
int nlnn)
{
    AC *p_ac;
    int ich;
    int nchs = p_sfinfo->cfg.ch;

    for (ich = 0; ich < nchs; ich++) {
        p_ac = p_sfinfo->ap_ac[ich];
        proc_mdct_core_ldac(p_ac->p_acsub->a_time, p_ac->p_acsub->a_spec, nlnn);
    }

    return;
}

