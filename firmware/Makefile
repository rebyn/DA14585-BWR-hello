# Bring-up firmware: ancestor (HMCLOCK-EN) sources + SDK GCC startup + SDK GNU ld.
# Usage:
#   make -C firmware SDK_DIR=<sdkroot> ANC_DIR=<ancestor> BUILD=<outdir> [BRINGUP=1]
# Produces $(BUILD)/bwr_hello.bin (raw OTA payload) + .elf/.map, then validates
# with tools/check_bin.py. The ancestor tree is NEVER modified: BRINGUP=1
# (default) applies patches/*.patch to a private copy first.
THISDIR := $(dir $(lastword $(MAKEFILE_LIST)))
BUILD ?= $(THISDIR)/../out
SDK_DIR ?= $(HOME)/sdk
ANC_DIR ?= $(THISDIR)/../ancestor
BRINGUP ?= 1
CC ?= arm-none-eabi-gcc
OBJCOPY ?= arm-none-eabi-objcopy

SRC_ANC := $(ANC_DIR)
ifeq ($(BRINGUP),1)
  SRC_ANC := $(BUILD)/ancestor-src
  ANC_DEP := $(BUILD)/ancestor-src
endif

include $(BUILD)/sources.mk

CFLAGS := -mcpu=cortex-m0 -mthumb -Os -std=gnu11 -ffunction-sections -fdata-sections \
  -D__DA14585__ -Wall $(INCLUDES) $(FORCE_INC)
ASFLAGS := -mcpu=cortex-m0 -mthumb -D__DA14585__ $(INCLUDES) $(FORCE_INC) -x assembler-with-cpp
LDCPP := -D__DA14585__ -I$(SRC_ANC)/src/config \
  -I$(SDK_DIR)/DA145xx_SDK/6.0.22.1401/sdk/common_project_files
LDFLAGS := -mcpu=cortex-m0 -mthumb -nostartfiles \
  -T $(BUILD)/ldscript.ld -L $(SDK_DIR)/DA145xx_SDK/6.0.22.1401/sdk/common_project_files/misc \
  -Wl,--gc-sections -Wl,-Map=$(BUILD)/bwr_hello.map

SOURCES := $(C_SRCS) $(S_SRCS)
OBJS := $(foreach s,$(SOURCES),$(BUILD)/obj/$(subst /,_,$(s)).o)

all: $(BUILD)/bwr_hello.bin

$(BUILD)/sources.mk: $(THISDIR)/../tools/gen_sources.py $(ANC_DEP)
	@mkdir -p $(BUILD)
	python3 $< --sdk $(SDK_DIR) --ancestor $(SRC_ANC) --out $@

$(BUILD)/ancestor-src: $(THISDIR)/../patches/bringup-no-selfflash.patch
	@rm -rf $@ && cp -r $(ANC_DIR) $@ && rm -rf $@/.git
	patch -p1 --directory=$@ < $< && touch $@

$(BUILD)/ldscript.ld:
	@mkdir -p $(BUILD)
	$(CC) -E -P $(LDCPP) -I$(SDK_DIR)/DA145xx_SDK/6.0.22.1401/sdk/common_project_files/ldscripts \
	  $(SDK_DIR)/DA145xx_SDK/6.0.22.1401/sdk/common_project_files/ldscripts/ldscript_DA14585_586.lds.S -o $@

define COMPILE_rule
$(BUILD)/obj/$(subst /,_,$(1)).o: $(1) | $(BUILD)/sources.mk
	@mkdir -p $$(@D)
	$(if $(filter %.S,$(1)),$(CC) $(ASFLAGS),$(CC) $(CFLAGS)) -c $$< -o $$@
endef
$(foreach s,$(SOURCES),$(eval $(call COMPILE_rule,$(s))))

$(BUILD)/bwr_hello.elf: $(OBJS) $(BUILD)/ldscript.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@ -lc -lgcc

$(BUILD)/bwr_hello.bin: $(BUILD)/bwr_hello.elf
	$(OBJCOPY) -O binary $< $@
	python3 $(THISDIR)/../tools/check_bin.py $@

.PHONY: all clean
clean:
	rm -rf $(BUILD)
