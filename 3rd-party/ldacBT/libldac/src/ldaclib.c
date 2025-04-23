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


#include "ldac.h"

/* Common Files */
#include "tables_ldac.c"
#ifndef _32BIT_FIXED_POINT
#include "tables_sigproc_ldac.c"
#include "setpcm_ldac.c"
#else /* _32BIT_FIXED_POINT */
#include "tables_sigproc_fixp_ldac.c"
#include "setpcm_fixp_ldac.c"
#include "func_fixp_ldac.c"
#endif /* _32BIT_FIXED_POINT */
#include "bitalloc_sub_ldac.c"
#include "memory_ldac.c"
#include "ldaclib_api.c"

/* Encoder Files */
#ifndef _32BIT_FIXED_POINT
#include "mdct_ldac.c"
#include "sigana_ldac.c"
#include "quant_ldac.c"
#else /* _32BIT_FIXED_POINT */
#include "mdct_fixp_ldac.c"
#include "sigana_fixp_ldac.c"
#include "quant_fixp_ldac.c"
#endif /* _32BIT_FIXED_POINT */
#include "bitalloc_ldac.c"
#include "pack_ldac.c"
#include "encode_ldac.c"


