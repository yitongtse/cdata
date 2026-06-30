ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

NAME=nfm-enbridge

include $(MKFILES_ROOT)/qmacros.mk
USEFILE=$(PROJECT_ROOT)/$(NAME).use

ifeq ($(filter dll, $(VARIANTS)),)
INSTALLDIR = /dev/null
CCFLAGS += -Dio_net_dll_entry=io_net_dll_entry_$(patsubst nfm-%,%,$(NAME))
endif

include $(MKFILES_ROOT)/qtargets.mk

