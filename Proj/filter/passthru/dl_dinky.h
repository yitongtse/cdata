/*
 * dl_pingd.h
 */

extern int io_net_dll_entry_dinky();

static const struct dll_syms _dinky_symbols[] = {
	{"io_net_dll_entry", &io_net_dll_entry_dinky},
	{NULL, NULL}
};

#define DLL_DINKY_LIST		"nfm-dinky.so", _dinky_symbols
