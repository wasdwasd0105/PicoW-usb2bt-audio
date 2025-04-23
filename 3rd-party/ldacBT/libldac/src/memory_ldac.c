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
    Align Memory
***************************************************************************************************/
#define LDAC_ALLOC_LINE 8

DECLFUNC size_t align_ldac(
size_t size)
{
    if (LDAC_ALLOC_LINE != 0) {
        size = (((size-1)/LDAC_ALLOC_LINE)+1) * LDAC_ALLOC_LINE;
    }

    return size;
}

/***************************************************************************************************
    Clear Allocate Memory
***************************************************************************************************/
DECLFUNC void *calloc_ldac(
SFINFO *p_sfinfo,
size_t nmemb,
size_t size)
{
    char *p_tmp;

    if (p_sfinfo->p_mempos != (char *)NULL) {
        p_tmp = p_sfinfo->p_mempos;
        p_sfinfo->p_mempos += nmemb * align_ldac(size);
    }
    else {
        p_tmp = calloc(nmemb, size);
    }

    return (void *)p_tmp;
}

