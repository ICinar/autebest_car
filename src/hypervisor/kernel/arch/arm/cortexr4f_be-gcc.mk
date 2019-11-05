# cortexr4f_be.mk -- for Cortex R4 with FPU, big endian
#
# ARM architecture specific build rules
#
# azuepke, 2013-09-11: initial
# azuepke, 2014-04-25: big endian

_ARCH_MCPU := -mcpu=cortex-r4 -mbig-endian

ARCH_CFLAGS := -mabi=aapcs-linux
ARCH_CFLAGS += -mthumb
ARCH_CFLAGS += $(_ARCH_MCPU) -DARM_MPU_12 -DARM_VFP16 -falign-loops=16

ARCH_CFLAGS_DEBUG := # -O
ARCH_CFLAGS_NDEBUG := -O2 -fomit-frame-pointer
ARCH_AFLAGS := -Wa,-mthumb -Wa,-mimplicit-it=always
ARCH_AFLAGS += $(_ARCH_MCPU) -DARM_MPU_12 -DARM_VFP16 -mfpu=vfpv3-d16
# for relocatable linking
ARCH_LDFLAGS := $(call ld-option,-marmelfb,-marmelfb_linux_eabi) -EB

ARCH_MODS := entry exception mpu
ARCH_MODS_SMP :=


# Recommended user compiler and linker flags
ARCH_USER_CFLAGS := -mthumb
ARCH_USER_CFLAGS += $(_ARCH_MCPU) -mfpu=vfpv3-d16 -falign-loops=16
ARCH_USER_AFLAGS := -Wa,-mthumb -Wa,-mimplicit-it=always
ARCH_USER_AFLAGS += $(_ARCH_MCPU) -mfpu=vfpv3-d16
# for relocatable linking
ARCH_USER_LDFLAGS := $(call ld-option,-marmelfb,-marmelfb_linux_eabi) -EB
