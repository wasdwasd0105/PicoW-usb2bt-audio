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

#ifndef _PROTO_LDAC_H
#define _PROTO_LDAC_H

/***************************************************************************************************
    Function Declarations
***************************************************************************************************/
/* encode_ldac.c */
DECLFUNC LDAC_RESULT init_encode_ldac(SFINFO *);
DECLFUNC void calc_initial_bits_ldac(SFINFO *);
DECLFUNC void free_encode_ldac(SFINFO *);
DECLFUNC int encode_ldac(SFINFO *, int, int, int, int, int, int, int);


/* setpcm_ldac.c */
DECLFUNC void set_input_pcm_ldac(SFINFO *, char *[], LDAC_SMPL_FMT_T, int);

/* mdct_ldac.c */
DECLFUNC void proc_mdct_ldac(SFINFO *, int);


/* sigana_ldac.c */
DECLFUNC int ana_frame_status_ldac(SFINFO *, int);

/* bitalloc_ldac.c */
DECLFUNC int alloc_bits_ldac(AB *);

/* bitalloc_sub_ldac.c */
DECLFUNC int encode_side_info_ldac(AB *);
DECLFUNC void calc_add_word_length_ldac(AC *);

/* quant_ldac.c */
DECLFUNC void norm_spectrum_ldac(AC *);
DECLFUNC void quant_spectrum_ldac(AC *);
DECLFUNC void quant_residual_ldac(AC *);


/* pack_ldac.c */
DECLFUNC void pack_frame_header_ldac(int, int, int, int, STREAM *);
DECLFUNC int pack_raw_data_frame_ldac(SFINFO *, STREAM *, int *, int *);
DECLFUNC int pack_null_data_frame_ldac(SFINFO *, STREAM *, int *, int *);


/* tables_ldac.c */
DECLFUNC int get_block_nchs_ldac(int);

/* tables_sigproc_ldac.c */
DECLFUNC void set_mdct_table_ldac(int);

/* memory_ldac.c */
DECLFUNC size_t align_ldac(size_t);
DECLFUNC void *calloc_ldac(SFINFO *, size_t, size_t);

#endif /* _PROTO_LDAC_H */

