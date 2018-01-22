ifeq ($(CFG_HI_EXPORT_FLAG),)
SDK_DIR := $(shell pwd)/../../../..
include $(SDK_DIR)/base.mak
endif

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	variable
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

objects := api
SUBLIBS += api/libapi.a

ifeq ($(CFG_HI_ADVCA_SUPPORT), y)
ifeq ($(CFG_HI_ADVCA_TYPE), VERIMATRIX)
ifeq ($(CFG_HI_ADVCA_VMX_3RD_SIGN),y)
objects += api/src/harden/vmx_advance/3rd_loader_verify_lib
SUBLIBS += api/src/harden/vmx_advance/3rd_loader_verify_lib/libhi_vmx_loader_verify.a
else
objects += api/src/harden/vmx_advance/vmx_loader_verify_lib
SUBLIBS += api/src/harden/vmx_advance/vmx_loader_verify_lib/libhi_vmx_loader_verify.a
endif
endif

ifeq ($(CFG_HI_ADVCA_TYPE), VERIMATRIX_ULTRA)
ifeq ($(CFG_HI_ADVCA_VMX_3RD_SIGN),y)
else
objects += api/src/harden/vmx_ultra/vmx_loader_verify_lib
SUBLIBS += api/src/harden/vmx_ultra/vmx_loader_verify_lib/libhi_vmx_loader_verify.a
endif
endif
else
objects += schedule
SUBLIBS += schedule/libschedule.a
endif

ifeq ($(CFG_HI_LOADER_BOOTLOADER),y)
objects += app
SUBLIBS += app/libapp.a
endif

ifeq ($(CFG_HI_MINIBOOT_SUPPORT),y)
CFLAGS += -DHI_LOADER_RUNNING_ENVIRONMENT_MINIBOOT
CFLAGS += -DHI_LOADER_RUNNING_ENVIRONMENT_BOOT
else ifeq ($(CFG_HI_UBOOT_SUPPORT),y)
CFLAGS += -DHI_LOADER_RUNNING_ENVIRONMENT_UBOOT
CFLAGS += -DHI_LOADER_RUNNING_ENVIRONMENT_BOOT
else 

endif

objects_clean:=$(addsuffix _clean,$(objects))
objects_install:=$(addsuffix _install,$(objects))
objects_uninstall:=$(addsuffix _uninstall,$(objects))

depends_clean:=$(addsuffix _clean,$(depends))
depends_install:=$(addsuffix _install,$(depends))
depends_uninstall:=$(addsuffix _uninstall,$(depends))

LIBS := libloader.a

$(LIBS): $(objects) 
	$(AT)rm -rf $@
	$(AT)rm -rf ar.mak
	$(AT)echo "CREATE $@" >> ar.mak
	$(AT)$(foreach sublib,$(SUBLIBS),echo "ADDLIB $(sublib)" >> ar.mak;)
	$(AT)echo "SAVE" >> ar.mak
	$(AT)echo "END" >> ar.mak
	$(AT)$(AR) -M < ar.mak

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	rules
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#==================================================================
#                          all
#==================================================================
.PHONY: all clean install uninstall

all: $(LIBS)
#	$(AT)$(STRIP) $(LIBS)

clean: $(objects_clean)

install: $(objects_install)

uninstall: $(objects_uninstall)

#==================================================================
#                          modules
#==================================================================
.PHONY: $(objects) $(objects_clean) $(objects_install) $(objects_uninstall)

$(objects): 
	- $(AT)test -d ./api/include || mkdir -p ./api/include
	- $(AT)test -d ./api/src || mkdir -p ./api/src
	- $(AT)test -d ./api/src/harden || mkdir -p ./api/src/harden
	- $(AT)test -d ./api/src/harden/include || mkdir -p ./api/src/harden/include
	- $(AT)test -d ./api/src/harden/conax || mkdir -p ./api/src/harden/conax
	- $(AT)test -d ./api/src/harden/crc || mkdir -p ./api/src/harden/crc
	- $(AT)test -d ./api/src/harden/vmx_advance || mkdir -p ./api/src/harden/vmx_advance
	- $(AT)test -d ./api/src/harden/vmx_advance/3rd_loader_verify_lib || mkdir -p ./api/src/harden/vmx_advance/3rd_loader_verify_lib
	- $(AT)test -d ./api/src/harden/vmx_advance/vmx_loader_verify_lib || mkdir -p ./api/src/harden/vmx_advance/vmx_loader_verify_lib
	- $(AT)test -d ./api/src/harden/vmx_ultra || mkdir -p ./api/src/harden/vmx_ultra
	- $(AT)test -d ./api/src/harden/vmx_ultra/3rd_loader_verify_lib || mkdir -p ./api/src/harden/vmx_ultra/3rd_loader_verify_lib
	- $(AT)test -d ./api/src/harden/vmx_ultra/vmx_loader_verify_lib || mkdir -p ./api/src/harden/vmx_ultra/vmx_loader_verify_lib

	- $(AT)cp -f $(COMPONENT_DIR)/loader/api/include/*.[ch] ./api/include/	
	- $(AT)cp -f $(COMPONENT_DIR)/loader/api/src/*.[ch] ./api/src/	
	- $(AT)cp -f $(COMPONENT_DIR)/loader/api/src/harden/include/*.[ch] ./api/src/harden/include/
	- $(AT)cp -f $(COMPONENT_DIR)/loader/api/src/harden/conax/*.[ch] ./api/src/harden/conax/
	- $(AT)cp -f $(COMPONENT_DIR)/loader/api/src/harden/vmx_ultra/*.[ch] ./api/src/harden/vmx_ultra/
	- $(AT)cp -f $(COMPONENT_DIR)/loader/api/src/harden/vmx_ultra/3rd_loader_verify_lib/*.[ch] ./api/src/harden/vmx_ultra/3rd_loader_verify_lib/
	- $(AT)cp -f $(COMPONENT_DIR)/loader/api/src/harden/vmx_ultra/vmx_loader_verify_lib/*.[ch] ./api/src/harden/vmx_ultra/vmx_loader_verify_lib/
	- $(AT)cp -f $(COMPONENT_DIR)/loader/api/src/harden/vmx_advance/*.[ch] ./api/src/harden/vmx_advance/
	- $(AT)cp -f $(COMPONENT_DIR)/loader/api/src/harden/vmx_advance/3rd_loader_verify_lib/*.[ch] ./api/src/harden/vmx_advance/3rd_loader_verify_lib/
	- $(AT)cp -f $(COMPONENT_DIR)/loader/api/src/harden/vmx_advance/vmx_loader_verify_lib/*.[ch] ./api/src/harden/vmx_advance/vmx_loader_verify_lib/
	- $(AT)cp -f $(COMPONENT_DIR)/loader/api/src/harden/crc/*.[ch] ./api/src/harden/crc/

	make -C $@ all

$(objects_clean):
	make -C $(patsubst %_clean,%,$@) clean
	
	$(AT)rm -rf $(LIBS)
	$(AT)rm -rf ar.mak
	
	$(AT)rm -f ./api/include/*.[ch]
	$(AT)rm -f ./api/src/*.[ch]
	$(AT)rm -f ./api/src/harden/include/*.[ch]
	$(AT)rm -f ./api/src/harden/conax/*.[ch]
	$(AT)rm -f ./api/src/harden/vmx_ultra/*.[ch]
	$(AT)rm -f ./api/src/harden/vmx_ultra/3rd_loader_verify_lib/*.[ch]
	$(AT)rm -f ./api/src/harden/vmx_ultra/vmx_loader_verify_lib/*.[ch]
	$(AT)rm -f ./api/src/harden/vmx_advance/*.[ch]
	$(AT)rm -f ./api/src/harden/vmx_advance/3rd_loader_verify_lib/*.[ch]
	$(AT)rm -f ./api/src/harden/vmx_advance/vmx_loader_verify_lib/*.[ch]
	$(AT)rm -f ./api/src/harden/crc/*.[ch]

$(objects_install):
	make -C $(patsubst %_install,%,$@) install

$(objects_uninstall):
	$(AT)echo "uninstall loader"
	#make -C $(patsubst %_uninstall,%,$@) uninstall

