# Bring-up firmware: vendored HMCLOCK-EN sources (ancestor/, plain files) +
# SDK GCC startup + SDK GNU ld.
# Usage:
#   make -C firmware SDK_DIR=<sdkroot> ANC_DIR=<ancestor> BUILD=<outdir>
# Produces $(BUILD)/bwr_hello.bin (raw OTA payload) + .elf/.map, then validates
# with tools/check_bin.py. ancestor/ is a vendored snapshot of
# terenceang/HMCLOCK-EN @ a424ca6 with our changes edited in directly
# (DCLK naming kept, demo attract loop, active-slot version report, v54);
# see PORT_CHECKLIST.md for provenance.
THISDIR := $(dir $(lastword $(MAKEFILE_LIST)))
BUILD ?= $(THISDIR)/../out
SDK_DIR ?= $(HOME)/sdk
ANC_DIR ?= $(THISDIR)/../ancestor

# NOTE: plain `CC ?=` loses to make's built-in default (cc); keep env override.
CC := $(if $(filter default,$(origin CC)),arm-none-eabi-gcc,$(CC))
OBJCOPY ?= arm-none-eabi-objcopy

SRC_ANC := $(ANC_DIR)

include $(BUILD)/sources.mk

CFLAGS := -mcpu=cortex-m0 -mthumb -Os -std=gnu11 -ffunction-sections -fdata-sections \
  -flto -D__DA14585__ -Wall $(INCLUDES) $(FORCE_INC)
# Bring-up defines (drop both for production: selflash then persists slots,
# and the pairing QR screen returns).
# - BRINGUP_NO_SELFFLASH: first boot leaves the vendor fallback bank alone.
# - DEMO_ATTRACT: boot cycles nav demo frames (see demo_draw).
CFLAGS += -DBRINGUP_NO_SELFFLASH -DDEMO_ATTRACT
ASFLAGS := -mcpu=cortex-m0 -mthumb -D__DA14585__ $(INCLUDES) -x assembler-with-cpp
# NOTE: no $(FORCE_INC) here — those are C headers; feeding struct/enum
# declarations to the assembler is exactly the user_config.h "junk at end
# of line" failure. Keil assembles without its C MiscControls for the same
# reason.
LDCPP := -D__DA14585__ \
  -I$(SRC_ANC)/src/config \
  -I$(SDK_DIR)/DA145xx_SDK/6.0.22.1401/sdk/common_project_files
# NOTE: no heap/retention -D overrides here. The ancestor pins 8-connection
# heap sizes explicitly (ENV_HEAP_SZ etc. in da14585_config_advanced.h), so a
# -D would lose to the header redefine. The 1-connection trim is edited into
# ancestor/src/config/da14585_config_advanced.h directly.
LDFLAGS := -mcpu=cortex-m0 -mthumb -Os -flto \
  -specs=nano.specs -specs=nosys.specs \
  -T $(BUILD)/ldscript.ld -L $(SDK_DIR)/DA145xx_SDK/6.0.22.1401/sdk/common_project_files/misc \
  -L $(SDK_DIR)/DA145xx_SDK/6.0.22.1401/sdk/platform/system_library/output/Keil_5 \
  -Wl,--gc-sections -Wl,--no-wchar-size-warning \
  -Wl,-Map=$(BUILD)/bwr_hello.map -Wl,--print-memory-usage
# NOTE: nano + nosys (not full newlib): full libc overflows LR_IROM3 by ~47KB.
# -f[do]function/data-sections on the objects + --gc-sections here is what lets
# nano pull in only the libc it references. Keil da14585_586.lib supplies the
# ROM-patch shims (PATCHED_*, patch_data1, arch_asm_*) that have no GCC sources.
# NOTE: no -nostartfiles: newlib crt0 supplies _start (called by the SDK GCC
# Reset_Handler) and main() lives in arch_main.c. The Keil ARMCC archive
# da14585_586.lib provides PATCHED_*, patch_data1, arch_asm_* (ROM-patch
# shims with no GCC sources); GNU ld usually accepts ARMCC Cortex-M0 objects.
LIBS := -l:da14585_586.lib -lc -lgcc

SOURCES := $(C_SRCS) $(S_SRCS) $(THISDIR)/syscalls.c
OBJS := $(foreach s,$(SOURCES),$(BUILD)/obj/$(subst /,_,$(s)).o)

all: $(BUILD)/bwr_hello.bin

$(BUILD)/sources.mk: $(THISDIR)/../tools/gen_sources.py
	@mkdir -p $(BUILD)
	python3 $< --sdk $(SDK_DIR) --ancestor $(SRC_ANC) --out $@

$(BUILD)/ldscript.ld:
	@mkdir -p $(BUILD)
	$(CC) -E -P $(LDCPP) -I$(SDK_DIR)/DA145xx_SDK/6.0.22.1401/sdk/common_project_files/ldscripts \
	  $(SDK_DIR)/DA145xx_SDK/6.0.22.1401/sdk/common_project_files/ldscripts/ldscript_DA14585_586.lds.S -o $@

define COMPILE_rule
$(BUILD)/obj/$(subst /,_,$(1)).o: $(1) | $(BUILD)/sources.mk
	@mkdir -p $$(@D)
	$(if $(filter %.S,$(1)),$(CC) $(ASFLAGS),$(CC) $(CFLAGS) $(if $(findstring syscalls.c,$(1)),-fno-lto)) -c $$< -o $$@
endef
$(foreach s,$(SOURCES),$(eval $(call COMPILE_rule,$(s))))
# NOTE: syscalls.c gets -fno-lto above. With -flto its stubs become LTO IR and
# --gc-sections discards them before libc references them (_fstat/_isatty).

$(BUILD)/bwr_hello.elf: $(OBJS) $(BUILD)/ldscript.ld
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(BUILD)/bwr_hello.bin: $(BUILD)/bwr_hello.elf
	$(OBJCOPY) -O binary $< $@
	python3 $(THISDIR)/../tools/check_bin.py $@

.PHONY: all clean
clean:
	rm -rf $(BUILD)
