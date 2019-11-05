# e200z6.mk -- for MPC55xx/MPC56xx (always big endian)
#
# PowerPC architecture specific build rules
#
# azuepke, 2014-06-03: cloned from a8.mk

ARCH_CFLAGS := -meabi -msdata=none -msoft-float -mno-spe -ffixed-r2 -mmultiple
ARCH_CFLAGS += -mcpu=8548 -DPPC_E200Z6

ARCH_CFLAGS_DEBUG := # -O
ARCH_CFLAGS_NDEBUG := -O2 -fomit-frame-pointer
ARCH_AFLAGS := -mcpu=8548 -DPPC_E200Z6
# for relocatable linking
ARCH_LDFLAGS := -melf32ppc

ARCH_MODS := entry exception mpu
ARCH_MODS_SMP :=


# Recommended user compiler and linker flags
ARCH_USER_CFLAGS := -mcpu=8548 -meabi -msdata=eabi -msoft-float
ARCH_USER_AFLAGS := -mcpu=8548
# for relocatable linking
ARCH_USER_LDFLAGS := -melf32ppc
