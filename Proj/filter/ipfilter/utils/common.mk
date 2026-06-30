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

NAME=$(SECTION)
USEFILE=$(PROJECT_ROOT)/$(SECTION)/$(NAME).use
EXTRA_SILENT_VARIANTS=$(SECTION)
INSTALLDIR=usr/sbin

LIBS=socket

EXTRA_INCVPATH = $(PRODUCT_ROOT)/netinet $(PRODUCT_ROOT)
CCFLAGS += -DRESMGR_OCB_T="void" -DGATEWAY 
CCFLAGS += -DDIRECTED_BROADCAST -DINET -DIPF_DEFAULT_PASS=FR_PASS
CCFLAGS += -DIPFILTER_LOG -DIPFILTER_AUTH -DIPF_FTP_PROXY
CCFLAGS += -Dioctl=ipf_devctl 
CCFLAGS += -DNetBSD=199905 -D__NetBSD_Version__=105000200 -DBSD=199506 -D__NetBSD__
CCFLAGS += -Du_quad_t=uint64_t -Du_int32_t=uint32_t

EXCLUDE_OBJS_ipf   = kmem.o
EXCLUDE_OBJS_ipfs  = common.o facpri.o parse.o opt.o
EXCLUDE_OBJS_ipmon = common.o facpri.o parse.o opt.o
EXCLUDE_OBJS_ipnat = facpri.o parse.o opt.o
EXCLUDE_OBJS = $(EXCLUDE_OBJS_$(SECTION))

include $(MKFILES_ROOT)/qtargets.mk
