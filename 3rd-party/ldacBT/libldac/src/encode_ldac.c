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
    Allocate Memory
***************************************************************************************************/
static LDAC_RESULT alloc_encode_ldac(
SFINFO *p_sfinfo)
{
    LDAC_RESULT result = LDAC_S_OK;
    CFG *p_cfg = &p_sfinfo->cfg;
    int ich;
    int nchs = p_cfg->ch;
    int nbks = gaa_block_setting_ldac[p_cfg->chconfig_id][1];

    /* Allocate AC */
    for (ich = 0; ich < nchs; ich++) {
        p_sfinfo->ap_ac[ich] = (AC *)calloc_ldac(p_sfinfo, 1, sizeof(AC));
        if (p_sfinfo->ap_ac[ich] != (AC *)NULL) {
            p_sfinfo->ap_ac[ich]->p_acsub = (ACSUB *)calloc_ldac(p_sfinfo, 1, sizeof(ACSUB));
            if (p_sfinfo->ap_ac[ich]->p_acsub == (ACSUB *)NULL) {
                result = LDAC_E_FAIL;
                break;
            }
        }
        else {
            result = LDAC_E_FAIL;
            break;
        }
    }

    if (result != LDAC_S_OK) {
        return result;
    }

    /* Allocate AB */
    p_sfinfo->p_ab = (AB *)calloc_ldac(p_sfinfo, nbks, sizeof(AB));
    if (p_sfinfo->p_ab == (AB *)NULL) {
        result = LDAC_E_FAIL;
    }

    return result;
}

/***************************************************************************************************
    Initialize Memory
***************************************************************************************************/
DECLFUNC LDAC_RESULT init_encode_ldac(
SFINFO *p_sfinfo)
{
    LDAC_RESULT result = LDAC_S_OK;
    CFG *p_cfg = &p_sfinfo->cfg;
    AB *p_ab;
    int ibk, ich;
    int blk_type, blk_nchs;
    int ch_offset = 0;
    int chconfig_id = p_cfg->chconfig_id;
    int nbks = gaa_block_setting_ldac[chconfig_id][1];

    if (alloc_encode_ldac(p_sfinfo) == LDAC_E_FAIL) {
        p_sfinfo->error_code = LDAC_ERR_ALLOC_MEMORY;
        return LDAC_E_FAIL;
    }

    p_sfinfo->error_code = LDAC_ERR_NONE;
    p_cfg->frame_status = LDAC_FRMSTAT_LEV_0;

    /* Set AB information */
    p_ab = p_sfinfo->p_ab;
    for (ibk = 0; ibk < nbks; ibk++){
        p_ab->blk_type = blk_type = gaa_block_setting_ldac[chconfig_id][ibk+2];
        p_ab->blk_nchs = blk_nchs = get_block_nchs_ldac(blk_type);
        p_ab->p_smplrate_id = &p_cfg->smplrate_id;
        p_ab->p_error_code = &p_sfinfo->error_code;

        /* Set AC Information */
        for (ich = 0; ich < blk_nchs; ich++) {
            p_ab->ap_ac[ich] = p_sfinfo->ap_ac[ch_offset++];
            p_ab->ap_ac[ich]->p_ab = p_ab;
            p_ab->ap_ac[ich]->ich = ich;
            p_ab->ap_ac[ich]->frmana_cnt = 0;
        }

        p_ab++;
    }

    calc_initial_bits_ldac(p_sfinfo);

    return result;
}

/***************************************************************************************************
    Calculate Initial Bits
***************************************************************************************************/
DECLFUNC void calc_initial_bits_ldac(
SFINFO *p_sfinfo)
{
    CFG *p_cfg = &p_sfinfo->cfg;
    AB *p_ab = p_sfinfo->p_ab;
    int ibk;
    int blk_type;
    int nbits_ab, nbits_ac;
    int chconfig_id = p_cfg->chconfig_id;
    int nbks = gaa_block_setting_ldac[chconfig_id][1];

    nbits_ac = p_cfg->frame_length * LDAC_BYTESIZE / p_cfg->ch;

    for (ibk = 0; ibk < nbks; ibk++){
        blk_type = gaa_block_setting_ldac[chconfig_id][ibk+2];

        if (blk_type == LDAC_BLKID_STEREO){
            nbits_ab = (nbits_ac * 2 / LDAC_BYTESIZE) * LDAC_BYTESIZE;
        }
        else{
            nbits_ab = (nbits_ac / LDAC_BYTESIZE) * LDAC_BYTESIZE;
        }
        p_ab->nbits_ab = nbits_ab;

        p_ab++;
    }

    return;
}

/***************************************************************************************************
    Free Memory
***************************************************************************************************/
DECLFUNC void free_encode_ldac(
SFINFO *p_sfinfo)
{
    int ich;
    int nchs = p_sfinfo->cfg.ch;

    /* Free AB */
    if (p_sfinfo->p_ab != (AB *)NULL) {
        free(p_sfinfo->p_ab);
        p_sfinfo->p_ab = (AB *)NULL;
    }

    /* Free AC */
    for (ich = 0; ich < nchs; ich++) {
        if (p_sfinfo->ap_ac[ich] != (AC *)NULL) {
            if (p_sfinfo->ap_ac[ich]->p_acsub != (ACSUB *)NULL) {
                free(p_sfinfo->ap_ac[ich]->p_acsub);
                p_sfinfo->ap_ac[ich]->p_acsub = (ACSUB *)NULL;
            }
            free(p_sfinfo->ap_ac[ich]);
            p_sfinfo->ap_ac[ich] = (AC *)NULL;
        }
    }

    return;
}

/***************************************************************************************************
    Encode Audio Block
***************************************************************************************************/
static int encode_audio_block_ldac(
AB *p_ab)
{
    AC *p_ac;
    int ich;
    int nchs = p_ab->blk_nchs;

    for (ich = 0; ich < nchs; ich++) {
        p_ac = p_ab->ap_ac[ich];

        norm_spectrum_ldac(p_ac);
    }

    if (!alloc_bits_ldac(p_ab)) {
        return LDAC_FALSE;
    }

    for (ich = 0; ich < nchs; ich++) {
        p_ac = p_ab->ap_ac[ich];

        quant_spectrum_ldac(p_ac);

        quant_residual_ldac(p_ac);
    }

    return LDAC_TRUE;
}

/***************************************************************************************************
    Encode
***************************************************************************************************/
DECLFUNC int encode_ldac(
SFINFO *p_sfinfo,
int nbands,
int grad_mode,
int grad_qu_l,
int grad_qu_h,
int grad_os_l,
int grad_os_h,
int abc_status)
{
    AB *p_ab = p_sfinfo->p_ab;
    int ibk;
    int nbks = gaa_block_setting_ldac[p_sfinfo->cfg.chconfig_id][1];

    for (ibk = 0; ibk < nbks; ibk++){
        p_ab->nbands = nbands;
        p_ab->nqus = ga_nqus_ldac[nbands];
        p_ab->grad_mode = grad_mode;
        p_ab->grad_qu_l = grad_qu_l;
        p_ab->grad_qu_h = grad_qu_h;
        p_ab->grad_os_l = grad_os_l;
        p_ab->grad_os_h = grad_os_h;
        p_ab->abc_status = abc_status;

        if (!encode_audio_block_ldac(p_ab)) {
            return LDAC_ERR_NON_FATAL_ENCODE;
        }

        p_ab++;
    }

    return LDAC_ERR_NONE;
}

