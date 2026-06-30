///  Copyright (c) 2010-2011 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      rfgw_errp_util.c
///  @brief     Utilities for mapping data structures between RFGW-10 and ERRP
///  @author    Yi Tong Tse


#include <stdio.h>
#include <string.h>
#include "common.h"
#include "errp.h"
#include "rfgw_errp.h"


/// Encode RFGW QAM modulation configuration according to ERRP
uint8 rfgw_errp_qam_cfg_modulation (const rfgw_qam_t *rfgw_cfg)
{
    switch (rfgw_cfg->format) {
    case RFGW_MB_DEFAULT_QAM_FORMAT64:
        return ERRP_QAM_CFG_MODULATION_64_QAM;
    case RFGW_MB_DEFAULT_QAM_FORMAT256:
        return ERRP_QAM_CFG_MODULATION_256_QAM;
    default:
        return ERRP_QAM_CFG_ERR;
    }
}


/// Encode RFGW QAM interleaver configuration according to ERRP
uint8 rfgw_errp_qam_cfg_interleaver (const rfgw_qam_t *rfgw_cfg)
{
    switch (rfgw_cfg->fec_i) {
    case 8:
        return (rfgw_cfg->fec_j == 16)?
                   ERRP_QAM_CFG_FEC_I8_J16 : ERRP_QAM_CFG_ERR;
    case 12:
        return (rfgw_cfg->fec_j == 7)?
                   ERRP_QAM_CFG_FEC_I12_J7 : ERRP_QAM_CFG_ERR;
    case 16:
        return (rfgw_cfg->fec_j == 8)?
                   ERRP_QAM_CFG_FEC_I16_J8 : ERRP_QAM_CFG_ERR;
    case 32:
        return (rfgw_cfg->fec_j == 4)?
                   ERRP_QAM_CFG_FEC_I32_J4 : ERRP_QAM_CFG_ERR;
    case 64:
        return (rfgw_cfg->fec_j == 2)?
                   ERRP_QAM_CFG_FEC_I64_J2 : ERRP_QAM_CFG_ERR;
    case 128:
        switch (rfgw_cfg->fec_j) {
        case 1:
            return ERRP_QAM_CFG_FEC_I128_J1;
        case 2:
            return ERRP_QAM_CFG_FEC_I128_J2;
        case 3:
            return ERRP_QAM_CFG_FEC_I128_J3;
        case 4:
            return ERRP_QAM_CFG_FEC_I128_J4;
        case 5:
            return ERRP_QAM_CFG_FEC_I128_J5;
        case 6:
            return ERRP_QAM_CFG_FEC_I128_J6;
        case 7:
            return ERRP_QAM_CFG_FEC_I128_J7;
        case 8:
            return ERRP_QAM_CFG_FEC_I128_J8;
        default:
            return ERRP_QAM_CFG_ERR;
        }
    default:
        return ERRP_QAM_CFG_ERR;
    }
}


/// Encode RFGW QAM J83 annex configuration according to ERRP
uint8 rfgw_errp_qam_cfg_annex (const rfgw_qam_t *rfgw_cfg)
{
    switch (rfgw_cfg->annex_type) {
    case RFGW_ANNEX_A:
        return ERRP_QAM_CFG_ANNEX_A;
    case RFGW_ANNEX_B:
        return ERRP_QAM_CFG_ANNEX_B;
    case RFGW_ANNEX_C:
        return ERRP_QAM_CFG_ANNEX_C;
    default:
        return ERRP_QAM_CFG_ERR;
    }
}


/// Encode RFGW QAM channel bandwidth configuration according to ERRP
uint8 rfgw_errp_qam_cfg_chan_bw (const rfgw_qam_t *rfgw_cfg)
{
    switch (rfgw_cfg->annex_type) {
    case RFGW_ANNEX_A:
        return ERRP_QAM_CFG_CHAN_BW_8MHZ;
    case RFGW_ANNEX_B:
    case RFGW_ANNEX_C:
        return ERRP_QAM_CFG_CHAN_BW_6MHZ;
    default:
        return ERRP_QAM_CFG_ERR;
    }
}


/// Encode RFGW QAM configuration according to ERRP
/// @return TRUE if successful
///         FALSE if conversion of any parameter fails
boolean rfgw_errp_get_qam_cfg (const rfgw_qam_t *rfgw_cfg,
                               errp_qam_cfg_t *errp_cfg)
{
    errp_cfg->modulation = rfgw_errp_qam_cfg_modulation(rfgw_cfg);
    if (errp_cfg->modulation == ERRP_QAM_CFG_ERR) {
        return FALSE;
    }

    errp_cfg->interleaver = rfgw_errp_qam_cfg_interleaver(rfgw_cfg);
    if (errp_cfg->interleaver == ERRP_QAM_CFG_ERR) {
        return FALSE;
    }


    errp_cfg->annex = rfgw_errp_qam_cfg_annex(rfgw_cfg);
    if (errp_cfg->annex == ERRP_QAM_CFG_ERR) {
        return FALSE;
    }

    errp_cfg->chan_bw = rfgw_errp_qam_cfg_chan_bw(rfgw_cfg);
    if (errp_cfg->freq == ERRP_QAM_CFG_ERR) {
        return FALSE;
    }

    errp_cfg->freq = rfgw_cfg->freq;
    errp_cfg->tsid = rfgw_cfg->tsid;

    return TRUE;
}


// For QAM attribute that has port-level dependency,
// the port_id serves as the TSID group ID
// Note: port_id starts at 1.
// 
#define RFGW_MIN_SLOT           3
#define RFGW_NUM_PORTS_PER_SLOT 12
#define RFGW_MIN_PORT           1

int rfgw_get_port_id (int slot, int port)
{
    return (slot - RFGW_MIN_SLOT) * RFGW_NUM_PORTS_PER_SLOT
           + (port - RFGW_MIN_PORT) + 1;
}


/// Encode RFGW qam capability info according to ERRP
/// @return FALSE if conversion of any parameter fails
/// @todo Need to double check with Nitya!!
void rfgw_errp_get_qam_cap (const rfgw_qam_t *rfgw_cfg,
                            errp_qam_cap_t *errp_cap)
{
    int tsid_grp_id = rfgw_get_port_id(rfgw_cfg->slot, rfgw_cfg->port);

    memset(errp_cap, 0, sizeof(errp_qam_cap_t));

    // Channel Bandwidth capability setting
    errp_cap->chan_bw.tsid_grp_id = tsid_grp_id;
    errp_cap->chan_bw._6mhz = 1;
    errp_cap->chan_bw._8mhz = 1;

    // J.83 capability setting
    errp_cap->j83.tsid_grp_id = tsid_grp_id;
    errp_cap->j83.annex_A = 1;
    errp_cap->j83.annex_B = 1;
    errp_cap->j83.annex_C = 1;

    // Interleaver capability setting
    errp_cap->interleaver.tsid_grp_id = 0;    // configurable per carrier
    errp_cap->interleaver.i8_j16 = 1;
    errp_cap->interleaver.i16_j8 = 1;
    errp_cap->interleaver.i32_j4 = 1;
    errp_cap->interleaver.i64_j2 = 1;
    errp_cap->interleaver.i128_j1 = 1;
    errp_cap->interleaver.i128_j2 = 1;
    errp_cap->interleaver.i128_j3 = 1;
    errp_cap->interleaver.i128_j4 = 1;
    errp_cap->interleaver.i128_j5 = 1;
    errp_cap->interleaver.i128_j6 = 1;
    errp_cap->interleaver.i128_j7 = 1;
    errp_cap->interleaver.i128_j8 = 1;
    errp_cap->interleaver.i12_j7 = 1;

    // Docsis / Video capability setting
    errp_cap->docsis_video.tsid_grp_id = 0;    // configurable per carrier
    errp_cap->docsis_video.video = 1;
    errp_cap->docsis_video.docsis_MPT = 1;

    // Modulation capability setting
    errp_cap->modulation.tsid_grp_id = 0;    // configurable per carrier
    errp_cap->modulation._64_qam = 1;
    errp_cap->modulation._256_qam = 1;
}

