/*
 * Copyright (C) 2013 - 2017 Sony Corporation
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

#include "ldacBT_internal.h"


/* Get LDAC library version */
#define LDACBT_LIB_VER_MAJOR   2
#define LDACBT_LIB_VER_MINOR   0
#define LDACBT_LIB_VER_BRANCH  2
LDACBT_API int ldacBT_get_version( void )
{
    return ((LDACBT_LIB_VER_MAJOR)<<16)|((LDACBT_LIB_VER_MINOR)<<8)|(LDACBT_LIB_VER_BRANCH);
}

/* Get LDAC handle */
LDACBT_API HANDLE_LDAC_BT ldacBT_get_handle( void )
{
    HANDLE_LDAC_BT hLdacBT;
    hLdacBT = (HANDLE_LDAC_BT)malloc( sizeof(STRUCT_LDACBT_HANDLE) );
    if( hLdacBT == NULL ){ return NULL; }

    /* Get ldaclib Handler */
    if( (hLdacBT->hLDAC = ldaclib_get_handle()) == NULL ){
        ldacBT_free_handle( hLdacBT );
        return NULL;
    }

    ldacBT_param_clear( hLdacBT );
    return hLdacBT;
}

/* Free LDAC handle */
LDACBT_API void ldacBT_free_handle( HANDLE_LDAC_BT hLdacBT )
{
    if( hLdacBT == NULL ){ return; }

    if( hLdacBT->hLDAC != NULL ){
        /* close ldaclib handle */
        if( hLdacBT->proc_mode == LDACBT_PROCMODE_ENCODE ){
            ldaclib_free_encode( hLdacBT->hLDAC );
        }
        /* free ldaclib handle */
        ldaclib_free_handle( hLdacBT->hLDAC );
        hLdacBT->hLDAC = NULL;
    }
    /* free ldacbt handle */
    free( hLdacBT );
}

/* Close LDAC handle */
LDACBT_API void ldacBT_close_handle( HANDLE_LDAC_BT hLdacBT )
{
    if( hLdacBT == NULL ){ return; }

    if( hLdacBT->hLDAC != NULL ){
        /* close ldaclib handle */
        if( hLdacBT->proc_mode == LDACBT_PROCMODE_ENCODE ){
            ldaclib_free_encode( hLdacBT->hLDAC );
        }
        /* clear error code */
        ldaclib_clear_error_code(hLdacBT->hLDAC);
        ldaclib_clear_internal_error_code(hLdacBT->hLDAC);
    }
    /* clear ldacbt handle */
    ldacBT_param_clear( hLdacBT );
}


/* Get ERROR CODE */
LDACBT_API int ldacBT_get_error_code( HANDLE_LDAC_BT hLdacBT )
{
    int error_code;
    if( hLdacBT == NULL ){return LDACBT_ERR_FATAL_HANDLE<<10;}
    ldacBT_check_ldaclib_error_code( hLdacBT );
    if( hLdacBT->error_code_api == LDACBT_GET_LDACLIB_ERROR_CODE ){
        error_code = LDACBT_ERR_FATAL << 20 | hLdacBT->error_code;
    }else if( hLdacBT->error_code_api != LDACBT_ERR_NONE ){
        error_code = hLdacBT->error_code_api << 20 | hLdacBT->error_code;
    }else{
        error_code = hLdacBT->error_code_api << 20;
    }
    return error_code;
}


/* Get Configured Sampling frequency */
LDACBT_API int ldacBT_get_sampling_freq( HANDLE_LDAC_BT hLdacBT )
{
    if( hLdacBT == NULL ){
        return LDACBT_E_FAIL;
    }
    if( hLdacBT->proc_mode != LDACBT_PROCMODE_ENCODE )
    {
        hLdacBT->error_code_api = LDACBT_ERR_HANDLE_NOT_INIT;
        return LDACBT_E_FAIL;
    }
    return hLdacBT->pcm.sf;
}

/* Get bitrate */
LDACBT_API int  ldacBT_get_bitrate( HANDLE_LDAC_BT hLdacBT )
{
    if( hLdacBT == NULL ){
        return LDACBT_E_FAIL;
    }
    if( hLdacBT->proc_mode != LDACBT_PROCMODE_ENCODE )
    {
        hLdacBT->error_code_api = LDACBT_ERR_HANDLE_NOT_INIT;
        return LDACBT_E_FAIL;
    }
    return hLdacBT->bitrate;
}

/* Init LDAC handle for ENCODE */
LDACBT_API int ldacBT_init_handle_encode( HANDLE_LDAC_BT hLdacBT, int mtu, int eqmid,
                                      int cm, LDACBT_SMPL_FMT_T fmt, int sf )
{
    LDAC_RESULT result;
    int sfid, frame_samples, cci;
    int nbasebands, grad_mode, grad_qu_l, grad_qu_h, grad_ofst_l, grad_ofst_h, abc_flag;
    P_LDACBT_CONFIG pCfg;
    const int a_cci_nch[] = { 1, 2, 2 };

    /* check arguments */
    if( hLdacBT == NULL ){ return LDACBT_E_FAIL; }
    if( (hLdacBT->error_code_api = ldacBT_assert_mtu( mtu )) != LDACBT_ERR_NONE ){
        return LDACBT_E_FAIL;
    }
    if( (hLdacBT->error_code_api = ldacBT_assert_eqmid( eqmid )) != LDACBT_ERR_NONE ){
        return LDACBT_E_FAIL;
    }
    if( (hLdacBT->error_code_api = ldacBT_assert_cm( cm )) != LDACBT_ERR_NONE ){
        return LDACBT_E_FAIL;
    }
    if( (hLdacBT->error_code_api = ldacBT_assert_sample_format( fmt )) != LDACBT_ERR_NONE ){
        return LDACBT_E_FAIL;
    }
    if( (hLdacBT->error_code_api = ldacBT_assert_pcm_sampling_freq( sf )) != LDACBT_ERR_NONE ){
        return LDACBT_E_FAIL;
    }

    ldacBT_close_handle( hLdacBT );

    /* initialize handle for encode processing */
    hLdacBT->proc_mode = LDACBT_PROCMODE_ENCODE;
    hLdacBT->flg_encode_flushed = FALSE;

    /* transport setting */
    /* The ldac frame header is REQUIRED for A2DP streaming. */
    hLdacBT->transport = TRUE;
    hLdacBT->tx.mtu = mtu;
    hLdacBT->tx.pkt_hdr_sz = LDACBT_TX_HEADER_SIZE;
    hLdacBT->tx.tx_size = LDACBT_MTU_REQUIRED;
    hLdacBT->tx.pkt_type = _2_DH5;
    /* - BT TRANS HEADER etc */
    hLdacBT->tx.tx_size -= hLdacBT->tx.pkt_hdr_sz;
    if( hLdacBT->tx.tx_size > (hLdacBT->tx.mtu - hLdacBT->tx.pkt_hdr_sz) ){
        /* never happen, mtu must be larger than LDACBT_MTU_REQUIRED(2DH5) */
        hLdacBT->tx.tx_size = (hLdacBT->tx.mtu - hLdacBT->tx.pkt_hdr_sz);
    }

    /* channel configration */
    cci = ldacBT_cm_to_cci(cm);
    hLdacBT->cm = cm;
    hLdacBT->cci = cci;
    /* input pcm configuration */
    hLdacBT->pcm.ch = a_cci_nch[cci];
    hLdacBT->pcm.sf = sf;
    hLdacBT->pcm.fmt = fmt;
    switch(hLdacBT->pcm.fmt){
      case LDACBT_SMPL_FMT_S16:
        hLdacBT->pcm.wl = 2;
        break;
      case LDACBT_SMPL_FMT_S24:
        hLdacBT->pcm.wl = 3;
        break;
      case LDACBT_SMPL_FMT_S32:
      case LDACBT_SMPL_FMT_F32:
        hLdacBT->pcm.wl = 4;
        break;
      default:
        // must be rejected by ldacBT_assert_sample_format()
        hLdacBT->pcm.wl = 4;
        break;
    }

    /* initilize ldac encode */
    /* Get sampling frequency index */
    result = ldaclib_get_sampling_rate_index( hLdacBT->pcm.sf, &sfid );
    if( LDAC_FAILED ( result ) ){
        hLdacBT->error_code_api = LDACBT_ERR_ILL_SAMPLING_FREQ;
        return LDACBT_E_FAIL;
    }
    hLdacBT->sfid = sfid;

    /* Get number of frame samples */
    result = ldaclib_get_frame_samples(sfid, &frame_samples);
    if (LDAC_FAILED(result)) {
        hLdacBT->error_code_api = LDACBT_ERR_ILL_SAMPLING_FREQ;
        return LDACBT_E_FAIL;
    }
    hLdacBT->frm_samples = frame_samples;


    /* Set Parameters by Encode Quality Mode Index */
    hLdacBT->eqmid = eqmid;
    /* get frame_length of EQMID */
    pCfg = ldacBT_get_config( hLdacBT->eqmid, hLdacBT->tx.pkt_type );
    /* set frame_length */
    hLdacBT->frmlen_tx = hLdacBT->pcm.ch * pCfg->frmlen_1ch;
    hLdacBT->frmlen = hLdacBT->frmlen_tx;
    if (hLdacBT->transport) {
        /* Adjust frame_length for Transport Header Data */
        hLdacBT->frmlen -= LDACBT_FRMHDRBYTES;
    }

    /* Calculate how many LDAC frames fit into payload packet */
    hLdacBT->tx.nfrm_in_pkt = hLdacBT->tx.tx_size / hLdacBT->frmlen_tx;
    
    
    /* Get ldac encode setting */
    result = ldaclib_get_encode_setting( pCfg->frmlen_1ch, sfid, &nbasebands, &grad_mode,
                     &grad_qu_l, &grad_qu_h, &grad_ofst_l, &grad_ofst_h, &abc_flag);
    if (LDAC_FAILED(result)) {
        hLdacBT->error_code_api = LDACBT_ERR_ILL_PARAM;
        return LDACBT_E_FAIL;
    }

    /* Set Configuration Information */
    result = ldaclib_set_config_info( hLdacBT->hLDAC, hLdacBT->sfid, hLdacBT->cci,
                                      hLdacBT->frmlen, hLdacBT->frm_status);
    if (LDAC_FAILED(result)) {
        hLdacBT->error_code_api = LDACBT_GET_LDACLIB_ERROR_CODE;
        return LDACBT_E_FAIL;
    }
    else if (result != LDAC_S_OK) {
        hLdacBT->error_code_api = LDACBT_GET_LDACLIB_ERROR_CODE;
    }

    /* Set Encoding Information */
    result = ldaclib_set_encode_info(hLdacBT->hLDAC, nbasebands, grad_mode,
                                     grad_qu_l, grad_qu_h, grad_ofst_l, grad_ofst_h, abc_flag);
    if (LDAC_FAILED(result)) {
        hLdacBT->error_code_api = LDACBT_GET_LDACLIB_ERROR_CODE;
        return LDACBT_E_FAIL;
    }
    else if (result != LDAC_S_OK) {
        hLdacBT->error_code_api = LDACBT_GET_LDACLIB_ERROR_CODE;
    }

    /* Initialize ldaclib for Encoding */
    result = ldaclib_init_encode(hLdacBT->hLDAC);
    if (LDAC_FAILED(result)) {
        hLdacBT->error_code_api = LDACBT_GET_LDACLIB_ERROR_CODE;
        return LDACBT_E_FAIL;
    }
    else if (result != LDAC_S_OK) {
        hLdacBT->error_code_api = LDACBT_GET_LDACLIB_ERROR_CODE;
    }

    /* reset target eqmid as current setting */
    hLdacBT->tgt_eqmid = hLdacBT->eqmid;
    hLdacBT->tgt_nfrm_in_pkt = hLdacBT->tx.nfrm_in_pkt;
    hLdacBT->tgt_frmlen = hLdacBT->frmlen;
    hLdacBT->stat_alter_op = LDACBT_ALTER_OP__NON;

    /* get bitrate */
    hLdacBT->bitrate = ldacBT_frmlen_to_bitrate( hLdacBT->frmlen, hLdacBT->transport,
                                                 hLdacBT->pcm.sf, hLdacBT->frm_samples );

    return (hLdacBT->error_code_api==LDACBT_ERR_NONE?LDACBT_S_OK:LDACBT_E_FAIL);
}

/* Set Encode Quality Mode index */
LDACBT_API int ldacBT_set_eqmid( HANDLE_LDAC_BT hLdacBT, int eqmid )
{
    if( hLdacBT == NULL ){
        return LDACBT_E_FAIL;
    }
    if( hLdacBT->proc_mode != LDACBT_PROCMODE_ENCODE ){
        hLdacBT->error_code_api = LDACBT_ERR_HANDLE_NOT_INIT;
        return LDACBT_E_FAIL;
    }

    if( (hLdacBT->error_code_api = ldacBT_assert_eqmid( eqmid )) != LDACBT_ERR_NONE ){
        return LDACBT_E_FAIL; /* fatal */
    }
	ldacBT_set_eqmid_core( hLdacBT, eqmid );

	return LDACBT_S_OK;
}

/* Get Encode Quality Mode index */
LDACBT_API int ldacBT_get_eqmid( HANDLE_LDAC_BT hLdacBT )
{
    if( hLdacBT == NULL ){
        return LDACBT_E_FAIL;
    }
    if( hLdacBT->proc_mode != LDACBT_PROCMODE_ENCODE ){
        hLdacBT->error_code_api = LDACBT_ERR_HANDLE_NOT_INIT;
        return LDACBT_E_FAIL;
    }
    return hLdacBT->tgt_eqmid;
}

/* Alter encode quality mode index */
LDACBT_API int  ldacBT_alter_eqmid_priority( HANDLE_LDAC_BT hLdacBT, int priority )
{
    int target_eqmid;
    if( hLdacBT == NULL ){ return LDACBT_E_FAIL; }
    if( hLdacBT->proc_mode != LDACBT_PROCMODE_ENCODE ){
        hLdacBT->error_code_api = LDACBT_ERR_HANDLE_NOT_INIT;
        return LDACBT_E_FAIL;
    }
    if( (priority != LDACBT_EQMID_INC_QUALITY) &&
        (priority != LDACBT_EQMID_INC_CONNECTION )
        ){
            hLdacBT->error_code_api = LDACBT_ERR_ILL_PARAM;
            return LDACBT_E_FAIL;
    }

    target_eqmid = ldacBT_get_altered_eqmid( hLdacBT,  priority);
    if( target_eqmid < 0 ){
        hLdacBT->error_code_api = LDACBT_ERR_ALTER_EQMID_LIMITED;
        return LDACBT_E_FAIL;
    }

    ldacBT_set_eqmid_core( hLdacBT, target_eqmid );
    return LDACBT_S_OK;
}

/* LDAC encode proccess */
LDACBT_API int ldacBT_encode( HANDLE_LDAC_BT hLdacBT, void *p_pcm, int *pcm_used,
                          unsigned char *p_stream, int *stream_sz, int *frame_num )
{
    LDAC_RESULT result;
    LDACBT_SMPL_FMT_T fmt;
    LDACBT_TRANSPORT_FRM_BUF *ptfbuf;
    LDACBT_PCM_RING_BUF *ppcmring;
    P_LDACBT_CONFIG pCfg;
    int frmlen, frmlen_wrote, frmlen_adj;
    int frm_status, flg_Do_Encode;
    int nFrmToPkt, ch, wl;
    unsigned char *p_ldac_transport_frame;
    unsigned char a_frm_header[LDACBT_FRMHDRBYTES + 2];
    if( hLdacBT == NULL ){
        return LDACBT_E_FAIL;
    }
    if( hLdacBT->hLDAC == NULL ){
        return LDACBT_E_FAIL;
    }
    if( hLdacBT->proc_mode != LDACBT_PROCMODE_ENCODE ){
        hLdacBT->error_code_api = LDACBT_ERR_HANDLE_NOT_INIT;
        return LDACBT_E_FAIL;
    }
    /* Clear Error Codes */
    hLdacBT->error_code_api = LDACBT_ERR_NONE;
    ldaclib_clear_error_code( hLdacBT->hLDAC );
    ldaclib_clear_internal_error_code( hLdacBT->hLDAC );

    if( ( pcm_used == NULL) ||
        ( p_stream == NULL ) ||
        ( stream_sz == NULL ) ||
        ( frame_num == NULL )
        ){
            hLdacBT->error_code_api = LDACBT_ERR_ILL_PARAM;
            return LDACBT_E_FAIL;
    }
    /* reset parameters */
    *pcm_used = 0;
    *stream_sz = 0;
    *frame_num = 0;
    flg_Do_Encode = 0;
    fmt = hLdacBT->pcm.fmt;
    ch = hLdacBT->pcm.ch;
    wl = hLdacBT->pcm.wl;
    ptfbuf = &hLdacBT->ldac_trns_frm_buf;
    ppcmring = &hLdacBT->pcmring;

    /* update input pcm data */
    if( p_pcm != NULL ){
        int nByteCpy, sz;
        nByteCpy = LDACBT_ENC_LSU * wl * ch;
        sz = ppcmring->nsmpl * wl * ch + nByteCpy;
        if( sz < LDACBT_ENC_PCM_BUF_SZ ){
            copy_data_ldac( p_pcm, ppcmring->buf + ppcmring->wp, nByteCpy );
            ppcmring->wp += nByteCpy;
            if( ppcmring->wp >= LDACBT_ENC_PCM_BUF_SZ ){
                ppcmring->wp = 0;
            }
            ppcmring->nsmpl += LDACBT_ENC_LSU;
            *pcm_used = nByteCpy;
        }else{
            /* Not enough space to copy.
             * This will happen when the last encode process failed.
             */
            *pcm_used = 0;
        }

        if( ppcmring->nsmpl >= hLdacBT->frm_samples )
        {
            flg_Do_Encode = 1;
        }
    }else{
        if (hLdacBT->flg_encode_flushed != TRUE){
            flg_Do_Encode = 1;
        }
    }

    if( !flg_Do_Encode ){
        /* nothing to do */
        return LDACBT_S_OK;
    }

    /* update frame_length if needed */
    if( (hLdacBT->tgt_eqmid != UNSET) && (hLdacBT->tgt_eqmid != hLdacBT->eqmid) ){
        if( ptfbuf->nfrm_in == 0 ){
            ldacBT_update_frmlen( hLdacBT, hLdacBT->tgt_frmlen );
            hLdacBT->stat_alter_op = LDACBT_ALTER_OP__NON;
        }
        else if( hLdacBT->tgt_nfrm_in_pkt > hLdacBT->tx.nfrm_in_pkt ){
            /* for better connectivity, apply ASAP */
            if( !hLdacBT->stat_alter_op ){
                nFrmToPkt = hLdacBT->tgt_nfrm_in_pkt - ptfbuf->nfrm_in;
                if( nFrmToPkt > 0 ){
                    pCfg = ldacBT_get_config(LDACBT_EQMID_END, hLdacBT->tx.pkt_type);
                    if( pCfg != NULL ){
                        do{
                            frmlen_adj = (hLdacBT->tx.tx_size - ptfbuf->used) / nFrmToPkt;
                            if( frmlen_adj > hLdacBT->tgt_frmlen ) {
                                frmlen_adj = hLdacBT->tgt_frmlen;
                            }
                            frmlen_adj -= LDACBT_FRMHDRBYTES;
                            if( frmlen_adj >= pCfg->frmlen ){
                                if( ldacBT_update_frmlen( hLdacBT, frmlen_adj ) == LDACBT_S_OK ){
                                    hLdacBT->stat_alter_op = LDACBT_ALTER_OP__ACTIVE;
                                    break;
                                }
                            }
                        }while( --nFrmToPkt > 0 );
                    }
                    if( !hLdacBT->stat_alter_op ){
                        /* force to flash streams */
                        hLdacBT->stat_alter_op = LDACBT_ALTER_OP__FLASH;
                    }
                }
            }
        }
        else{
            /* wait the condition ptfbuf->nfrm_in == 0 for apply new frame_length */
            hLdacBT->stat_alter_op = LDACBT_ALTER_OP__STANDBY;
        }

    }
    else if( hLdacBT->tgt_frmlen != hLdacBT->frmlen ){
        if( ptfbuf->nfrm_in == 0 ){
            ldacBT_update_frmlen( hLdacBT, hLdacBT->tgt_frmlen );
            hLdacBT->stat_alter_op = LDACBT_ALTER_OP__NON;
        }else{
            if( hLdacBT->tgt_nfrm_in_pkt == hLdacBT->tx.nfrm_in_pkt ){
                ldacBT_update_frmlen( hLdacBT, hLdacBT->tgt_frmlen );
                hLdacBT->stat_alter_op = LDACBT_ALTER_OP__NON;
            }else{
                if( hLdacBT->tgt_nfrm_in_pkt > hLdacBT->tx.nfrm_in_pkt ){
                    /* for better connectivity, apply ASAP */
                    if( !hLdacBT->stat_alter_op ){
                        nFrmToPkt = hLdacBT->tgt_nfrm_in_pkt - ptfbuf->nfrm_in;
                        if( nFrmToPkt > 0 ){
                            frmlen_adj = (hLdacBT->tx.tx_size - ptfbuf->used) / nFrmToPkt;
                            if( frmlen_adj > hLdacBT->tgt_frmlen ) {
                                frmlen_adj = hLdacBT->tgt_frmlen;
                            }
                            if( ldacBT_update_frmlen( hLdacBT, frmlen_adj ) == LDACBT_S_OK ){
                                hLdacBT->stat_alter_op = LDACBT_ALTER_OP__ACTIVE;
                            }
                            if( !hLdacBT->stat_alter_op ){
                                /* flash streams */
                                hLdacBT->stat_alter_op = LDACBT_ALTER_OP__FLASH;
                            }
                        }
                    }
                }else{
                    /* wait the condition ptfbuf->nfrm_in == 0 for apply new frame_length */
                    hLdacBT->stat_alter_op = LDACBT_ALTER_OP__STANDBY;
                }
            }
        }
    }

    /* check write space for encoded data */
    ldaclib_get_encode_frame_length( hLdacBT->hLDAC, &frmlen );

    if( (( ptfbuf->used + frmlen + LDACBT_FRMHDRBYTES) > hLdacBT->tx.tx_size) ||
        (hLdacBT->stat_alter_op == LDACBT_ALTER_OP__FLASH) || /* need to flash streams? */
        (( ptfbuf->used + frmlen + LDACBT_FRMHDRBYTES) >= LDACBT_ENC_STREAM_BUF_SZ )
        )
    {
        copy_data_ldac( ptfbuf->buf, p_stream, ptfbuf->used );
        *stream_sz = ptfbuf->used;
        *frame_num = ptfbuf->nfrm_in;
        clear_data_ldac( ptfbuf->buf, sizeof(char)*LDACBT_ENC_STREAM_BUF_SZ);
        ptfbuf->used = 0;
        ptfbuf->nfrm_in = 0;
        if( hLdacBT->stat_alter_op != LDACBT_ALTER_OP__NON ){
            /* update frame length */
            ldacBT_update_frmlen( hLdacBT, hLdacBT->tgt_frmlen );
            hLdacBT->stat_alter_op = LDACBT_ALTER_OP__NON;
        }
    }
    p_ldac_transport_frame = ptfbuf->buf + ptfbuf->used;

    /* Encode Frame */
    if( ppcmring->nsmpl > 0 ){
        char *p_pcm_ring_r;
        int nsmpl_to_clr;
        nsmpl_to_clr = hLdacBT->frm_samples - ppcmring->nsmpl;
        if( nsmpl_to_clr > 0 ){
            int pos, nBytesToZero;
            pos = ppcmring->rp + ppcmring->nsmpl * wl * ch;
            nBytesToZero = nsmpl_to_clr * wl * ch;
            while( nBytesToZero > 0 ){
                int clearBytes;
                clearBytes = nBytesToZero;
                if ( pos + clearBytes >= LDACBT_ENC_PCM_BUF_SZ ){
                    clearBytes = (LDACBT_ENC_PCM_BUF_SZ - pos);
                }
                clear_data_ldac( ppcmring->buf + pos, clearBytes);
                nBytesToZero -= clearBytes;
                if( (pos += clearBytes) >= LDACBT_ENC_PCM_BUF_SZ ){
                    pos = 0;
                }
            }
        }
        p_pcm_ring_r = ppcmring->buf + ppcmring->rp;
        ldacBT_prepare_pcm_encode( p_pcm_ring_r, hLdacBT->pp_pcm, hLdacBT->frm_samples, ch, fmt );
        result = ldaclib_encode(hLdacBT->hLDAC, hLdacBT->pp_pcm, (LDAC_SMPL_FMT_T)fmt,
                         p_ldac_transport_frame+LDACBT_FRMHDRBYTES, &frmlen_wrote);
        if( !LDAC_FAILED(result) ){
            ppcmring->rp += hLdacBT->frm_samples * wl * ch;
            ppcmring->nsmpl -= hLdacBT->frm_samples;
            if( ppcmring->rp >= LDACBT_ENC_PCM_BUF_SZ ){ ppcmring->rp = 0; }
            if( ppcmring->nsmpl < 0 ){ ppcmring->nsmpl = 0; }
        }
    }else{
        result = ldaclib_flush_encode(hLdacBT->hLDAC, (LDAC_SMPL_FMT_T)fmt,
                             p_ldac_transport_frame+LDACBT_FRMHDRBYTES, &frmlen_wrote);
        hLdacBT->flg_encode_flushed = TRUE;
    }

    if( LDAC_FAILED(result) ){
        hLdacBT->error_code_api = LDACBT_GET_LDACLIB_ERROR_CODE;
        return LDACBT_E_FAIL;
    }
    else if( result != LDAC_S_OK ){
        hLdacBT->error_code_api = LDACBT_GET_LDACLIB_ERROR_CODE;
    }

    if( frmlen_wrote > 0 ){
        if( hLdacBT->transport == TRUE ){
            /* Set Frame Header Data */
            clear_data_ldac( a_frm_header, LDACBT_FRMHDRBYTES+2 );
            /* Get Frame Header Information */
            result = ldaclib_get_config_info(hLdacBT->hLDAC, &hLdacBT->sfid, &hLdacBT->cci,
                            &frmlen, &frm_status);
            if( LDAC_FAILED(result) ){
                hLdacBT->error_code_api = LDACBT_GET_LDACLIB_ERROR_CODE;
                return LDACBT_E_FAIL;
            }
            else if (result != LDAC_S_OK) {
                hLdacBT->error_code_api = LDACBT_GET_LDACLIB_ERROR_CODE;
            }

            /* Set Frame Header */
            result = ldaclib_set_frame_header(hLdacBT->hLDAC, a_frm_header, hLdacBT->sfid,
                            hLdacBT->cci, frmlen, frm_status);
            if( LDAC_FAILED(result) ){
                hLdacBT->error_code_api = LDACBT_GET_LDACLIB_ERROR_CODE;
                return LDACBT_E_FAIL;
            }
            else if (result != LDAC_S_OK) {
                hLdacBT->error_code_api = LDACBT_GET_LDACLIB_ERROR_CODE;
            }
            copy_data_ldac( a_frm_header, p_ldac_transport_frame, LDACBT_FRMHDRBYTES );
            frmlen_wrote += LDACBT_FRMHDRBYTES;
        }
        ptfbuf->used += frmlen_wrote;
        ptfbuf->nfrm_in ++;
    }

    /* check for next frame buffer status */
    if( *stream_sz == 0 ){
        if( (( ptfbuf->used + frmlen_wrote) > hLdacBT->tx.tx_size) ||
            (  ptfbuf->nfrm_in >= LDACBT_NFRM_TX_MAX ) || 
            (( ptfbuf->used + frmlen_wrote) >= LDACBT_ENC_STREAM_BUF_SZ ) ||
            ( p_pcm == NULL ) /* flush encode */
            )
        {
            copy_data_ldac( ptfbuf->buf, p_stream, ptfbuf->used );
            *stream_sz = ptfbuf->used;
            *frame_num = ptfbuf->nfrm_in;
            clear_data_ldac( ptfbuf->buf, sizeof(char)*LDACBT_ENC_STREAM_BUF_SZ);
            ptfbuf->used = 0;
            ptfbuf->nfrm_in = 0;
            if( hLdacBT->stat_alter_op != LDACBT_ALTER_OP__NON ){
                ldacBT_update_frmlen( hLdacBT, hLdacBT->tgt_frmlen );
                hLdacBT->stat_alter_op = LDACBT_ALTER_OP__NON;
            }
        }
    }

    return LDACBT_S_OK;
}
