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
    Pack and Store from MSB
***************************************************************************************************/
static void pack_store_ldac(
int idata,
int nbits,
STREAM *p_block,
int *p_loc)
{
    STREAM *p_bufptr;
    register int bpos;
    register unsigned int tmp;

    p_bufptr = p_block + (*p_loc >> LDAC_LOC_SHIFT);
    bpos = *p_loc & LDAC_LOC_MASK;

    tmp = (idata << (24-nbits)) & 0xffffff;
    tmp >>= bpos;
    *p_bufptr++ |= (tmp>>16);
    *p_bufptr++ = (tmp>>8) & 0xff;
    *p_bufptr = tmp & 0xff;

    *p_loc += nbits;

    return;
}


/***************************************************************************************************
    Pack Frame Header
***************************************************************************************************/
DECLFUNC void pack_frame_header_ldac(
int smplrate_id,
int chconfig_id,
int frame_length,
int frame_status,
STREAM *p_stream)
{
    int loc = 0;

    pack_store_ldac(LDAC_SYNCWORD, LDAC_SYNCWORDBITS, p_stream, &loc);

    pack_store_ldac(smplrate_id, LDAC_SMPLRATEBITS, p_stream, &loc);

    pack_store_ldac(chconfig_id, LDAC_CHCONFIG2BITS, p_stream, &loc);

    pack_store_ldac(frame_length-1, LDAC_FRAMELEN2BITS, p_stream, &loc);

    pack_store_ldac(frame_status, LDAC_FRAMESTATBITS, p_stream, &loc);

    return;
}

/***************************************************************************************************
    Pack Frame Alignment
***************************************************************************************************/
static void pack_frame_alignment_ldac(
STREAM *p_stream,
int *p_loc,
int nbytes_frame)
{
    int i;
    int nbytes_filled;

    nbytes_filled = nbytes_frame - *p_loc / LDAC_BYTESIZE;

    for (i = 0; i < nbytes_filled; i++) {
        pack_store_ldac(LDAC_FILLCODE, LDAC_BYTESIZE, p_stream, p_loc);
    }

    return;
}

/***************************************************************************************************
    Pack Byte Alignment
***************************************************************************************************/
#define pack_block_alignment_ldac(p_stream, p_loc) pack_byte_alignment_ldac((p_stream), (p_loc))

static void pack_byte_alignment_ldac(
STREAM *p_stream,
int *p_loc)
{
    int nbits_padding;

    nbits_padding = ((*p_loc + LDAC_BYTESIZE - 1) / LDAC_BYTESIZE) * LDAC_BYTESIZE - *p_loc;

    if (nbits_padding > 0) {
        pack_store_ldac(0, nbits_padding, p_stream, p_loc);
    }

    return;
}

/***************************************************************************************************
    Pack Band Info
***************************************************************************************************/
static void pack_band_info_ldac(
AB *p_ab,
STREAM *p_stream,
int *p_loc)
{
    pack_store_ldac(p_ab->nbands-LDAC_BAND_OFFSET, LDAC_NBANDBITS, p_stream, p_loc);

    pack_store_ldac(LDAC_FALSE, LDAC_FLAGBITS, p_stream, p_loc);

    return;
}

/***************************************************************************************************
    Pack Gradient Data
***************************************************************************************************/
static void pack_gradient_ldac(
AB *p_ab,
STREAM *p_stream,
int *p_loc)
{
    pack_store_ldac(p_ab->grad_mode, LDAC_GRADMODEBITS, p_stream, p_loc);

    if (p_ab->grad_mode == LDAC_MODE_0) {
        pack_store_ldac(p_ab->grad_qu_l, LDAC_GRADQU0BITS, p_stream, p_loc);

        pack_store_ldac(p_ab->grad_qu_h-1, LDAC_GRADQU0BITS, p_stream, p_loc);

        pack_store_ldac(p_ab->grad_os_l, LDAC_GRADOSBITS, p_stream, p_loc);

        pack_store_ldac(p_ab->grad_os_h, LDAC_GRADOSBITS, p_stream, p_loc);
    }
    else {
        pack_store_ldac(p_ab->grad_qu_l, LDAC_GRADQU1BITS, p_stream, p_loc);

        pack_store_ldac(p_ab->grad_os_l, LDAC_GRADOSBITS, p_stream, p_loc);
    }

    pack_store_ldac(p_ab->nadjqus, LDAC_NADJQUBITS, p_stream, p_loc);

    return;
}

/***************************************************************************************************
    Subfunction: Pack Scale Factor Data - Mode 0
***************************************************************************************************/
static void pack_scale_factor_0_ldac(
AC *p_ac,
STREAM *p_stream,
int *p_loc)
{
    HCENC *p_hcsf;
    int iqu;
    int nqus = p_ac->p_ab->nqus;
    int dif, val0, val1;
    const unsigned char *p_tbl;

    pack_store_ldac(p_ac->sfc_bitlen-LDAC_MINSFCBLEN_0, LDAC_SFCBLENBITS, p_stream, p_loc);

    pack_store_ldac(p_ac->sfc_offset, LDAC_IDSFBITS, p_stream, p_loc);

    pack_store_ldac(p_ac->sfc_weight, LDAC_SFCWTBLBITS, p_stream, p_loc);

    p_tbl = gaa_sfcwgt_ldac[p_ac->sfc_weight];
    val0 = p_ac->a_idsf[0] + p_tbl[0];

    pack_store_ldac(val0-p_ac->sfc_offset, p_ac->sfc_bitlen, p_stream, p_loc);

    p_hcsf = ga_hcenc_sf0_ldac + (p_ac->sfc_bitlen-LDAC_MINSFCBLEN_0);
    for (iqu = 1; iqu < nqus; iqu++) {
        val1 = p_ac->a_idsf[iqu] + p_tbl[iqu];
        dif = (val1 - val0) & p_hcsf->mask;
        pack_store_ldac(hc_word_ldac(p_hcsf->p_tbl+dif), hc_len_ldac(p_hcsf->p_tbl+dif), p_stream, p_loc);
        val0 = val1;
    }

    return;
}

/***************************************************************************************************
    Subfunction: Pack Scale Factor Data - Mode 1
***************************************************************************************************/
static void pack_scale_factor_1_ldac(
AC *p_ac,
STREAM *p_stream,
int *p_loc)
{
    int iqu;
    int nqus = p_ac->p_ab->nqus;
    const unsigned char *p_tbl;

    pack_store_ldac(p_ac->sfc_bitlen-LDAC_MINSFCBLEN_1, LDAC_SFCBLENBITS, p_stream, p_loc);

    if (p_ac->sfc_bitlen > 4) {
        for (iqu = 0; iqu < nqus; iqu++) {
            pack_store_ldac(p_ac->a_idsf[iqu], LDAC_IDSFBITS, p_stream, p_loc);
        }
    }
    else {
        pack_store_ldac(p_ac->sfc_offset, LDAC_IDSFBITS, p_stream, p_loc);

        pack_store_ldac(p_ac->sfc_weight, LDAC_SFCWTBLBITS, p_stream, p_loc);

        p_tbl = gaa_sfcwgt_ldac[p_ac->sfc_weight];
        for (iqu = 0; iqu < nqus; iqu++) {
            pack_store_ldac(p_ac->a_idsf[iqu]+p_tbl[iqu]-p_ac->sfc_offset, p_ac->sfc_bitlen, p_stream, p_loc);
        }
    }

    return;
}

/***************************************************************************************************
    Subfunction: Pack Scale Factor Data - Mode 2
***************************************************************************************************/
static void pack_scale_factor_2_ldac(
AC *p_ac,
STREAM *p_stream,
int *p_loc)
{
    HCENC *p_hcsf;
    int iqu;
    int nqus = p_ac->p_ab->nqus;
    int dif;

    pack_store_ldac(p_ac->sfc_bitlen-LDAC_MINSFCBLEN_2, LDAC_SFCBLENBITS, p_stream, p_loc);

    p_hcsf = ga_hcenc_sf1_ldac + (p_ac->sfc_bitlen-LDAC_MINSFCBLEN_2);
    for (iqu = 0; iqu < nqus; iqu++) {
        dif = (p_ac->a_idsf[iqu] - p_ac->p_ab->ap_ac[0]->a_idsf[iqu]) & p_hcsf->mask;
        pack_store_ldac(hc_word_ldac(p_hcsf->p_tbl+dif), hc_len_ldac(p_hcsf->p_tbl+dif), p_stream, p_loc);
    }

    return;
}

/***************************************************************************************************
    Pack Scale Factor Data
***************************************************************************************************/
static void pack_scale_factor_ldac(
AC *p_ac,
STREAM *p_stream,
int *p_loc)
{
    int sfc_mode = p_ac->sfc_mode;

    pack_store_ldac(sfc_mode, LDAC_SFCMODEBITS, p_stream, p_loc);

    if (p_ac->ich == 0) {
        if (sfc_mode == LDAC_MODE_0) {
            pack_scale_factor_0_ldac(p_ac, p_stream, p_loc);
        }
        else {
            pack_scale_factor_1_ldac(p_ac, p_stream, p_loc);
        }
    }
    else {
        if (sfc_mode == LDAC_MODE_0) {
            pack_scale_factor_0_ldac(p_ac, p_stream, p_loc);
        }
        else {
            pack_scale_factor_2_ldac(p_ac, p_stream, p_loc);
        }
    }

    return;
}

/***************************************************************************************************
    Pack Spectrum Data
***************************************************************************************************/
static void pack_spectrum_ldac(
AC *p_ac,
STREAM *p_stream,
int *p_loc)
{
    int iqu, isp, i;
    int lsp, hsp;
    int nqus = p_ac->p_ab->nqus;
    int nsps, idwl1, wl, val;

    for (iqu = 0; iqu < nqus; iqu++) {
        lsp = ga_isp_ldac[iqu];
        hsp = ga_isp_ldac[iqu+1];
        nsps = ga_nsps_ldac[iqu];
        idwl1 = p_ac->a_idwl1[iqu];
        wl = ga_wl_ldac[idwl1];

        if (idwl1 == 1) {
            isp = lsp;

            if (nsps == 2) {
                val  = (p_ac->a_qspec[isp  ]+1) << 2;
                val += (p_ac->a_qspec[isp+1]+1);
                pack_store_ldac(ga_2dimenc_spec_ldac[val], LDAC_2DIMSPECBITS, p_stream, p_loc);
            }
            else {
                for (i = 0; i < nsps>>2; i++, isp+=4) {
                    val  = (p_ac->a_qspec[isp  ]+1) << 6;
                    val += (p_ac->a_qspec[isp+1]+1) << 4;
                    val += (p_ac->a_qspec[isp+2]+1) << 2;
                    val += (p_ac->a_qspec[isp+3]+1);
                    pack_store_ldac(ga_4dimenc_spec_ldac[val], LDAC_4DIMSPECBITS, p_stream, p_loc);
                }
            }
        }
        else {
            for (isp = lsp; isp < hsp; isp++) {
                pack_store_ldac(p_ac->a_qspec[isp], wl, p_stream, p_loc);
            }
        }
    }

    return;
}

/***************************************************************************************************
    Pack Residual Data
***************************************************************************************************/
static void pack_residual_ldac(
AC *p_ac,
STREAM *p_stream,
int *p_loc)
{
    int iqu, isp;
    int lsp, hsp;
    int nqus = p_ac->p_ab->nqus;
    int idwl2, wl;

    for (iqu = 0; iqu < nqus; iqu++) {
        idwl2 = p_ac->a_idwl2[iqu];

        if (idwl2 > 0) {
            lsp = ga_isp_ldac[iqu];
            hsp = ga_isp_ldac[iqu+1];
            wl = ga_wl_ldac[idwl2];

            for (isp = lsp; isp < hsp; isp++) {
                pack_store_ldac(p_ac->a_rspec[isp], wl, p_stream, p_loc);
            }
        }
    }    

    return;
}

/***************************************************************************************************
    Pack Audio Block
***************************************************************************************************/
static int pack_audio_block_ldac(
AB *p_ab,
STREAM *p_stream,
int *p_loc)
{
    AC *p_ac;
    int ich;
    int nchs = p_ab->blk_nchs;
    int nbits_band, nbits_grad, a_nbits_scfc[2], a_nbits_spec[2], nbits_used;
    int loc;

    for (ich = 0; ich < 2; ich++) {
        a_nbits_scfc[ich] = 0;
        a_nbits_spec[ich] = 0;
    }

    loc = *p_loc;
    pack_band_info_ldac(p_ab, p_stream, p_loc);
    nbits_band = *p_loc - loc;

    loc = *p_loc;
    pack_gradient_ldac(p_ab, p_stream, p_loc);
    nbits_grad = *p_loc - loc;

    nbits_used = nbits_band + nbits_grad;

    for (ich = 0; ich < nchs; ich++) {
        p_ac = p_ab->ap_ac[ich];

        loc = *p_loc;
        pack_scale_factor_ldac(p_ac, p_stream, p_loc);
        a_nbits_scfc[ich] = *p_loc - loc;

        loc = *p_loc;
        pack_spectrum_ldac(p_ac, p_stream, p_loc);
        a_nbits_spec[ich] = *p_loc - loc;

        loc = *p_loc;
        pack_residual_ldac(p_ac, p_stream, p_loc);
        a_nbits_spec[ich] += *p_loc - loc;

        nbits_used += a_nbits_scfc[ich] + a_nbits_spec[ich];
    }

    if (nbits_used > p_ab->nbits_used) {
        *p_ab->p_error_code = LDAC_ERR_BIT_PACKING;
        return LDAC_FALSE;
    }
    else if (nbits_used < p_ab->nbits_used) {
        *p_ab->p_error_code = LDAC_ERR_BIT_PACKING;
        return LDAC_FALSE;
    }

    return LDAC_TRUE;
}

/***************************************************************************************************
    Pack Raw Data Frame
***************************************************************************************************/
DECLFUNC int pack_raw_data_frame_ldac(
SFINFO *p_sfinfo,
STREAM *p_stream,
int *p_loc,
int *p_nbytes_used)
{
    CFG *p_cfg = &p_sfinfo->cfg;
    AB *p_ab = p_sfinfo->p_ab;
    int ibk;
    int nbks = gaa_block_setting_ldac[p_cfg->chconfig_id][1];

    for (ibk = 0; ibk < nbks; ibk++) {
        if (!pack_audio_block_ldac(p_ab, p_stream, p_loc)) {
            return LDAC_ERR_PACK_BLOCK_FAILED;
        }

        pack_block_alignment_ldac(p_stream, p_loc);

        p_ab++;
    }

    pack_frame_alignment_ldac(p_stream, p_loc, p_cfg->frame_length);

    *p_nbytes_used = *p_loc / LDAC_BYTESIZE;

    return LDAC_ERR_NONE;
}

/***************************************************************************************************
    Pack Null Data Frame
***************************************************************************************************/
static const int sa_null_data_size_ldac[2] = {
    11, 15,
};
static const STREAM saa_null_data_ldac[2][15] = {
    {0x07, 0xa0, 0x16, 0x00, 0x20, 0xad, 0x51, 0x45, 0x14, 0x50, 0x49},
    {0x07, 0xa0, 0x0a, 0x00, 0x20, 0xad, 0x51, 0x41, 0x24, 0x93, 0x00, 0x28, 0xa0, 0x92, 0x49},
};

DECLFUNC int pack_null_data_frame_ldac(
SFINFO *p_sfinfo,
STREAM *p_stream,
int *p_loc,
int *p_nbytes_used)
{
    CFG *p_cfg = &p_sfinfo->cfg;
    AB *p_ab = p_sfinfo->p_ab;
    int ibk;
    int nbks = gaa_block_setting_ldac[p_cfg->chconfig_id][1];
    int blk_type, size, offset = 0;

    for (ibk = 0; ibk < nbks; ibk++) {
        blk_type = p_ab->blk_type;
        size = sa_null_data_size_ldac[blk_type];

        copy_data_ldac(saa_null_data_ldac[blk_type], p_stream+offset, size*sizeof(STREAM));
        *p_loc += size*LDAC_BYTESIZE;

        offset += size;
        p_ab++;
    }
    if (p_cfg->frame_length < offset) {
        return LDAC_ERR_PACK_BLOCK_FAILED;
    }

    pack_frame_alignment_ldac(p_stream, p_loc, p_cfg->frame_length);

    *p_nbytes_used = *p_loc / LDAC_BYTESIZE;

    return LDAC_ERR_NONE;
}

