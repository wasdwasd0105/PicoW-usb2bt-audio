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


#define LDAC_TH_LOWENERGY_L _scalar(225.47)
#define LDAC_TH_LOWENERGY_M _scalar(897.61)
#define LDAC_TH_LOWENERGY_H _scalar(3573.44)

#define LDAC_TH_CENTROID    _scalar(45.0)
#define LDAC_TH_ZERODIV     _scalar(1.0e-6)

/***************************************************************************************************
    Calculate Pseudo Spectrum and Low Band Energy
***************************************************************************************************/
static SCALAR calc_mdct_pseudo_spectrum_ldac(
SCALAR *p_spec,
SCALAR *p_psd,
int n)
{
    int isp;
    SCALAR low_energy, tmp;
    SCALAR y0, y1, y2;

    {
        y1 = p_spec[0];
        y2 = p_spec[1];
        tmp = y1 * y1 + y2 * y2;
        low_energy = tmp;
        p_psd[0] = sqrt(tmp);
    }

    for (isp = 1; isp < LDAC_NSP_LOWENERGY; isp++) {
        y0 = y1;
        y1 = y2;
        y2 = p_spec[isp+1];
        tmp = y1 * y1 + (y0-y2) * (y0-y2);
        low_energy += tmp;
        p_psd[isp] = sqrt(tmp);
    }

    for (isp = LDAC_NSP_LOWENERGY; isp < n-1; isp++) {
        y0 = y1;
        y1 = y2;
        y2 = p_spec[isp+1];
        tmp = y1 * y1 + (y0-y2) * (y0-y2);
        p_psd[isp] = sqrt(tmp);
    }

    {
        tmp = y1 * y1 + y2 * y2;
        p_psd[n-1] = sqrt(tmp);
    }

    return low_energy;
}

/***************************************************************************************************
    Calculate Pseudo Spectrum Centroid
***************************************************************************************************/
static SCALAR calc_spectral_centroid_ldac(
SCALAR *p_spec,
int nsp)
{
    int isp;
    SCALAR centroid;
    SCALAR s1, s2;

    s1 = s2 = _scalar(0.0);
    for (isp = 0; isp < nsp; isp++) {
        s1 += (SCALAR)isp * *p_spec;
        s2 += *p_spec++;
    }

    if (s2 < LDAC_TH_ZERODIV) {
        centroid = _scalar(0.0);
    }
    else {
        centroid = s1 / s2;
    }

    return centroid;
}

/***************************************************************************************************
    Calculate Number of Zero Cross
***************************************************************************************************/
static int calc_zero_cross_number_ldac(
SCALAR *p_time,
int n)
{
    int i;
    int zero_cross = 0;
    SCALAR prev;

    prev = _scalar(0.0);
    for (i = 0; i < n; i++) {
        if (prev * *p_time < _scalar(0.0)) {
            zero_cross++;
        }
        prev = *p_time++;
    }

    return zero_cross;
}

/***************************************************************************************************
    Analyze Frame Status
***************************************************************************************************/
DECLSPEC int ana_frame_status_ldac(
SFINFO *p_sfinfo,
int nlnn)
{
    AC *p_ac;
    int ich;
    int nchs = p_sfinfo->cfg.ch;
    int nsmpl = npow2_ldac(nlnn+1);
    int cnt, zero_cross;
    int a_status[LDAC_PRCNCH];
    SCALAR low_energy, centroid;
    SCALAR a_psd_spec[LDAC_NSP_PSEUDOANA];

    for (ich = 0; ich < nchs; ich++) {
        p_ac = p_sfinfo->ap_ac[ich];

        low_energy = calc_mdct_pseudo_spectrum_ldac(p_ac->p_acsub->a_spec, a_psd_spec, LDAC_NSP_PSEUDOANA);

        centroid = calc_spectral_centroid_ldac(a_psd_spec, LDAC_NSP_PSEUDOANA);

        zero_cross = calc_zero_cross_number_ldac(p_ac->p_acsub->a_time, nsmpl);

        a_status[ich] = LDAC_FRMSTAT_LEV_0;
        if (low_energy < LDAC_TH_LOWENERGY_L) { 
            a_status[ich] = LDAC_FRMSTAT_LEV_3;
        }
        else {
            if (low_energy < LDAC_TH_LOWENERGY_M) {
                a_status[ich] = LDAC_FRMSTAT_LEV_2;
            }
            else if (low_energy < LDAC_TH_LOWENERGY_H) {
                a_status[ich] = LDAC_FRMSTAT_LEV_1;
            }

            cnt = p_ac->frmana_cnt;
            if ((centroid > LDAC_TH_CENTROID) && (zero_cross >= LDAC_TH_ZCROSNUM)) {
                cnt++;

                if (cnt >= LDAC_MAXCNT_FRMANA) {
                    cnt = LDAC_MAXCNT_FRMANA;
                    a_status[ich] = LDAC_FRMSTAT_LEV_2;
                }
                else if (a_status[ich] <= LDAC_FRMSTAT_LEV_1) {
                    a_status[ich]++;
                }
            }
            else {
                cnt = 0;
            }
            p_ac->frmana_cnt = cnt;
        }
    }

    if (nchs == LDAC_CHANNEL_1CH) {
        return a_status[0];
    }
    else {
        return min_ldac(a_status[0], a_status[1]);
    }
}

