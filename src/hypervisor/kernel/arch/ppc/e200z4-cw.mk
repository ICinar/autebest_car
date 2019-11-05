# e200z4.mk -- for MPC55xx/MPC56xx (always big endian)
#
# PowerPC architecture specific build rules
#
# tjordan, 2014-07-15: cloned from e200z6.mk

ARCH_CFLAGS := -G0 -msoft-float -mno-spe -ffixed-r2 -mmultiple
ARCH_CFLAGS += -mcpu=8548 -DPPC_E200Z4

ARCH_CFLAGS_DEBUG := # -O
ARCH_CFLAGS_NDEBUG := -O2 -fomit-frame-pointer
ARCH_AFLAGS := -mcpu=8548 -DPPC_E200Z4
# for relocatable linking
ARCH_LDFLAGS := -melf32ppc

ARCH_MODS := entry exception string mpu
ARCH_MODS_SMP :=


# Recommended user compiler and linker flags
ARCH_USER_CFLAGS := -mcpu=8548 -G0 -msoft-float
ARCH_USER_AFLAGS := -mcpu=8548
# for relocatable linking
ARCH_USER_LDFLAGS := -melf32ppc

ARCH_CFLAGS :=
ARCH_MODS :=
ARCH_CFLAGS_DEBUG :=
ARCH_CFLAGS_NDEBUG :=
ARCH_AFLAGS :=
ARCH_USER_CFLAGS :=
ARCH_USER_AFLAGS :=
ARCH_USER_LDFLAGS :=
ARCH_LDFLAGS :=