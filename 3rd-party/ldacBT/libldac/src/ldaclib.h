/*
 * Copyright (C) 2013 - 2016 Sony Corporation
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

#ifndef _LDACLIB_H
#define _LDACLIB_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************************************
    Macro Definitions
***************************************************************************************************/
#define LDAC_HOST_ENDIAN_LITTLE

#define LDAC_PRCNCH        2
#define LDAC_FRMHDRBYTES   3
#define LDAC_CONFIGBYTES   4

typedef int LDAC_RESULT;
#define LDAC_S_OK          ((LDAC_RESULT)0x00000000L)
#define LDAC_S_FALSE       ((LDAC_RESULT)0x00000001L)
#define LDAC_E_UNEXPECTED  ((LDAC_RESULT)0x8000FFFFL)
#define LDAC_E_OUTOFMEMORY ((LDAC_RESULT)0x8007000EL)
#define LDAC_E_INVALIDARG  ((LDAC_RESULT)0x80070057L)
#define LDAC_E_FAIL        ((LDAC_RESULT)0x80004005L)
#define LDAC_E_NOTIMPL     ((LDAC_RESULT)0x80004001L)

#define LDAC_SUCCEEDED(result) ((LDAC_RESULT)(result)>=0)
#define LDAC_FAILED(result)    ((LDAC_RESULT)(result)<0)

#define DECLSPEC

typedef struct _handle_ldac_struct *HANDLE_LDAC;

typedef enum {
    LDAC_SMPL_FMT_NONE,
    LDAC_SMPL_FMT_S08,
    LDAC_SMPL_FMT_S16,
    LDAC_SMPL_FMT_S24,
    LDAC_SMPL_FMT_S32,
    LDAC_SMPL_FMT_F32,
    LDAC_SMPL_FMT_NUM,
    LDAC_SMPL_FMT_MAX = 0x7fffffff
} LDAC_SMPL_FMT_T;

/***************************************************************************************************
    Function Declarations
***************************************************************************************************/
/* Common API Functions */
DECLSPEC int ldaclib_get_version(void);
DECLSPEC int ldaclib_get_major_version(void);
DECLSPEC int ldaclib_get_minor_version(void);
DECLSPEC int ldaclib_get_branch_version(void);

DECLSPEC LDAC_RESULT ldaclib_get_sampling_rate_index(int, int *);
DECLSPEC LDAC_RESULT ldaclib_get_sampling_rate(int, int *);
DECLSPEC LDAC_RESULT ldaclib_get_frame_samples(int, int *);
DECLSPEC LDAC_RESULT ldaclib_get_nlnn(int, int *);
DECLSPEC LDAC_RESULT ldaclib_get_channel(int, int *);
DECLSPEC LDAC_RESULT ldaclib_get_channel_config_index(int, int *);
DECLSPEC LDAC_RESULT ldaclib_check_nlnn_shift(int, int);

DECLSPEC HANDLE_LDAC ldaclib_get_handle(void);
DECLSPEC LDAC_RESULT ldaclib_free_handle(HANDLE_LDAC);

DECLSPEC LDAC_RESULT ldaclib_set_config_info(HANDLE_LDAC, int, int, int, int);
DECLSPEC LDAC_RESULT ldaclib_get_config_info(HANDLE_LDAC, int *, int *, int *, int *);
DECLSPEC LDAC_RESULT ldaclib_set_frame_header(HANDLE_LDAC, unsigned char *, int, int, int, int);

/* Encoder API Functions */
DECLSPEC LDAC_RESULT ldaclib_get_encode_setting(int, int, int *, int *, int *, int *, int *, int *, int *);
DECLSPEC LDAC_RESULT ldaclib_set_encode_frame_length(HANDLE_LDAC, int);
DECLSPEC LDAC_RESULT ldaclib_get_encode_frame_length(HANDLE_LDAC, int *);
DECLSPEC LDAC_RESULT ldaclib_set_encode_info(HANDLE_LDAC, int, int, int, int, int, int, int);
DECLSPEC LDAC_RESULT ldaclib_init_encode(HANDLE_LDAC);
DECLSPEC LDAC_RESULT ldaclib_free_encode(HANDLE_LDAC);
DECLSPEC LDAC_RESULT ldaclib_encode(HANDLE_LDAC, char *[], LDAC_SMPL_FMT_T, unsigned char *, int *);
DECLSPEC LDAC_RESULT ldaclib_flush_encode(HANDLE_LDAC, LDAC_SMPL_FMT_T, unsigned char *, int *);


/* Error Code Dispatch */
DECLSPEC LDAC_RESULT ldaclib_get_error_code(HANDLE_LDAC, int *);
DECLSPEC LDAC_RESULT ldaclib_get_internal_error_code(HANDLE_LDAC, int *);
DECLSPEC LDAC_RESULT ldaclib_clear_error_code(HANDLE_LDAC);
DECLSPEC LDAC_RESULT ldaclib_clear_internal_error_code(HANDLE_LDAC);

/***************************************************************************************************
    Error Code Definitions
***************************************************************************************************/
#define LDAC_ERR_NONE                       0

/* Non Fatal Error */
#define LDAC_ERR_NON_FATAL                  1

/* Non Fatal Error (Block Level) */
#define LDAC_ERR_BIT_ALLOCATION             5

/* Non Fatal Error (Handle Level) */
#define LDAC_ERR_NOT_IMPLEMENTED          128
#define LDAC_ERR_NON_FATAL_ENCODE         132

/* Fatal Error */
#define LDAC_ERR_FATAL                    256

/* Fatal Error (Block Level) */
#define LDAC_ERR_SYNTAX_BAND              260
#define LDAC_ERR_SYNTAX_GRAD_A            261
#define LDAC_ERR_SYNTAX_GRAD_B            262
#define LDAC_ERR_SYNTAX_GRAD_C            263
#define LDAC_ERR_SYNTAX_GRAD_D            264
#define LDAC_ERR_SYNTAX_GRAD_E            265
#define LDAC_ERR_SYNTAX_IDSF              266
#define LDAC_ERR_SYNTAX_SPEC              267

#define LDAC_ERR_BIT_PACKING              280

#define LDAC_ERR_ALLOC_MEMORY             300

/* Fatal Error (Handle Level) */
#define LDAC_ERR_FATAL_HANDLE             512

#define LDAC_ERR_ILL_SYNCWORD             516
#define LDAC_ERR_ILL_SMPL_FORMAT          517

#define LDAC_ERR_ASSERT_SAMPLING_RATE     530
#define LDAC_ERR_ASSERT_SUP_SAMPLING_RATE 531
#define LDAC_ERR_CHECK_SAMPLING_RATE      532
#define LDAC_ERR_ASSERT_CHANNEL_CONFIG    533
#define LDAC_ERR_CHECK_CHANNEL_CONFIG     534
#define LDAC_ERR_ASSERT_FRAME_LENGTH      535
#define LDAC_ERR_ASSERT_SUP_FRAME_LENGTH  536
#define LDAC_ERR_ASSERT_FRAME_STATUS      537
#define LDAC_ERR_ASSERT_NSHIFT            538

#define LDAC_ERR_ENC_INIT_ALLOC           550
#define LDAC_ERR_ENC_ILL_GRADMODE         551
#define LDAC_ERR_ENC_ILL_GRADPAR_A        552
#define LDAC_ERR_ENC_ILL_GRADPAR_B        553
#define LDAC_ERR_ENC_ILL_GRADPAR_C        554
#define LDAC_ERR_ENC_ILL_GRADPAR_D        555
#define LDAC_ERR_ENC_ILL_NBANDS           556
#define LDAC_ERR_PACK_BLOCK_FAILED        557

#define LDAC_ERR_DEC_INIT_ALLOC           570
#define LDAC_ERR_INPUT_BUFFER_SIZE        571
#define LDAC_ERR_UNPACK_BLOCK_FAILED      572
#define LDAC_ERR_UNPACK_BLOCK_ALIGN       573
#define LDAC_ERR_UNPACK_FRAME_ALIGN       574
#define LDAC_ERR_FRAME_LENGTH_OVER        575
#define LDAC_ERR_FRAME_ALIGN_OVER         576

#define LDAC_ERR_FRAME_LIMIT              998
#define LDAC_ERR_TIME_EXPIRED             999

#define LDAC_ERROR(err)       ((LDAC_ERR_NON_FATAL) <= (err) ? 1 : 0)
#define LDAC_FATAL_ERROR(err) ((LDAC_ERR_FATAL) <= (err) ? 1 : 0)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _LDACLIB_H */

