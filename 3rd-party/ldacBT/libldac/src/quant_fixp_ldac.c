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
    Subfunction: Get Scale Factor Index
***************************************************************************************************/
__inline static int get_scale_factor_id_ldac(
INT32 val)
{
    int i;
    int id, step;

    if (ga_sf_ldac[0] > val) {
        return 0;
    }

    id = LDAC_NIDSF >> 1;
    step = LDAC_NIDSF >> 2;
    for (i = 0; i < LDAC_IDSFBITS-1; i++) {
        if (ga_sf_ldac[id] > val) {
            id -= step;
        }
        else {
            id += step;
        }
        step >>= 1;
    }

    if ((ga_sf_ldac[id] <= val) && (id < LDAC_NIDSF-1)) {
        id++;
    }

    return id;
}

/***************************************************************************************************
    Normalize Spectrum
***************************************************************************************************/
static INT32 sa_val_ldac[LDAC_MAXNSPS] = { /* Q31 */
    0xa0000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

DECLFUNC void norm_spectrum_ldac(
AC *p_ac)
{
    int iqu, isp;
    int lsp, hsp;
    int nqus = p_ac->p_ab->nqus;
    int idsf;
    INT32 maxspec, tmp;
    INT32 *p_spec = p_ac->p_acsub->a_spec;

    for (iqu = 0; iqu < nqus; iqu++) {
        lsp = ga_isp_ldac[iqu];
        hsp = ga_isp_ldac[iqu+1];

        maxspec = abs(p_spec[lsp]);
        for (isp = lsp+1; isp < hsp; isp++) {
            tmp = abs(p_spec[isp]);
            if (maxspec < tmp) {
                maxspec = tmp;
            }
        }
        idsf = get_scale_factor_id_ldac(maxspec);

        if (idsf > 0) {
            for (isp = lsp; isp < hsp; isp++) {
                p_spec[isp] = sftrnd_ldac(p_spec[isp], idsf-LDAC_Q_NORM);
            }
        }
        else {
            for (isp = lsp; isp < hsp; isp++) {
                p_spec[isp] = sa_val_ldac[isp-lsp];
            }
        }

        p_ac->a_idsf[iqu] = idsf;
    }

    return;
}

/***************************************************************************************************
    Subfunction: Quantize Spectrum Core
***************************************************************************************************/
__inline static void quant_spectrum_core_ldac(
AC *p_ac,
int iqu)
{
    int i;
    int isp = ga_isp_ldac[iqu];
    int nsps = ga_nsps_ldac[iqu];
    int *p_qspec = p_ac->a_qspec+isp;
    INT32 qf = ga_qf_ldac[p_ac->a_idwl1[iqu]];
    INT32 *p_nspec = p_ac->p_acsub->a_spec+isp;

    for (i = 0; i < nsps; i++) {
        /* Q00 <- Q31 * Q16 */
        p_qspec[i] = mul_rsftrnd_ldac(p_nspec[i], qf, LDAC_Q_QUANT1);
    }

    return;
}

/***************************************************************************************************
    Quantize Spectrum
***************************************************************************************************/
DECLFUNC void quant_spectrum_ldac(
AC *p_ac)
{
    int iqu;
    int nqus = p_ac->p_ab->nqus;

    for (iqu = 0; iqu < nqus; iqu++) {
        quant_spectrum_core_ldac(p_ac, iqu);
    }

    return;
}

/***************************************************************************************************
    Subfunction: Quantize Residual Spectrum Core
***************************************************************************************************/
__inline static void quant_residual_core_ldac(
AC *p_ac,
int iqu)
{
    int i;
    int isp = ga_isp_ldac[iqu];
    int nsps = ga_nsps_ldac[iqu];
    int *p_qspec = p_ac->a_qspec+isp;
    int *p_rspec = p_ac->a_rspec+isp;
    INT32 ldqspec, rnspec;
    INT32 iqf = ga_iqf_ldac[LDAC_MAXIDWL1];
    INT32 rqf = ga_qf_ldac[p_ac->a_idwl2[iqu]];
    INT32 irsf = ga_irsf_ldac[LDAC_MAXIDWL1];
    INT32 *p_nspec = p_ac->p_acsub->a_spec+isp;

    for (i = 0; i < nsps; i++) {
        /* Q31 <- Q00 * Q31 */
        ldqspec = mul_lsftrnd_ldac(p_qspec[i], iqf, LDAC_Q_QUANT2);
        /* Q31 <- (Q31 - Q31) * Q15 */
        rnspec = mul_rsftrnd_ldac(p_nspec[i]-ldqspec, irsf, LDAC_Q_QUANT3);
        /* Q00 <- Q31 * Q16 */
        p_rspec[i] = mul_rsftrnd_ldac(rnspec, rqf, LDAC_Q_QUANT4);
    }

    return;
}

/***************************************************************************************************
    Quantize Residual Spectrum
***************************************************************************************************/
DECLFUNC void quant_residual_ldac(
AC *p_ac)
{
    int iqu;
    int nqus = p_ac->p_ab->nqus;
    int	*p_idwl2 = p_ac->a_idwl2;

    for (iqu = 0; iqu < nqus; iqu++) {
        if (p_idwl2[iqu] > 0) {
            quant_residual_core_ldac(p_ac, iqu);
        }
    }

    return;
}

