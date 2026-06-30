ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

EXTRA_INCVPATH = $(PROJECT_ROOT)/../../public
CCFLAGS += -DRESMGR_OCB_T="void"

NAME=nfm-dinky
include $(MKFILES_ROOT)/qmacros.mk
USEFILE=$(PROJECT_ROOT)/$(NAME).use

ifeq ($(filter dll, $(VARIANTS)),)
DEBUG=-g -O0
INSTALLDIR = /dev/null
CCFLAGS += -Dio_net_dll_entry=io_net_dll_entry_$(patsubst nfm-%,%,$(NAME))
endif

ifeq ($(filter ctl, $(VARIANTS)),)
EXCLUDE_OBJS+=dinkyctl.o
else
NAME=dinky
EXCLUDE_OBJS+=dinky.o
endif

include $(MKFILES_ROOT)/qtargets.mk

