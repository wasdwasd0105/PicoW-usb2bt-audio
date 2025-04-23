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
SCALAR val)
{
    int id;
    IEEE754_FI fi;

    fi.f = val;
    id = ((fi.i & 0x7fffffff) >> 23) - 111;

    if (id < 0) {
        id = 0;
    }
    if (id > LDAC_NIDSF-1) {
        id = LDAC_NIDSF-1;
    }

    return id;
}

/***************************************************************************************************
    Normalize Spectrum
***************************************************************************************************/
static SCALAR sa_val_ldac[LDAC_MAXNSPS] = {
    -0.75, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
};

DECLFUNC void norm_spectrum_ldac(
AC *p_ac)
{
    int iqu, isp;
    int lsp, hsp;
    int nqus = p_ac->p_ab->nqus;
    int idsf;
    int *p_idsf = p_ac->a_idsf;
    SCALAR maxspec, tmp;
    SCALAR *p_spec = p_ac->p_acsub->a_spec;

    for (iqu = 0; iqu < nqus; iqu++) {
        lsp = ga_isp_ldac[iqu];
        hsp = ga_isp_ldac[iqu+1];

        maxspec = fabs(p_spec[lsp]);
        for (isp = lsp+1; isp < hsp; isp++) {
            tmp = fabs(p_spec[isp]);
            if (maxspec < tmp) {
                maxspec = tmp;
            }
        }
        idsf = get_scale_factor_id_ldac(maxspec);

        if (idsf > 0) {
            tmp = ga_isf_ldac[idsf];
            for (isp = lsp; isp < hsp; isp++) {
                p_spec[isp] *= tmp;
            }
        }
        else {
            for (isp = lsp; isp < hsp; isp++) {
                p_spec[isp] = sa_val_ldac[isp-lsp];
            }
        }

        p_idsf[iqu] = idsf;
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
    SCALAR qf = ga_qf_ldac[p_ac->a_idwl1[iqu]];
    SCALAR *p_nspec = p_ac->p_acsub->a_spec+isp;

    IEEE754_FI fi;
    const float fc = (float)((1 << 23) + (1 << 22));

    for (i = 0; i < nsps; i++) {
        fi.f = p_nspec[i] * qf + fc;
        p_qspec[i] = (short)fi.i;
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
    SCALAR ldqspec;
    SCALAR iqf = ga_iqf_ldac[LDAC_MAXIDWL1];
    SCALAR rqsf = ga_qf_ldac[p_ac->a_idwl2[iqu]] * ga_irsf_ldac[LDAC_MAXIDWL1]
            * _scalar(0.996093750);
    SCALAR *p_nspec = p_ac->p_acsub->a_spec+isp;

    IEEE754_FI fi;
    const float fc = (float)((1 << 23) + (1 << 22));

    for (i = 0; i < nsps; i++) {
        ldqspec = p_qspec[i] * iqf;
        fi.f = (p_nspec[i] - ldqspec) * rqsf + fc;
        p_rspec[i] = (short)fi.i;
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
    int *p_idwl2 = p_ac->a_idwl2;

    for (iqu = 0; iqu < nqus; iqu++) {
        if (p_idwl2[iqu] > 0) {
            quant_residual_core_ldac(p_ac, iqu);
        }
    }

    return;
}

