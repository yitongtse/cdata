ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

#===== USEFILE - the file containing the usage message for the application. 
USEFILE=$(PROJECT_ROOT)/$(SECTION)/devn-$(SECTION).use
#===== EXTRA_SRCVPATH - a space-separated list of directories to search for source files.
EXTRA_SRCVPATH=$(EXTRA_SRCVPATH_$(SECTION))

#===== EXTRA_INCVPATH - a space-separated list of directories to search for include files.
EXTRA_INCVPATH=$(PROJECT_ROOT)/public  \
	$(PROJECT_ROOT)/public/drvr  \
	$(USE_ROOT_nto)/usr/include/drvr  \
	$(INSTALL_ROOT_nto)/usr/include/xilinx  \
	$(PROJECT_ROOT)/../bsp-freescale-cdsmpc85xx_prebuilt/usr/include
#===== EXTRA_LIBVPATH - a space-separated list of directories to search for library files.
EXTRA_LIBVPATH+=$(INSTALL_ROOT_nto)/usr/lib/xilinx  \
	$(PROJECT_ROOT)/../bsp-freescale-cdsmpc85xx_prebuilt/$(CPUDIR)/lib  \
	$(PROJECT_ROOT)/../bsp-freescale-cdsmpc85xx_prebuilt/$(CPUDIR)/usr/lib

PRE_SRCVPATH=$(foreach var,$(filter a, $(VARIANTS)),$(CPU_ROOT)/$(subst $(space),.,$(patsubst a,dll,$(filter-out g, $(VARIANTS)))))

#===== EXTRA_SILENT_VARIANTS - variants that are not appended to the result binary name (like MyBin_g)
EXTRA_SILENT_VARIANTS=$(subst -, ,$(SECTION))
#===== NAME - name of the project (default - name of project directory).
NAME=devn-mpc85xx

define PINFO
PINFO DESCRIPTION=
endef

#===== CCFLAGS - add the flags to the C compiler command line. 
CCFLAGS+=$(CCFLAGS_$(basename $@))  \
	$(CCFLAGS_$(basename $@)_$(CPU))
CCFLAGS_wlc_sh=-O0



include $(MKFILES_ROOT)/qmacros.mk

-include $(PROJECT_ROOT)/roots.mk

ifeq ($(filter dll, $(VARIANTS)),)
DEBUG = -g
INSTALLDIR = /lib/dll
CCFLAGS += -Dio_net_dll_entry=io_net_dll_entry_$(SECTION)
USEFILE=
else
LIBS = drvrS pmS cacheS
endif

include $(PROJECT_ROOT)/$(SECTION)/pinfo.mk
#QNX internal start
ENDIAN:=$(filter le be, $(VARIANT_LIST)) 
GCC_VERSION:=$($(firstword $(foreach a, GCC_VERSION_$(CPUDIR) GCC_VERSION, \
			$(if $($(a)), $(a), ))))

internal_libs=$(foreach token, $(LIBS),$(subst ^,,$(token)))
libnames = $(subst lib-Bdynamic.a, ,$(subst lib-Bstatic.a, , \
	$(foreach lib, $(internal_libs), $(IMAGE_PREF_AR)$(lib)$(IMAGE_SUFF_AR))))
libopts = $(subst -l-B,-B, $(foreach lib, $(internal_libs), $(LIBPREF_$(notdir $(lib))) -l$(lib)) )

#QNX internal end
include $(MKFILES_ROOT)/qtargets.mk
