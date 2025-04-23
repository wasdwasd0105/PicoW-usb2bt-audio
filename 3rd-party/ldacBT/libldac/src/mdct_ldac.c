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
SCALAR *p_x,
SCALAR *p_y,
int nlnn)
{
    int i, j, k;
    int loop1, loop2;
    int coef, index0, index1, offset;
    int nsmpl = npow2_ldac(nlnn);
    const int *p_p;
    const SCALAR *p_w, *p_c, *p_s;
    SCALAR a_work[LDAC_MAXLSU];
    SCALAR *p_work = a_work;
    SCALAR a, b, c, d, tmp;
    SCALAR cc, cs;

    i = nlnn - LDAC_1FSLNN;
    p_w = gaa_fwin_ldac[i];
    p_c = gaa_wcos_ldac[i];
    p_s = gaa_wsin_ldac[i];
    p_p = gaa_perm_ldac[i];

    /* Windowing */
    for (i = 0; i < nsmpl>>1; i++) {
        p_work[p_p[i]] = -p_x[3*nsmpl/2-1-i] * p_w[nsmpl/2+i] - p_x[3*nsmpl/2+i] * p_w[nsmpl/2-1-i];

        p_work[p_p[nsmpl/2+i]] = p_x[i] * p_w[i] - p_x[nsmpl-1-i] * p_w[nsmpl-1-i];
    }

    /* Butterfly */
    coef = 0;
    for (i = 0; i < nlnn-1; ++i) {
        loop1 = 1 << (nlnn-2-i);
        loop2 = 1 << i;
        index0 = 0;
        index1 = 1 << (i+1);
        offset = 1 << (i+2);

        for (k = 0; k < loop2; ++k) {
            cc = p_c[coef];
            cs = p_s[coef++];
            for (j = 0; j < loop1; ++j) {
                a = p_work[index0+0];
                b = p_work[index0+1];
                c = p_work[index1+0] * cc + p_work[index1+1] * cs;
                d = p_work[index1+0] * cs - p_work[index1+1] * cc;

                p_work[index0+0] = a + c;
                p_work[index0+1] = b + d;
                p_work[index1+0] = a - c;
                p_work[index1+1] = b - d;
                index0 += offset;
                index1 += offset;
            }
            index0 += 2 - nsmpl;
            index1 += 2 - nsmpl;
        }
    }

    tmp = _scalar(1.0) / (SCALAR)(nsmpl>>1);
    for (i = 0; i < nsmpl>>1; i++) {
        cc = p_c[coef];
        cs = p_s[coef++];

        index0 = i << 1;
        a = p_work[index0] * cc + p_work[index0+1] * cs;
        b = p_work[index0] * cs - p_work[index0+1] * cc;

        p_y[index0] = a * tmp;
        p_y[nsmpl-index0-1] = b * tmp;
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

