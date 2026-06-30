/*
 *------------------------------------------------------------------
 * ermi_r_errp_msg.h
 *
 * ERRP protocol message processing header file
 *
 * Jan 2009, Dean Chen
 *
 * Copyright (c) 2009 by cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */

enum {
  ERRP_QAM_CH_CFG_MOD_MODE_64QAM = 3,
  ERRP_QAM_CH_CFG_MOD_MODE_256QAM = 4,
  ERRP_QAM_CH_CFG_MOD_MODE_128QAM = 5,
  ERRP_QAM_CH_CFG_MOD_MODE_512QAM = 6,
  ERRP_QAM_CH_CFG_MOD_MODE_1024QAM = 7,
};

enum {
  ERRP_QAM_CH_CFG_INTRLVR_I8_J16 = 3,
  ERRP_QAM_CH_CFG_INTRLVR_I16_J8 = 4,
  ERRP_QAM_CH_CFG_INTRLVR_I32_J4 = 5,
  ERRP_QAM_CH_CFG_INTRLVR_I64_J2 = 6,
  ERRP_QAM_CH_CFG_INTRLVR_I128_J1 = 7,
  ERRP_QAM_CH_CFG_INTRLVR_I12_J7 = 8,
  ERRP_QAM_CH_CFG_INTRLVR_I128_J2 = 9,
  ERRP_QAM_CH_CFG_INTRLVR_I128_J3 = 10,
  ERRP_QAM_CH_CFG_INTRLVR_I128_J4 = 11,
  ERRP_QAM_CH_CFG_INTRLVR_I128_J5 = 12,
  ERRP_QAM_CH_CFG_INTRLVR_I128_J6 = 13,
  ERRP_QAM_CH_CFG_INTRLVR_I128_J7 = 14,
  ERRP_QAM_CH_CFG_INTRLVR_I128_J8 = 15,
};

enum {
  ERRP_QAM_CH_CFG_ANNEX_A = 0,
  ERRP_QAM_CH_CFG_ANNEX_B = 1,
  ERRP_QAM_CH_CFG_ANNEX_C = 2,
};

enum {
  ERRP_QAM_CH_CFG_BW_6MHZ = 0,
  ERRP_QAM_CH_CFG_BW_7MHZ = 2,
  ERRP_QAM_CH_CFG_BW_8MHZ = 1,
};
