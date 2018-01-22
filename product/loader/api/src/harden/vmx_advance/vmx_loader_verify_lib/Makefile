#===============================================================================
# export variable
#===============================================================================
ifeq ($(CFG_HI_EXPORT_FLAG),)
SDK_DIR ?= $(shell cd $(CURDIR)/../../../../../../../ && /bin/pwd)

include $(SDK_DIR)/base.mak
endif

#===============================================================================
# local variable
#===============================================================================
CFLAGS += -I$(CURDIR)
CFLAGS += -I$(COMPONENT_DIR)/loader/api/src

SOURCE :=  vmx_auth.c

LIBS := libhi_vmx_loader_verify.a
OBJS := $(patsubst %.c,%.o, $(SOURCE))

all: $(LIBS)

install: all

$(LIBS): $(OBJS)
	$(AT)$(AR) -rc $@ $^

%.o: %.c
	$(AT)echo "cc: $^ *************************"
	$(AT)$(CC) $(CFLAGS) -c $^ -o "$(*D)/$(*F).o"

clean:
	$(AT)rm -f $(OBJS) $(LIBS)
