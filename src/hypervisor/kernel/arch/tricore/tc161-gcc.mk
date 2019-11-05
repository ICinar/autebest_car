# tc161.mk -- for Infineon TriCore 1.6.1 architecture (always little endian)
#
# Tricore architecture specific build rules
#
# azuepke, 2014-10-24: cloned from ppc

ARCH_CFLAGS := -mtc161 -DTRICORE_TC161

ARCH_CFLAGS_DEBUG := -O # enable inlining, or we get an CSA overflow!
ARCH_CFLAGS_NDEBUG := -Os -fomit-frame-pointer
ARCH_AFLAGS := -mtc161 -DTRICORE_TC161
# for relocatable linking
ARCH_LDFLAGS := -melf32tricore --mcpu=tc161

ARCH_MODS := entry exception mpu
ARCH_MODS_SMP :=


# Recommended user compiler and linker flags
ARCH_USER_CFLAGS := -mtc161 -msmall=8
ARCH_USER_AFLAGS := -mtc161
# for relocatable linking
ARCH_USER_LDFLAGS := -melf32tricore --mcpu=tc161
