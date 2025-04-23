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
    Subfunction: Calculate Bits for Audio Block
***************************************************************************************************/
static int encode_audio_block_a_ldac(
AB *p_ab, 
int hqu)
{
    AC *p_ac;
    int ich, iqu;
    int nchs = p_ab->blk_nchs;
    int tmp, nbits = 0;
    int idsp, idwl1, idwl2;
    int grad_mode = p_ab->grad_mode;
    int grad_qu_l = p_ab->grad_qu_l;
    int grad_qu_h = p_ab->grad_qu_h;
    int grad_os_l = p_ab->grad_os_l;
    int grad_os_h = p_ab->grad_os_h;
    int *p_grad = p_ab->a_grad;
    int *p_idsf, *p_addwl, *p_idwl1, *p_idwl2;
    const unsigned char *p_t;

    /* Calculate Gradient Curve */
    tmp = grad_qu_h - grad_qu_l;

    for (iqu = 0; iqu < grad_qu_h; iqu++) {
        p_grad[iqu] = -grad_os_l;
    }
    for (iqu = grad_qu_h; iqu < hqu; iqu++) {
        p_grad[iqu] = -grad_os_h;
    }

    if (tmp > 0) {
        p_t = gaa_resamp_grad_ldac[tmp-1];

        tmp = grad_os_h - grad_os_l;
        if (tmp > 0) {
            tmp = tmp-1;
            for (iqu = grad_qu_l; iqu < grad_qu_h; iqu++) {
                p_grad[iqu] -= ((*p_t++ * tmp) >> 8) + 1;
            }
        }
        else if (tmp < 0) {
            tmp = -tmp-1;
            for (iqu = grad_qu_l; iqu < grad_qu_h; iqu++) {
                p_grad[iqu] += ((*p_t++ * tmp) >> 8) + 1;
            }
        }
    }

    /* Calculate Bits */
    for (ich = 0; ich < nchs; ich++) {
        p_ac = p_ab->ap_ac[ich];
	p_idsf = p_ac->a_idsf;
	p_addwl = p_ac->a_addwl;
	p_idwl1 = p_ac->a_idwl1;
	p_idwl2 = p_ac->a_idwl2;

        if (grad_mode == LDAC_MODE_0) { 
            for (iqu = 0; iqu < hqu; iqu++) {
                idwl1 = p_idsf[iqu] + p_grad[iqu];
                if (idwl1 < LDAC_MINIDWL1) {
                    idwl1 = LDAC_MINIDWL1;
                }
                idwl2 = 0;
                if (idwl1 > LDAC_MAXIDWL1) {
                    idwl2 = idwl1 - LDAC_MAXIDWL1;
                    if (idwl2 > LDAC_MAXIDWL2) {
                        idwl2 = LDAC_MAXIDWL2;
                    }
                    idwl1 = LDAC_MAXIDWL1;
                }
                p_idwl1[iqu] = idwl1;
                p_idwl2[iqu] = idwl2;
                idsp = ga_idsp_ldac[iqu];
                nbits += gaa_ndim_wls_ldac[idsp][idwl1] + ga_wl_ldac[idwl2] * ga_nsps_ldac[iqu];
            }
        }
        else if (grad_mode == LDAC_MODE_1) {
            for (iqu = 0; iqu < hqu; iqu++) {
                idwl1 = p_idsf[iqu] + p_grad[iqu] + p_addwl[iqu];
                if (idwl1 > 0) {
                    idwl1 = idwl1 >> 1;
                }
                if (idwl1 < LDAC_MINIDWL1) {
                    idwl1 = LDAC_MINIDWL1;
                }
                idwl2 = 0;
                if (idwl1 > LDAC_MAXIDWL1) {
                    idwl2 = idwl1 - LDAC_MAXIDWL1;
                    if (idwl2 > LDAC_MAXIDWL2) {
                        idwl2 = LDAC_MAXIDWL2;
                    }
                    idwl1 = LDAC_MAXIDWL1;
                }
                p_idwl1[iqu] = idwl1;
                p_idwl2[iqu] = idwl2;
                idsp = ga_idsp_ldac[iqu];
                nbits += gaa_ndim_wls_ldac[idsp][idwl1] + ga_wl_ldac[idwl2] * ga_nsps_ldac[iqu];
            }
        }
        else if (grad_mode == LDAC_MODE_2) {
            for (iqu = 0; iqu < hqu; iqu++) {
                idwl1 = p_idsf[iqu] + p_grad[iqu] + p_addwl[iqu];
                if (idwl1 > 0) {
                    idwl1 = (idwl1*3) >> 3;
                }
                if (idwl1 < LDAC_MINIDWL1) {
                    idwl1 = LDAC_MINIDWL1;
                }
                idwl2 = 0;
                if (idwl1 > LDAC_MAXIDWL1) {
                    idwl2 = idwl1 - LDAC_MAXIDWL1;
                    if (idwl2 > LDAC_MAXIDWL2) {
                        idwl2 = LDAC_MAXIDWL2;
                    }
                    idwl1 = LDAC_MAXIDWL1;
                }
                p_idwl1[iqu] = idwl1;
                p_idwl2[iqu] = idwl2;
                idsp = ga_idsp_ldac[iqu];
                nbits += gaa_ndim_wls_ldac[idsp][idwl1] + ga_wl_ldac[idwl2] * ga_nsps_ldac[iqu];
            }
        }
        else if (grad_mode == LDAC_MODE_3) {
            for (iqu = 0; iqu < hqu; iqu++) {
                idwl1 = p_idsf[iqu] + p_grad[iqu] + p_addwl[iqu];
                if (idwl1 > 0) {
                    idwl1 = idwl1 >> 2;
                }
                if (idwl1 < LDAC_MINIDWL1) {
                    idwl1 = LDAC_MINIDWL1;
                }
                idwl2 = 0;
                if (idwl1 > LDAC_MAXIDWL1) {
                    idwl2 = idwl1 - LDAC_MAXIDWL1;
                    if (idwl2 > LDAC_MAXIDWL2) {
                        idwl2 = LDAC_MAXIDWL2;
                    }
                    idwl1 = LDAC_MAXIDWL1;
                }
                p_idwl1[iqu] = idwl1;
                p_idwl2[iqu] = idwl2;
                idsp = ga_idsp_ldac[iqu];
                nbits += gaa_ndim_wls_ldac[idsp][idwl1] + ga_wl_ldac[idwl2] * ga_nsps_ldac[iqu];
            }
        }
    }

    return nbits;
}

/***************************************************************************************************
    Subfunction: Calculate Bits for Audio Block
***************************************************************************************************/
static int encode_audio_block_b_ldac(
AB *p_ab,
int nadjqus)
{
    AC *p_ac;
    int ich, iqu;
    int nchs = p_ab->blk_nchs;
    int nqus = min_ldac(LDAC_MAXNADJQUS, p_ab->nqus);
    int nbits = 0;
    int idsp, idwl1, idwl2;
    int *p_idwl1, *p_idwl2, *p_tmp;

    /* Calculate Bits */
    for (ich = 0; ich < nchs; ich++) {
        p_ac = p_ab->ap_ac[ich]; 
	p_idwl1 = p_ac->a_idwl1;
	p_idwl2 = p_ac->a_idwl2;
	p_tmp = p_ac->a_tmp;

        for (iqu = 0; iqu < nqus; iqu++) {
            idwl1 = p_tmp[iqu];
            if (iqu < nadjqus) {
                idwl1++;
            }
            idwl2 = 0;
            if (idwl1 > LDAC_MAXIDWL1) {
                idwl2 = idwl1 - LDAC_MAXIDWL1;
                if (idwl2 > LDAC_MAXIDWL2) {
                    idwl2 = LDAC_MAXIDWL2;
                }
                idwl1 = LDAC_MAXIDWL1;
            }
            p_idwl1[iqu] = idwl1;
            p_idwl2[iqu] = idwl2;
            idsp = ga_idsp_ldac[iqu];
            nbits += gaa_ndim_wls_ldac[idsp][idwl1] + ga_wl_ldac[idwl2] * ga_nsps_ldac[iqu];
        }
    }

    return nbits;
}

/***************************************************************************************************
    Subfunction: Decrease Lower Offset of Gradient Curve
***************************************************************************************************/
static int decrease_offset_low_ldac(
AB *p_ab,
int limit,
int *p_nbits_spec)
{
    int ncalls = 0;
    int nqus = p_ab->nqus;
    int grad_os_l = p_ab->grad_os_l;
    int nbits_avail = p_ab->nbits_avail;
    int step = limit - grad_os_l;
    int a_checked[LDAC_MAXGRADOS+1];

    if (*p_nbits_spec > nbits_avail) {
        memset(a_checked, 0, (LDAC_MAXGRADOS+1)*sizeof(int));

        while (grad_os_l < limit) {
            if (step > 1) {
                step = (step+1)/2;
            }

            if (*p_nbits_spec < nbits_avail) {
                grad_os_l -= step;
                if (grad_os_l < 0) {
                    grad_os_l += step;
                    break;
                }
                else if (a_checked[grad_os_l]) {
                    grad_os_l += step;
                    break;
                }
            }
            else if (*p_nbits_spec > nbits_avail) {
                grad_os_l += step;
                if (grad_os_l > LDAC_MAXGRADOS) {
                    grad_os_l -= step;
                    break;
                }
                else if (a_checked[grad_os_l]) {
                    grad_os_l -= step;
                    break;
                }
            }
            else {
                break;
            }

            p_ab->grad_os_l = grad_os_l;
            *p_nbits_spec = encode_audio_block_a_ldac(p_ab, nqus);
            a_checked[grad_os_l] = *p_nbits_spec;
            ncalls++;
        }

        while ((*p_nbits_spec > nbits_avail) && (grad_os_l < limit)) {
            p_ab->grad_os_l = ++grad_os_l;
            *p_nbits_spec = encode_audio_block_a_ldac(p_ab, nqus);
            ncalls++;
        }
    }

    return ncalls;
}

/***************************************************************************************************
    Subfunction: Decrease Higher Offset of Gradient Curve
***************************************************************************************************/
static int decrease_offset_high_ldac(
AB *p_ab,
int *p_nbits_spec)
{
    int ncalls = 0;
    int nqus = p_ab->nqus;
    int grad_os_h = p_ab->grad_os_h;
    int nbits_avail = p_ab->nbits_avail;
    int step = LDAC_MAXGRADOS - grad_os_h;
    int a_checked[LDAC_MAXGRADOS+1];

    if (*p_nbits_spec > nbits_avail) {
        memset(a_checked, 0, (LDAC_MAXGRADOS+1)*sizeof(int));

        while (grad_os_h < LDAC_MAXGRADOS) {
            if (step > 1) {
                step = (step+1)/2;
            }

            if (*p_nbits_spec < nbits_avail) {
                grad_os_h -= step;
                if (grad_os_h < 0) {
                    grad_os_h += step;
                    break;
                }
                else if (a_checked[grad_os_h]) {
                    grad_os_h += step;
                    break;
                }
            }
            else if (*p_nbits_spec > nbits_avail) {
                grad_os_h += step;
                if (grad_os_h > LDAC_MAXGRADOS) {
                    grad_os_h -= step;
                    break;
                }
                else if (a_checked[grad_os_h]) {
                    grad_os_h -= step;
                    break;
                }
            }
            else {
                break;
            }

            p_ab->grad_os_h = grad_os_h;
            *p_nbits_spec = encode_audio_block_a_ldac(p_ab, nqus);
            a_checked[grad_os_h] = *p_nbits_spec;
            ncalls++;
        }

        while ((*p_nbits_spec > nbits_avail) && (grad_os_h < LDAC_MAXGRADOS)) {
            p_ab->grad_os_h = ++grad_os_h;
            *p_nbits_spec = encode_audio_block_a_ldac(p_ab, nqus);
            ncalls++;
        }
    }

    return ncalls;
}

/***************************************************************************************************
    Subfunction: Increase Lower Offset of Gradient Curve
***************************************************************************************************/
static int increase_offset_low_ldac(
AB *p_ab,
int *p_nbits_spec)
{
    int ncalls = 0;
    int nqus = p_ab->nqus;
    int grad_os_l = p_ab->grad_os_l;
    int nbits_avail = p_ab->nbits_avail;
    int step = grad_os_l;
    int a_checked[LDAC_MAXGRADOS+1];

    memset(a_checked, 0, (LDAC_MAXGRADOS+1)*sizeof(int));

    while (grad_os_l > 0) {
        if (step > 1) {
            step = (step+1)/2;
        }

        if (*p_nbits_spec < nbits_avail) {
            grad_os_l -= step;
            if (grad_os_l < 0) {
                grad_os_l += step;
                break;
            }
            else if (a_checked[grad_os_l]) {
                grad_os_l += step;
                break;
            }
        }
        else if (*p_nbits_spec > nbits_avail) {
            grad_os_l += step;
            if (grad_os_l > LDAC_MAXGRADOS) {
                grad_os_l -= step;
                break;
            }
            else if (a_checked[grad_os_l]) {
                grad_os_l -= step;
                break;
            }
        }
        else {
            break;
        }

        p_ab->grad_os_l = grad_os_l;
        *p_nbits_spec = encode_audio_block_a_ldac(p_ab, nqus);
        a_checked[grad_os_l] = *p_nbits_spec;
        ncalls++;
    }

    while ((*p_nbits_spec > nbits_avail) && (grad_os_l < LDAC_MAXGRADOS)) {
        p_ab->grad_os_l = ++grad_os_l;
        *p_nbits_spec = encode_audio_block_a_ldac(p_ab, nqus);
        ncalls++;
    }

    return ncalls;
}


/***************************************************************************************************
    Subfunction: Increase Lower QU of Gradient Curve
***************************************************************************************************/
static int increase_qu_low_ldac(
AB *p_ab,
int *p_nbits_spec)
{
    int ncalls = 0;
    int nqus = p_ab->nqus;
    int grad_qu_l = p_ab->grad_qu_l;
    int grad_qu_h = p_ab->grad_qu_h;
    int nbits_avail = p_ab->nbits_avail;
    int step = grad_qu_h - grad_qu_l;
    int a_checked[LDAC_DEFGRADQUH+1];

    memset(a_checked, 0, (LDAC_DEFGRADQUH+1)*sizeof(int));

    while ((grad_qu_l > 0) && (grad_qu_l < LDAC_DEFGRADQUH)) {
        if (step > 1) {
            step = (step+1)/2;
        }

        if (*p_nbits_spec < nbits_avail) {
            grad_qu_l += step;
            if (grad_qu_l > LDAC_DEFGRADQUH) {
                grad_qu_l -= step;
                break;
            }
            else if (a_checked[grad_qu_l]) {
                grad_qu_l -= step;
                break;
            }
        }
        else if (*p_nbits_spec > nbits_avail) {
            grad_qu_l -= step;
            if (grad_qu_l < 0) {
                grad_qu_l += step;
                break;
            }
            else if (a_checked[grad_qu_l]) {
                grad_qu_l += step;
                break;
            }
        }
        else {
            break;
        }

        p_ab->grad_qu_l = grad_qu_l;
        *p_nbits_spec = encode_audio_block_a_ldac(p_ab, nqus);
        a_checked[grad_qu_l] = *p_nbits_spec;
        ncalls++;
    }

    while ((*p_nbits_spec > nbits_avail) && (grad_qu_l <= LDAC_DEFGRADQUH)) {
        p_ab->grad_qu_l = --grad_qu_l;
        *p_nbits_spec = encode_audio_block_a_ldac(p_ab, nqus);
        ncalls++;
    }

    return ncalls;
}

/***************************************************************************************************
    Subfunction: Increase Lower QU of Gradient Curve
***************************************************************************************************/
static int increase_qu_low_0_ldac(
AB *p_ab,
int *p_nbits_spec)
{
    int ncalls = 0;
    int nqus = p_ab->nqus;
    int grad_qu_l = p_ab->grad_qu_l;
    int grad_qu_h = p_ab->grad_qu_h;
    int nbits_avail = p_ab->nbits_avail;
    int step = grad_qu_h - grad_qu_l;
    int a_checked[LDAC_MAXGRADQU+1];

    memset(a_checked, 0, (LDAC_MAXGRADQU+1)*sizeof(int));

    while ((grad_qu_l > 0) && (grad_qu_l < grad_qu_h)) {
        if (step > 1) {
            step = step/2;
        }

        if (*p_nbits_spec < nbits_avail) {
            grad_qu_l += step;
            if (grad_qu_l >= grad_qu_h) {
                grad_qu_l -= step;
                break;
            }
            else if (a_checked[grad_qu_l]) {
                grad_qu_l -= step;
                break;
            }
        }
        else if (*p_nbits_spec > nbits_avail) {
            grad_qu_l -= step;
            if (grad_qu_l < 0) {
                grad_qu_l += step;
                break;
            }
            else if (a_checked[grad_qu_l]) {
                grad_qu_l += step;
                break;
            }
        }
        else {
            break;
        }

        p_ab->grad_qu_l = grad_qu_l;
        *p_nbits_spec = encode_audio_block_a_ldac(p_ab, nqus);
        a_checked[grad_qu_l] = *p_nbits_spec;
        ncalls++;
    }

    while ((*p_nbits_spec > nbits_avail) && (grad_qu_l > 0)) {
        p_ab->grad_qu_l = --grad_qu_l;
        *p_nbits_spec = encode_audio_block_a_ldac(p_ab, nqus);
        ncalls++;
    }

    return ncalls;
}

/***************************************************************************************************
    Subfunction: Adjust Remaining Bits
***************************************************************************************************/
static int adjust_remain_bits_ldac(
AB *p_ab, 
int *p_nbits_spec,
int *p_nadjqus)
{
    int ich, iqu;
    int ncalls = 0;
    int nbits_fix, nbits_spec;
    int nbits_avail = p_ab->nbits_avail;
    int idsp, idwl1, idwl2, tmp;
    int step = LDAC_MAXNADJQUS>>1;
    int nadjqus = LDAC_MAXNADJQUS>>1;
    int nchs = p_ab->blk_nchs;
    int nqus = min_ldac(LDAC_MAXNADJQUS, p_ab->nqus);
    int grad_mode = p_ab->grad_mode;
    int *p_grad = p_ab->a_grad;
    int *p_idsf, *p_addwl, *p_idwl1, *p_idwl2, *p_tmp;
    AC *p_ac;

    nbits_fix = 0;
    for (ich = 0; ich < nchs; ich++){
        p_ac = p_ab->ap_ac[ich];
        p_idsf = p_ac->a_idsf;
        p_addwl = p_ac->a_addwl;
        p_idwl1 = p_ac->a_idwl1;
        p_idwl2 = p_ac->a_idwl2;
        p_tmp = p_ac->a_tmp;

        if (grad_mode == LDAC_MODE_0) {
            for (iqu = 0; iqu < nqus; iqu++) {
		idwl1 = p_idwl1[iqu];
		idwl2 = p_idwl2[iqu];
                idsp = ga_idsp_ldac[iqu];
                nbits_fix += gaa_ndim_wls_ldac[idsp][idwl1] + ga_wl_ldac[idwl2] * ga_nsps_ldac[iqu];
                tmp = p_idsf[iqu] + p_grad[iqu];
                if (tmp < LDAC_MINIDWL1) {
                    tmp = LDAC_MINIDWL1;
                }
                p_tmp[iqu] = tmp;
            }
        }
        else if (grad_mode == LDAC_MODE_1) {
            for (iqu = 0; iqu < nqus; iqu++) {
		idwl1 = p_idwl1[iqu];
		idwl2 = p_idwl2[iqu];
                idsp = ga_idsp_ldac[iqu];
                nbits_fix += gaa_ndim_wls_ldac[idsp][idwl1] + ga_wl_ldac[idwl2] * ga_nsps_ldac[iqu];
                tmp = p_idsf[iqu] + p_grad[iqu] + p_addwl[iqu];
                if (tmp > 0) {
                    tmp = tmp >> 1;
                }
                if (tmp < LDAC_MINIDWL1) {
                    tmp = LDAC_MINIDWL1;
                }
                p_tmp[iqu] = tmp;
            }
        }
        else if (grad_mode == LDAC_MODE_2) {
            for (iqu = 0; iqu < nqus; iqu++) {
		idwl1 = p_idwl1[iqu];
		idwl2 = p_idwl2[iqu];
                idsp = ga_idsp_ldac[iqu];
                nbits_fix += gaa_ndim_wls_ldac[idsp][idwl1] + ga_wl_ldac[idwl2] * ga_nsps_ldac[iqu];
                tmp = p_idsf[iqu] + p_grad[iqu] + p_addwl[iqu];
                if (tmp > 0) {
                    tmp = (tmp*3) >> 3;
                }
                if (tmp < LDAC_MINIDWL1) {
                    tmp = LDAC_MINIDWL1;
                }
                p_tmp[iqu] = tmp;
            }
        }
        else if (grad_mode == LDAC_MODE_3) {
            for (iqu = 0; iqu < nqus; iqu++) {
		idwl1 = p_idwl1[iqu];
		idwl2 = p_idwl2[iqu];
                idsp = ga_idsp_ldac[iqu];
                nbits_fix += gaa_ndim_wls_ldac[idsp][idwl1] + ga_wl_ldac[idwl2] * ga_nsps_ldac[iqu];
                tmp = p_idsf[iqu] + p_grad[iqu] + p_addwl[iqu];
                if (tmp > 0) {
                    tmp = tmp >> 2;
                }
                if (tmp < LDAC_MINIDWL1) {
                    tmp = LDAC_MINIDWL1;
                }
                p_tmp[iqu] = tmp;
            }
        }
    }

    nbits_fix = *p_nbits_spec - nbits_fix;
    nbits_spec = nbits_fix + encode_audio_block_b_ldac(p_ab, nadjqus);
    ncalls++;

    while (step > 1) {
        step >>= 1;

        if (nbits_spec < nbits_avail) {
            nadjqus += step;
            if (nadjqus > p_ab->nqus) {
                nadjqus = p_ab->nqus;
            }
        }
        else if (nbits_spec > nbits_avail) {
            nadjqus -= step; 
        }
        else {
            if (nadjqus > p_ab->nqus) {
                nadjqus = p_ab->nqus;
            }
            break;
        }
        nbits_spec = nbits_fix + encode_audio_block_b_ldac(p_ab, nadjqus);
        ncalls++;
    }

    if (nbits_spec > nbits_avail) {
        nadjqus--;
        nbits_spec = nbits_fix + encode_audio_block_b_ldac(p_ab, nadjqus);
        ncalls++;
    }
    *p_nadjqus = nadjqus;
    *p_nbits_spec = nbits_spec;

    return ncalls;
}

/***************************************************************************************************
    Allocate Bits
***************************************************************************************************/
#define LDAC_UPPER_NOISE_LEVEL 20
#define LDAC_LOWER_NOISE_LEVEL 5

DECLFUNC int alloc_bits_ldac(
AB *p_ab)
{
    int nbits_avail, nbits_side = 0, nbits_spec = 0;
    int nbits_ab = p_ab->nbits_ab;

    nbits_side = encode_side_info_ldac(p_ab);
    p_ab->nbits_avail = nbits_avail = nbits_ab - nbits_side;

    nbits_spec = encode_audio_block_a_ldac(p_ab, p_ab->nqus);

    if (nbits_spec > nbits_avail) {
        if (p_ab->grad_mode == LDAC_MODE_0) {
            decrease_offset_low_ldac(p_ab, LDAC_UPPER_NOISE_LEVEL, &nbits_spec);

            decrease_offset_high_ldac(p_ab, &nbits_spec);

            decrease_offset_low_ldac(p_ab, LDAC_MAXGRADOS, &nbits_spec);
        }
        else {
            decrease_offset_low_ldac(p_ab, LDAC_MAXGRADOS, &nbits_spec);
        }

        while ((nbits_spec > nbits_avail) && (p_ab->nbands > LDAC_BAND_OFFSET)) {
            p_ab->nbands--;
            p_ab->nqus = ga_nqus_ldac[p_ab->nbands];

            nbits_side = encode_side_info_ldac(p_ab);
            p_ab->nbits_avail = nbits_avail = nbits_ab - nbits_side;

            nbits_spec = encode_audio_block_a_ldac(p_ab, p_ab->nqus);
        }
    }

    if (nbits_spec < nbits_avail) {
        if (p_ab->grad_mode == LDAC_MODE_0) {
            increase_offset_low_ldac(p_ab, &nbits_spec);

            increase_qu_low_0_ldac(p_ab, &nbits_spec);
        }    
        else {
            increase_offset_low_ldac(p_ab, &nbits_spec);

            increase_qu_low_ldac(p_ab, &nbits_spec);
        }
    }

    p_ab->nadjqus = 0;
    adjust_remain_bits_ldac(p_ab, &nbits_spec, &p_ab->nadjqus);

    if (nbits_spec > nbits_avail) {
        *p_ab->p_error_code = LDAC_ERR_BIT_ALLOCATION;
        return LDAC_FALSE;
    }
    p_ab->nbits_spec = nbits_spec;
    p_ab->nbits_used = nbits_spec + nbits_side;


    return LDAC_TRUE;
}


