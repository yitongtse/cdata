/*
 * $QNXLicenseC:  
 * Copyright 2005, QNX Software Systems. All Rights Reserved.
 *
 * This source code may contain confidential information of QNX Software 
 * Systems (QSS) and its licensors.  Any use, reproduction, modification, 
 * disclosure, distribution or transfer of this software, or any software 
 * that includes or is based upon any of this code, is prohibited unless 
 * expressly authorized by QSS by written agreement.  For more information 
 * (including whether this source code file has been published) please
 * email licensing@qnx.com. $
*/





/*
 * dl_mpc85xx.h
 */

extern int io_net_dll_entry_mpc85xx();

static const struct dll_syms _mpc85xx_symbols[] = {
	{"io_net_dll_entry", &io_net_dll_entry_mpc85xx},
	{NULL, NULL}
};

#define DLL_MPC85XX_LIST		"devn-mpc85xx.so", _mpc85xx_symbols
