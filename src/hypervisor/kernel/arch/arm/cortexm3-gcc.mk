# cortexm3.mk -- for Cortex M3 (always little endian)
#
# ARM architecture specific build rules
#
# azuepke, 2015-06-24: cloned from a8.mk


_ARCH_MCPU := -mcpu=cortex-m3

ARCH_CFLAGS := -mabi=aapcs-linux
ARCH_CFLAGS += -mthumb
ARCH_CFLAGS += $(_ARCH_MCPU) -DARM_MPU_8 -DARM_CORTEXM -DARM_CORTEXM3 -falign-loops=16

ARCH_CFLAGS_DEBUG := # -O
ARCH_CFLAGS_NDEBUG := -O2 -fomit-frame-pointer
ARCH_AFLAGS := -Wa,-mthumb -Wa,-mimplicit-it=always
ARCH_AFLAGS += $(_ARCH_MCPU) -DARM_MPU_8 -DARM_CORTEXM -DARM_CORTEXM3
# for relocatable linking
ARCH_LDFLAGS := $(call ld-option,-marmelf,-marmelf_linux_eabi)

ARCH_MODS := entry_m exception_m mpu_m
ARCH_MODS_SMP :=


# Recommended user compiler and linker flags
ARCH_USER_CFLAGS := -mthumb
ARCH_USER_CFLAGS += $(_ARCH_MCPU) -DARM_CORTEXM -DARM_CORTEXM3 -falign-loops=16
ARCH_USER_AFLAGS := -Wa,-mthumb -Wa,-mimplicit-it=always
ARCH_USER_AFLAGS += $(_ARCH_MCPU) -DARM_CORTEXM -DARM_CORTEXM3
# for relocatable linking
ARCH_USER_LDFLAGS := $(call ld-option,-marmelf,-marmelf_linux_eabi)
