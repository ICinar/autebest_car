# cortexa15.mk -- for Cortex A15 (always little endian)
#
# ARM architecture specific build rules
#
# azuepke, 2013-09-11: initial
# azuepke, 2014-04-25: little endian

_ARCH_MCPU := $(call cc-option,-mcpu=cortex-a15,-mcpu=cortex-a9)

ARCH_CFLAGS := -mabi=aapcs-linux
ARCH_CFLAGS += -mthumb
ARCH_CFLAGS += $(_ARCH_MCPU) -DARM_MMU -DARM_CORTEXA15 -DARM_VFP32

ARCH_CFLAGS_DEBUG := # -O
ARCH_CFLAGS_NDEBUG := -O2 -fomit-frame-pointer
ARCH_AFLAGS := -Wa,-mthumb -Wa,-mimplicit-it=always
ARCH_AFLAGS += $(_ARCH_MCPU) -DARM_MMU -DARM_CORTEXA15 -DARM_VFP32
# for relocatable linking
ARCH_LDFLAGS := $(call ld-option,-marmelf,-marmelf_linux_eabi)

ARCH_MODS := entry exception mmu
ARCH_MODS_SMP :=


# Recommended user compiler and linker flags
ARCH_USER_CFLAGS := -mthumb
ARCH_USER_CFLAGS += $(_ARCH_MCPU)
ARCH_USER_AFLAGS := -Wa,-mthumb -Wa,-mimplicit-it=always
ARCH_USER_AFLAGS += $(_ARCH_MCPU)
# for relocatable linking
ARCH_USER_LDFLAGS := $(call ld-option,-marmelf,-marmelf_linux_eabi)
