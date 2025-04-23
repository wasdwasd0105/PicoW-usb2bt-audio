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

#ifndef _LDAC_H
#define _LDAC_H

#include "ldaclib.h"
#include "struct_ldac.h"





/***************************************************************************************************
    Macro Definitions
***************************************************************************************************/
/* Configuration */
#define LDAC_SYNCWORDBITS      8
#define LDAC_SYNCWORD       0xAA
/** Sampling Rate **/
#define LDAC_SMPLRATEBITS      3
#define LDAC_NSMPLRATEID       6
#define LDAC_NSUPSMPLRATEID    4
#define LDAC_SMPLRATEID_0    0x0
#define LDAC_SMPLRATEID_1    0x1
#define LDAC_SMPLRATEID_2    0x2
#define LDAC_SMPLRATEID_3    0x3
/** Channel **/
#define LDAC_CHCONFIG1BITS     3
#define LDAC_CHCONFIG2BITS     2
#define LDAC_NCHCONFIGID       8
#define LDAC_MAXNCH            2
#define LDAC_CHANNEL_1CH       1
#define LDAC_CHANNEL_2CH       2
#define LDAC_CHCONFIGID_MN     0
#define LDAC_CHCONFIGID_DL     1
#define LDAC_CHCONFIGID_ST     2
/** Frame Length **/
#define LDAC_FRAMELEN1BITS    11
#define LDAC_FRAMELEN2BITS     9
#define LDAC_MAXNBYTES      1024
#define LDAC_MAXSUPNBYTES    512
#define LDAC_MINSUPNBYTES     22
/** Frame Status **/
#define LDAC_FRAMESTATBITS     2
#define LDAC_FRMSTAT_LEV_0     0
#define LDAC_FRMSTAT_LEV_1     1
#define LDAC_FRMSTAT_LEV_2     2
#define LDAC_FRMSTAT_LEV_3     3
/** Other **/
#define LDAC_RESERVE1BITS      2
#define LDAC_RESERVE2BITS      5
#define LDAC_DUMMYCODE      0x00

/* Signal Processing */
#define LDAC_NFRAME            2
#define LDAC_NSFTSTEP          5
/** Frame Samples (log base 2 of) **/
#define LDAC_NUMLNN            2
#define LDAC_MAXLNN            8
#define LDAC_2FSLNN            8
#define LDAC_1FSLNN            7
/** Frame Samples **/
#define LDAC_MAXLSU (1<<LDAC_MAXLNN)
#define LDAC_2FSLSU (1<<LDAC_2FSLNN)
#define LDAC_1FSLSU (1<<LDAC_1FSLNN)
/** Band **/
#define LDAC_MAXNBANDS        16
#define LDAC_2FSNBANDS        16
#define LDAC_1FSNBANDS        12
/** QU **/
#define LDAC_MAXGRADQU        50
#define LDAC_MAXNQUS          34
#define LDAC_2FSNQUS          34
#define LDAC_1FSNQUS          26
#define LDAC_MAXNSPS          16
/** Frame Status Analysis **/
#define LDAC_NSP_PSEUDOANA   128
#define LDAC_NSP_LOWENERGY    12
#define LDAC_TH_ZCROSNUM      90
#define LDAC_MAXCNT_FRMANA    10

/* Stream Syntax */
#define LDAC_BLKID_MONO        0
#define LDAC_BLKID_STEREO      1
#define LDAC_FILLCODE       0x01
/** Band Info **/
#define LDAC_NBANDBITS         4
#define LDAC_BAND_OFFSET       2
/** Gradient Data **/
#define LDAC_GRADMODEBITS      2
#define LDAC_GRADOSBITS        5
#define LDAC_MAXGRADOS        31
#define LDAC_DEFGRADOSH       31
#define LDAC_GRADQU0BITS       6
#define LDAC_GRADQU1BITS       5
#define LDAC_DEFGRADQUH       26
#define LDAC_NADJQUBITS        5
#define LDAC_MAXNADJQUS       32
/** Scale Factor Data **/
#define LDAC_IDSFBITS          5
#define LDAC_NIDSF            32
#define LDAC_SFCMODEBITS       1
#define LDAC_NSFCMODE          2
#define LDAC_SFCWTBLBITS       3
#define LDAC_NSFCWTBL          8
#define LDAC_SFCBLENBITS       2
#define LDAC_MINSFCBLEN_0      3
#define LDAC_MAXSFCBLEN_0      6
#define LDAC_MINSFCBLEN_1      2
#define LDAC_MAXSFCBLEN_1      5
#define LDAC_MINSFCBLEN_2      2
#define LDAC_MAXSFCBLEN_2      5
/** Spectrum/Residual Data **/
#define LDAC_NIDWL            16
#define LDAC_MINIDWL1          1
#define LDAC_MAXIDWL1         15
#define LDAC_MAXIDWL2         15
#define LDAC_2DIMSPECBITS      3
#define LDAC_N2DIMSPECENCTBL  16
#define LDAC_N2DIMSPECDECTBL   8
#define LDAC_4DIMSPECBITS      7
#define LDAC_N4DIMSPECENCTBL 256
#define LDAC_N4DIMSPECDECTBL  81
/** Bit Operation **/
#define LDAC_LOC_SHIFT         3
#define LDAC_LOC_MASK        0x7
#define LDAC_BYTESIZE          8
#define LDAC_MAXBITNUM      8192

/* Flag */
#define LDAC_FLAGBITS          1
#define LDAC_TRUE              1
#define LDAC_FALSE             0

/* Mode */
#define LDAC_MODE_0            0
#define LDAC_MODE_1            1
#define LDAC_MODE_2            2
#define LDAC_MODE_3            3

/***************************************************************************************************
    Structure Definitions
***************************************************************************************************/
typedef struct _sfinfo_ldac SFINFO;
typedef struct _config_info_ldac CFG;
typedef struct _audio_block_ldac AB;
typedef struct _audio_channel_ldac AC;
typedef struct _audio_channel_sub_ldac ACSUB;

/* Configuration Information Structure */
struct _config_info_ldac {
    int syncword;
    int smplrate_id;
    int chconfig_id;
    int ch;
    int frame_length;
    int frame_status;
};

/* Audio Channel (AC) Sub Structure */
#ifndef _32BIT_FIXED_POINT
struct _audio_channel_sub_ldac {
    SCALAR a_time[LDAC_MAXLSU*LDAC_NFRAME];
    SCALAR a_spec[LDAC_MAXLSU];
};
#else /* _32BIT_FIXED_POINT */
struct _audio_channel_sub_ldac {
    INT32 a_time[LDAC_MAXLSU*LDAC_NFRAME];
    INT32 a_spec[LDAC_MAXLSU];
};
#endif /* _32BIT_FIXED_POINT */

/* Audio Channel (AC) Structure */
struct _audio_channel_ldac {
    int ich;
    int frmana_cnt;
    int sfc_mode;
    int sfc_bitlen;
    int sfc_offset;
    int sfc_weight;
    int a_idsf[LDAC_MAXNQUS];
    int a_idwl1[LDAC_MAXNQUS];
    int a_idwl2[LDAC_MAXNQUS];
    int a_addwl[LDAC_MAXNQUS];
    int a_tmp[LDAC_MAXNQUS];
    int a_qspec[LDAC_MAXLSU];
    int a_rspec[LDAC_MAXLSU];
    AB *p_ab;
    ACSUB *p_acsub;
};

/* Audio Block (AB) Structure */
struct _audio_block_ldac {
    int blk_type;
    int blk_nchs;
    int nbands;
    int nqus;
    int grad_mode;
    int grad_qu_l;
    int grad_qu_h;
    int grad_os_l;
    int grad_os_h;
    int a_grad[LDAC_MAXGRADQU];
    int nadjqus;
    int abc_status;
    int nbits_ab;
    int nbits_band;
    int nbits_grad;
    int nbits_scfc;
    int nbits_spec;
    int nbits_avail;
    int nbits_used;
    int *p_smplrate_id;
    int *p_error_code;
    AC  *ap_ac[2];
};

/* Sound Frame Structure */
struct _sfinfo_ldac {
    CFG cfg;
    AB *p_ab;
    AC *ap_ac[LDAC_MAXNCH];
    char *p_mempos;
    int error_code;
};

/* LDAC Handle */
typedef struct _handle_ldac_struct {
    int nlnn;
    int nbands;
    int grad_mode;
    int grad_qu_l;
    int grad_qu_h;
    int grad_os_l;
    int grad_os_h;
    int abc_status;
    int error_code;
    SFINFO sfinfo;
} HANDLE_LDAC_STRUCT;

/* Huffman Codeword */
typedef struct {
    unsigned char word;
    unsigned char len;
} HC;

/* Huffman Encoding Structure */
typedef struct _hcenc_ldac HCENC;
struct _hcenc_ldac {
    const HC *p_tbl;
    unsigned char ncodes;
    unsigned char wl;
    unsigned char mask;
};


/*******************************************************************************
    Function Declarations
*******************************************************************************/
#define npow2_ldac(n)  (1 << (n))
#define min_ldac(a, b) (((a)<(b)) ? (a) : (b))
#define max_ldac(a, b) (((a)>(b)) ? (a) : (b))

/* Get Huffman Codeword Property */
#define hc_len_ldac(p)  ((p)->len)
#define hc_word_ldac(p) ((p)->word)

/* Convert a Signed Number with nbits to a Signed Integer */
#define bs_to_int_ldac(bs, nbits) (((bs)&(0x1<<((nbits)-1))) ? ((bs)|((~0x0)<<(nbits))) : bs)

#ifdef _32BIT_FIXED_POINT
#include "fixp_ldac.h"
#endif /* _32BIT_FIXED_POINT */
#include "proto_ldac.h"


#endif /* _LDAC_H */

