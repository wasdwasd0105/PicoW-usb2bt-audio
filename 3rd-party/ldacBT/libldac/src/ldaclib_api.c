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

#include "ldaclib.h"
#include "ldac.h"

#define LDACLIB_MAJOR_VERSION  01
#define LDACLIB_MINOR_VERSION  00
#define LDACLIB_BRANCH_VERSION 00

/***************************************************************************************************
    Local Assert Functions
***************************************************************************************************/
static int ldaclib_assert_sampling_rate_index(
int smplrate_id)
{
    if ((LDAC_SMPLRATEID_0 <= smplrate_id) && (smplrate_id < LDAC_NSMPLRATEID)) {
        return LDAC_TRUE;
    }
    else {
        return LDAC_FALSE;
    }
}

static int ldaclib_assert_supported_sampling_rate_index(
int smplrate_id)
{
    if ((LDAC_SMPLRATEID_0 <= smplrate_id) && (smplrate_id < LDAC_NSUPSMPLRATEID)) {
        return LDAC_TRUE;
    }
    else {
        return LDAC_FALSE;
    }
}

static int ldaclib_assert_channel_config_index(
int chconfig_id)
{
    if ((chconfig_id == LDAC_CHCONFIGID_MN)
            || (chconfig_id == LDAC_CHCONFIGID_DL) || (chconfig_id == LDAC_CHCONFIGID_ST)) {
        return LDAC_TRUE;
    }
    else {
        return LDAC_FALSE;
    }
}

static int ldaclib_assert_channel(
int ch)
{
    if ((ch == LDAC_CHANNEL_1CH) || (ch == LDAC_CHANNEL_2CH)) {
        return LDAC_TRUE;
    }
    else {
        return LDAC_FALSE;
    }
}

static int ldaclib_assert_frame_length(
int frame_length)
{
    if ((0 < frame_length) && (frame_length <= LDAC_MAXNBYTES)) {
        return LDAC_TRUE;
    }
    else {
        return LDAC_FALSE;
    }
}

static int ldaclib_assert_supported_frame_length(
int frame_length,
int chconfig_id)
{
    if (chconfig_id == LDAC_CHCONFIGID_MN) {
        if ((LDAC_MINSUPNBYTES/2 <= frame_length) && (frame_length <= LDAC_MAXSUPNBYTES/2)) {
            return LDAC_TRUE;
        }
        else {
            return LDAC_FALSE;
        }
    }
    else if ((chconfig_id == LDAC_CHCONFIGID_DL) || (chconfig_id == LDAC_CHCONFIGID_ST)) {
        if ((LDAC_MINSUPNBYTES <= frame_length) && (frame_length <= LDAC_MAXSUPNBYTES)) {
            return LDAC_TRUE;
        }
        else {
            return LDAC_FALSE;
        }
    }
    else {
        return LDAC_FALSE;
    }
}

static int ldaclib_assert_frame_status(
int frame_status)
{
    if ((LDAC_FRMSTAT_LEV_0 <= frame_status) && (frame_status <= LDAC_FRMSTAT_LEV_3)) {
        return LDAC_TRUE;
    }
    else {
        return LDAC_FALSE;
    }
}

static int ldaclib_assert_nlnn_shift(
int nlnn_shift)
{
    if ((-2 <= nlnn_shift) && (nlnn_shift < LDAC_NSFTSTEP-2)) {
        return LDAC_TRUE;
    }
    else {
        return LDAC_FALSE;
    }
}

static int ldaclib_assert_sample_format(
LDAC_SMPL_FMT_T sample_format)
{
#ifndef _32BIT_FIXED_POINT
    if ((LDAC_SMPL_FMT_S16 <= sample_format) && (sample_format <= LDAC_SMPL_FMT_F32)) {
#else /* _32BIT_FIXED_POINT */
    if ((LDAC_SMPL_FMT_S16 <= sample_format) && (sample_format <= LDAC_SMPL_FMT_S32)) {
#endif /* _32BIT_FIXED_POINT */
        return LDAC_TRUE;
    }
    else {
        return LDAC_FALSE;
    }
}


/***************************************************************************************************
    Common API Functions
***************************************************************************************************/

/***************************************************************************************************
    Get Library Version
***************************************************************************************************/
DECLSPEC int ldaclib_get_version(void) {
    return (LDACLIB_MAJOR_VERSION<<16) | (LDACLIB_MINOR_VERSION<<8) | LDACLIB_BRANCH_VERSION;
}

DECLSPEC int ldaclib_get_major_version(void) {
    return LDACLIB_MAJOR_VERSION;
}

DECLSPEC int ldaclib_get_minor_version(void) {
    return LDACLIB_MINOR_VERSION;
}

DECLSPEC int ldaclib_get_branch_version(void) {
    return LDACLIB_BRANCH_VERSION;
}

/***************************************************************************************************
    Get Basic Parameters
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_get_sampling_rate_index(
int smplrate,
int *p_smplrate_id)
{
    if (smplrate == 44100) {
        *p_smplrate_id = LDAC_SMPLRATEID_0;
    }
    else if (smplrate == 48000) {
        *p_smplrate_id = LDAC_SMPLRATEID_1;
    }
    else if (smplrate == 88200) {
        *p_smplrate_id = LDAC_SMPLRATEID_2;
    }
    else if (smplrate == 96000) {
        *p_smplrate_id = LDAC_SMPLRATEID_3;
    }
    else {
        return LDAC_E_FAIL;
    }

    return LDAC_S_OK;
}

DECLSPEC LDAC_RESULT ldaclib_get_sampling_rate(
int smplrate_id,
int *p_smplrate)
{
    if (!ldaclib_assert_sampling_rate_index(smplrate_id)) {
        return LDAC_E_FAIL;
    }
    if (!ldaclib_assert_supported_sampling_rate_index(smplrate_id)) {
        return LDAC_E_FAIL;
    }

    *p_smplrate = ga_smplrate_ldac[smplrate_id];

    return LDAC_S_OK;
}

DECLSPEC LDAC_RESULT ldaclib_get_frame_samples(
int smplrate_id,
int *p_framesmpls)
{
    if (!ldaclib_assert_sampling_rate_index(smplrate_id)) {
        return LDAC_E_FAIL;
    }
    if (!ldaclib_assert_supported_sampling_rate_index(smplrate_id)) {
        return LDAC_E_FAIL;
    }

    *p_framesmpls = ga_framesmpls_ldac[smplrate_id];

    return LDAC_S_OK;
}

DECLSPEC LDAC_RESULT ldaclib_get_nlnn(
int smplrate_id,
int *p_nlnn)
{
    if (!ldaclib_assert_sampling_rate_index(smplrate_id)) {
        return LDAC_E_FAIL;
    }
    if (!ldaclib_assert_supported_sampling_rate_index(smplrate_id)) {
        return LDAC_E_FAIL;
    }

    *p_nlnn = ga_ln_framesmpls_ldac[smplrate_id];

    return LDAC_S_OK;
}

DECLSPEC LDAC_RESULT ldaclib_get_channel(
int chconfig_id,
int *p_ch)
{
    if (!ldaclib_assert_channel_config_index(chconfig_id)) {
        return LDAC_E_FAIL;
    }

    *p_ch = ga_ch_ldac[chconfig_id];

    return LDAC_S_OK;
}

DECLSPEC LDAC_RESULT ldaclib_get_channel_config_index(
int ch,
int *p_chconfig_id)
{
    if (!ldaclib_assert_channel(ch)) {
        return LDAC_E_FAIL;
    }

    *p_chconfig_id = ga_chconfig_id_ldac[ch];

    return LDAC_S_OK;
}

DECLSPEC LDAC_RESULT ldaclib_check_nlnn_shift(
int smplrate_id,
int nlnn_shift)
{
    if (!ldaclib_assert_sampling_rate_index(smplrate_id)) {
        return LDAC_E_FAIL;
    }
    if (!ldaclib_assert_supported_sampling_rate_index(smplrate_id)) {
        return LDAC_E_FAIL;
    }
    if (!ldaclib_assert_nlnn_shift(nlnn_shift)) {
        return LDAC_E_FAIL;
    }

    if (gaa_nlnn_shift_ldac[smplrate_id][nlnn_shift+2] < 0) {
        return LDAC_E_FAIL;
    }

    return LDAC_S_OK;
}

/***************************************************************************************************
    Get Handle
***************************************************************************************************/
DECLSPEC HANDLE_LDAC ldaclib_get_handle(
void)
{
    HANDLE_LDAC hData;

    hData = (HANDLE_LDAC)malloc(sizeof(HANDLE_LDAC_STRUCT));
    if (hData != (HANDLE_LDAC)NULL) {
        clear_data_ldac(hData, sizeof(HANDLE_LDAC_STRUCT));
        hData->sfinfo.p_mempos = (char *)NULL;
        hData->error_code = LDAC_ERR_NONE;
    }

    return hData;
}

/***************************************************************************************************
    Free Handle
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_free_handle(
HANDLE_LDAC hData)
{
    if (hData != (HANDLE_LDAC)NULL) {
        if (hData->sfinfo.p_mempos != (char *)NULL) {
            return LDAC_S_OK;
        }

        free(hData);
    }

    return LDAC_S_OK;
}

/***************************************************************************************************
    Set Configuration Information
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_set_config_info(
HANDLE_LDAC hData,
int smplrate_id,
int chconfig_id,
int frame_length,
int frame_status)
{
    CFG *p_cfg = &hData->sfinfo.cfg;

    if (!ldaclib_assert_sampling_rate_index(smplrate_id)) {
        hData->error_code = LDAC_ERR_ASSERT_SAMPLING_RATE;
        return LDAC_E_FAIL;
    }

    if (!ldaclib_assert_supported_sampling_rate_index(smplrate_id)) {
        hData->error_code = LDAC_ERR_ASSERT_SUP_SAMPLING_RATE;
        return LDAC_E_FAIL;
    }

    if (!ldaclib_assert_channel_config_index(chconfig_id)) {
        hData->error_code = LDAC_ERR_ASSERT_CHANNEL_CONFIG;
        return LDAC_E_FAIL;
    }

    if (!ldaclib_assert_frame_length(frame_length)) {
        hData->error_code = LDAC_ERR_ASSERT_FRAME_LENGTH;
        return LDAC_E_FAIL;
    }

    if (!ldaclib_assert_supported_frame_length(frame_length, chconfig_id)) {
        hData->error_code = LDAC_ERR_ASSERT_SUP_FRAME_LENGTH;
        return LDAC_E_FAIL;
    }

    if (!ldaclib_assert_frame_status(frame_status)) {
        hData->error_code = LDAC_ERR_ASSERT_FRAME_STATUS;
        return LDAC_E_FAIL;
    }

    p_cfg->smplrate_id = smplrate_id;
    p_cfg->chconfig_id = chconfig_id;
    p_cfg->frame_length = frame_length;
    p_cfg->frame_status = frame_status;

    ldaclib_get_channel(chconfig_id, &p_cfg->ch);

    return LDAC_S_OK;
}

/***************************************************************************************************
    Get Configuration Information
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_get_config_info(
HANDLE_LDAC hData,
int *p_smplrate_id,
int *p_chconfig_id,
int *p_frame_length,
int *p_frame_status)
{
    CFG *p_cfg = &hData->sfinfo.cfg;

    *p_smplrate_id = p_cfg->smplrate_id;
    *p_chconfig_id = p_cfg->chconfig_id;
    *p_frame_length = p_cfg->frame_length;
    *p_frame_status = p_cfg->frame_status;

    return LDAC_S_OK;
}


/***************************************************************************************************
    Set Frame Header
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_set_frame_header(
HANDLE_LDAC hData,
unsigned char *p_stream,
int smplrate_id,
int chconfig_id,
int frame_length,
int frame_status)
{
    if (!ldaclib_assert_sampling_rate_index(smplrate_id)) {
        hData->error_code = LDAC_ERR_ASSERT_SAMPLING_RATE;
        return LDAC_E_FAIL;
    }

    if (!ldaclib_assert_supported_sampling_rate_index(smplrate_id)) {
        hData->error_code = LDAC_ERR_ASSERT_SUP_SAMPLING_RATE;
        return LDAC_E_FAIL;
    }

    if (!ldaclib_assert_channel_config_index(chconfig_id)) {
        hData->error_code = LDAC_ERR_ASSERT_CHANNEL_CONFIG;
        return LDAC_E_FAIL;
    }

    if (!ldaclib_assert_frame_length(frame_length)) {
        hData->error_code = LDAC_ERR_ASSERT_FRAME_LENGTH;
        return LDAC_E_FAIL;
    }

    if (!ldaclib_assert_supported_frame_length(frame_length, chconfig_id)) {
        hData->error_code = LDAC_ERR_ASSERT_SUP_FRAME_LENGTH;
        return LDAC_E_FAIL;
    }

    if (!ldaclib_assert_frame_status(frame_status)) {
        hData->error_code = LDAC_ERR_ASSERT_FRAME_STATUS;
        return LDAC_E_FAIL;
    }

    pack_frame_header_ldac(smplrate_id, chconfig_id, frame_length, frame_status,
            (STREAM *)p_stream);

    return LDAC_S_OK;
}


/***************************************************************************************************
    Encoder API Functions
***************************************************************************************************/

/***************************************************************************************************
    Get Encoder Setting
***************************************************************************************************/
#define LDAC_ENC_NSETTING 15
#define LDAC_ENC_NPROPERTY 9

static const int saa_encode_setting_ldac[LDAC_ENC_NSETTING][LDAC_ENC_NPROPERTY] = {
    {0, 512,  17,   0,  28,  44,   8,  24,   0},
    {0, 256,  17,   0,  28,  44,   6,  22,   0},
#ifdef MODIFY_LDAC_ENC_SETTING_FOR_ABR_DEBUG  // See file "ldacBT_abr.h" for description
    {0, 164,  16,   0,  18,  32,   7,  23,   0},
    {0, 110,   8,   0,  16,  32,  10,  31,   0},
    {0,  82,   6,   0,  16,  32,  12,  31,   0},
    {0,  66,   4,   0,  14,  26,  12,  31,   0},
    {0,  54,   2,   0,  14,  26,  12,  31,   0},
    {0,  46,   2,   1,  10,  26,  12,  31,   0},
    {0,  40,   2,   2,  10,  26,  12,  31,   0},
    {0,  36,   2,   2,   8,  26,  12,  31,   0},
    {0,  32,   2,   2,   8,  26,  16,  31,   0},
    {0,  30,   2,   2,   4,  26,  16,  31,   0},
    {0,  26,   2,   3,   4,  26,  16,  31,   0},
    {0,  24,   2,   3,   4,  26,  16,  31,   0},
#else
    {0, 164,  16,   0,  18,  32,   7,  23,   0},
    {0, 110,  13,   0,  16,  32,  10,  31,   0},
    {0,  82,  12,   0,  16,  32,  12,  31,   0},
    {0,  66,  11,   0,  14,  26,  12,  31,   0},
    {0,  54,  10,   0,  14,  26,  12,  31,   0},
    {0,  46,   9,   1,  10,  26,  12,  31,   0},
    {0,  40,   8,   2,  10,  26,  12,  31,   0},
    {0,  36,   7,   2,   8,  26,  12,  31,   0},
    {0,  32,   6,   2,   8,  26,  16,  31,   0},
    {0,  30,   5,   2,   4,  26,  16,  31,   0},
    {0,  26,   4,   3,   4,  26,  16,  31,   0},
    {0,  24,   3,   3,   4,  26,  16,  31,   0},
#endif
    {0,  22,   2,   3,   4,  26,  16,  31,   0},
};

DECLSPEC LDAC_RESULT ldaclib_get_encode_setting(
int nbytes_ch,
int smplrate_id,
int *p_nbands,
int *p_grad_mode,
int *p_grad_qu_l,
int *p_grad_qu_h,
int *p_grad_os_l,
int *p_grad_os_h,
int *p_abc_status)
{
    int i, id;

    id = LDAC_ENC_NSETTING-1;
    for (i = LDAC_ENC_NSETTING-1; i >= 0; i--) {
        if (nbytes_ch >= saa_encode_setting_ldac[i][1]) {
            id = i;
        }
    }

    *p_nbands = min_ldac(saa_encode_setting_ldac[id][2], ga_max_nbands_ldac[smplrate_id]);
    *p_grad_mode = saa_encode_setting_ldac[id][3];
    *p_grad_qu_l = saa_encode_setting_ldac[id][4];
    *p_grad_qu_h = saa_encode_setting_ldac[id][5];
    *p_grad_os_l = saa_encode_setting_ldac[id][6];
    *p_grad_os_h = saa_encode_setting_ldac[id][7];
    *p_abc_status = saa_encode_setting_ldac[id][8];

    return LDAC_S_OK;
}

/***************************************************************************************************
    Set Frame Length
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_set_encode_frame_length(
HANDLE_LDAC hData,
int frame_length)
{
    CFG *p_cfg = &hData->sfinfo.cfg;

    if (!ldaclib_assert_frame_length(frame_length)) {
        hData->error_code = LDAC_ERR_ASSERT_FRAME_LENGTH;
        return LDAC_E_FAIL;
    }

    if (!ldaclib_assert_supported_frame_length(frame_length, p_cfg->chconfig_id)) {
        hData->error_code = LDAC_ERR_ASSERT_SUP_FRAME_LENGTH;
        return LDAC_E_FAIL;
    }

    p_cfg->frame_length = frame_length;

    calc_initial_bits_ldac(&hData->sfinfo);

    return LDAC_S_OK;
}

/***************************************************************************************************
    Get Frame Length
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_get_encode_frame_length(
HANDLE_LDAC hData,
int *p_frame_length)
{
    CFG *p_cfg = &hData->sfinfo.cfg;

    *p_frame_length = p_cfg->frame_length;

    return LDAC_S_OK;
}

/***************************************************************************************************
    Set Information
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_set_encode_info(
HANDLE_LDAC hData,
int nbands,
int grad_mode,
int grad_qu_l,
int grad_qu_h,
int grad_os_l,
int grad_os_h,
int abc_status)
{
    if ((nbands < LDAC_BAND_OFFSET) ||
            (ga_max_nbands_ldac[hData->sfinfo.cfg.smplrate_id] < nbands)) {
        hData->error_code = LDAC_ERR_ENC_ILL_NBANDS;
        return LDAC_E_FAIL;
    }

    if ((grad_mode < LDAC_MODE_0) || (LDAC_MODE_3 < grad_mode)) {
            hData->error_code = LDAC_ERR_ENC_ILL_GRADMODE;
            return LDAC_E_FAIL;
    }

    if (grad_mode == LDAC_MODE_0) {
        if ((grad_qu_l < 0) || (LDAC_MAXGRADQU <= grad_qu_l)) {
            hData->error_code = LDAC_ERR_ENC_ILL_GRADPAR_A;
            return LDAC_E_FAIL;
        }

        if ((grad_qu_h < 1) || (LDAC_MAXGRADQU+1 <= grad_qu_h) || (grad_qu_h < grad_qu_l)) {
            hData->error_code = LDAC_ERR_ENC_ILL_GRADPAR_B;
            return LDAC_E_FAIL;
        }

        if ((grad_os_h < 0) || (LDAC_NIDSF <= grad_os_h)) {
            hData->error_code = LDAC_ERR_ENC_ILL_GRADPAR_C;
            return LDAC_E_FAIL;
        }
    }
    else {
        if ((grad_qu_l < 0) || (LDAC_DEFGRADQUH < grad_qu_l)) {
            hData->error_code = LDAC_ERR_ENC_ILL_GRADPAR_A;
            return LDAC_E_FAIL;
        }
    }

    if ((grad_os_l < 0) || (LDAC_NIDSF <= grad_os_l)) {
        hData->error_code = LDAC_ERR_ENC_ILL_GRADPAR_D;
        return LDAC_E_FAIL;
    }

    hData->nbands = nbands;
    hData->grad_mode = grad_mode;
    hData->grad_qu_l = grad_qu_l;
    hData->grad_os_l = grad_os_l;
    if (grad_mode == LDAC_MODE_0) {
        hData->grad_qu_h = grad_qu_h;
        hData->grad_os_h = grad_os_h;
    }
    else {
        hData->grad_qu_h = LDAC_DEFGRADQUH;
        hData->grad_os_h = LDAC_DEFGRADOSH;
    }
    hData->abc_status = abc_status;

    return LDAC_S_OK;
}

/***************************************************************************************************
    Initialize
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_init_encode(
HANDLE_LDAC hData)
{
    SFINFO *p_sfinfo = &hData->sfinfo;
    LDAC_RESULT result;


    ldaclib_get_nlnn(p_sfinfo->cfg.smplrate_id, &hData->nlnn);

    set_mdct_table_ldac(hData->nlnn);

    result = init_encode_ldac(p_sfinfo);
    if (result != LDAC_S_OK) {
        hData->error_code = LDAC_ERR_ENC_INIT_ALLOC;
        return LDAC_E_FAIL;
    }

    return LDAC_S_OK;
}

/***************************************************************************************************
    Free
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_free_encode(
HANDLE_LDAC hData)
{
    if (hData->sfinfo.p_mempos == NULL) {
        free_encode_ldac(&hData->sfinfo);
    }

    return LDAC_S_OK;
}

/***************************************************************************************************
    Encode
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_encode(
HANDLE_LDAC hData,
char *ap_pcm[],
LDAC_SMPL_FMT_T sample_format,
unsigned char *p_stream,
int *p_nbytes_used)
{
    SFINFO *p_sfinfo = &hData->sfinfo;
    int loc = 0;
    int error_code;
    int frame_length;


    if (!ldaclib_assert_sample_format(sample_format)) {
        hData->error_code = LDAC_ERR_ILL_SMPL_FORMAT;
        return LDAC_E_FAIL;
    }

    frame_length = p_sfinfo->cfg.frame_length;
    clear_data_ldac(p_stream, frame_length*sizeof(unsigned char));

    set_input_pcm_ldac(p_sfinfo, ap_pcm, sample_format, hData->nlnn);

    proc_mdct_ldac(p_sfinfo, hData->nlnn);

    p_sfinfo->cfg.frame_status = ana_frame_status_ldac(p_sfinfo, hData->nlnn);

    error_code = encode_ldac(p_sfinfo, hData->nbands, hData->grad_mode,
            hData->grad_qu_l, hData->grad_qu_h, hData->grad_os_l, hData->grad_os_h,
            hData->abc_status);
    if (LDAC_ERROR(error_code) && !LDAC_FATAL_ERROR(error_code)) {
        int error_code2;
        error_code2 = pack_null_data_frame_ldac(p_sfinfo, (STREAM *)p_stream, &loc, p_nbytes_used);
        if (LDAC_FATAL_ERROR(error_code2)) {
            clear_data_ldac(p_stream, frame_length*sizeof(unsigned char));
            hData->error_code = error_code2;
            return LDAC_E_FAIL;
        }
        hData->error_code = error_code;
        return LDAC_S_FALSE;
    }

    error_code = pack_raw_data_frame_ldac(p_sfinfo, (STREAM *)p_stream, &loc, p_nbytes_used);
    if (LDAC_FATAL_ERROR(error_code)) {
        int error_code2;
        loc = 0;
        clear_data_ldac(p_stream, frame_length*sizeof(unsigned char));
        error_code2 = pack_null_data_frame_ldac(p_sfinfo, (STREAM *)p_stream, &loc, p_nbytes_used);
        if (LDAC_FATAL_ERROR(error_code2)) {
            clear_data_ldac(p_stream, frame_length*sizeof(unsigned char));
            hData->error_code = error_code2;
            return LDAC_E_FAIL;
        }
        hData->error_code = error_code;
        return LDAC_E_FAIL;
    }

    return LDAC_S_OK;
}

/***************************************************************************************************
    Flush Encode
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_flush_encode(
HANDLE_LDAC hData,
LDAC_SMPL_FMT_T sample_format,
unsigned char *p_stream,
int *p_nbytes_used)
{
    LDAC_RESULT result;
    int ich;
    char *ap_buf[LDAC_PRCNCH];
    int a_buf[LDAC_MAXLSU*LDAC_PRCNCH];

    if (!ldaclib_assert_sample_format(sample_format)) {
        hData->error_code = LDAC_ERR_ILL_SMPL_FORMAT;
        return LDAC_E_FAIL;
    }

    clear_data_ldac(a_buf, (LDAC_MAXLSU*LDAC_PRCNCH)*sizeof(int));

    for (ich = 0; ich < LDAC_PRCNCH; ich++) {
        ap_buf[ich] = (char *)(a_buf + ich * LDAC_MAXLSU);
    }

    result = ldaclib_encode(hData, ap_buf, sample_format, p_stream, p_nbytes_used);

    return result;
}




/***************************************************************************************************
    Error Code Dispatch
***************************************************************************************************/

/***************************************************************************************************
    Clear Error Code at Handle Level
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_clear_error_code(
HANDLE_LDAC hData)
{
    hData->error_code = LDAC_ERR_NONE;

    return LDAC_S_OK;
}

/***************************************************************************************************
    Get Error Code at Handle Level
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_get_error_code(
HANDLE_LDAC hData,
int *p_error_code)
{
    *p_error_code = hData->error_code;

    return LDAC_S_OK;
}

/***************************************************************************************************
    Clear Error Code at Internal Block Level
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_clear_internal_error_code(
HANDLE_LDAC hData)
{
    hData->sfinfo.error_code = LDAC_ERR_NONE;

    return LDAC_S_OK;
}

/***************************************************************************************************
    Get Error Code at Internal Block Level
***************************************************************************************************/
DECLSPEC LDAC_RESULT ldaclib_get_internal_error_code(
HANDLE_LDAC hData,
int *p_error_code)
{
    *p_error_code = hData->sfinfo.error_code;

    return LDAC_S_OK;
}

