#
# Copyright 2001, QNX Software Systems Ltd. All Rights Reserved
#
# This source code has been published by QNX Software Systems Ltd. (QSSL).
# However, any use, reproduction, modification, distribution or transfer of
# this software, or any software which includes or is based upon any of this
# code, is only permitted under the terms of the QNX Open Community License
# version 1.0 (see licensing.qnx.com for details) or as otherwise expressly
# authorized by a written license agreement from QSSL. For more information,
# please email licensing@qnx.com.

ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

NAME=ipfilter

TCPIP6_PATH = $(PRODUCT_ROOT)/../../../../../services/net/npm/tcpip-1-5
EXTRA_INCVPATH = $(PROJECT_ROOT)/../../public $(TCPIP6_PATH) $(TCPIP6_PATH)/sys-nto
EXTRA_INCVPATH += $(PRODUCT_ROOT)/../../../../../lib/socket/public_nto

# this is how the stack compiled.
CCFLAGS += -D_KERNEL -DMROUTING -DNO_SPLIMP -D_LKM -DSIMPLE_MALLOC_BSD \
       	   -DRANDOM_BSD -DMAXUSERS=32 -DKMEMSTATS
CCFLAGS +=  -Wp,-include -Wp,$(TCPIP6_PATH)/qnx.h
CCFLAGS += -DRESMGR_OCB_T="void" -DGATEWAY -DIPFILTER_LOG

include $(MKFILES_ROOT)/qmacros.mk
USEFILE=$(PROJECT_ROOT)/$(NAME).use

# force a link to tcpip
VARIENT_ENDIAN=$(filter be le, $(VARIANT_LIST))
ifeq (,$(VARIENT_ENDIAN))
LDOPTS=-l$(TCPIP6_PATH)/$(CPU)/dll/npm-tcpip-v6.so
else
LDOPTS=-l$(TCPIP6_PATH)/$(CPU)/dll.$(VARIENT_ENDIAN)/npm-tcpip6.so
endif

include $(MKFILES_ROOT)/qtargets.mk
