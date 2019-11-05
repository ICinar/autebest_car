# e200z4-gcc.mk -- for MPC55xx/MPC56xx/MPC5748G (always big endian)
#
# PowerPC architecture specific build rules
#
# tjordan, 2014-07-15: cloned from e200z6.mk

ARCH_CFLAGS := -G0 -meabi -msdata=none -msoft-float -mno-spe -ffixed-r2 -mmultiple
ARCH_CFLAGS += -mcpu=8548 -DPPC_E200Z4


ARCH_CFLAGS_DEBUG := # -O
ARCH_CFLAGS_NDEBUG := -O2 -fomit-frame-pointer

ARCH_AFLAGS := -mcpu=8548 -DPPC_E200Z4
ARCH_AFLAGS += -G0 -msoft-float

# for relocatable linking
ARCH_LDFLAGS := -melf32ppc

ifeq ($(VLE),yes)
ARCH_MODS := entry_vle
else
ARCH_MODS := entry
endif

ARCH_MODS += exception mpu
ARCH_MODS_SMP :=


# Recommended user space compiler, assembler and linker flags

ARCH_USER_CFLAGS := -mcpu=8548 -meabi -msdata=eabi -msoft-float

ARCH_USER_AFLAGS := -mcpu=8548
# don't use floating points
# without this option you will get the error 'E500 and FPRs not supported'
ARCH_USER_AFLAGS += -msoft-float


ifeq ($(VLE),yes)
ARCH_CFLAGS += -mvle -eabi=vle
ARCH_CFLAGS += -DVLE_ON
ARCH_AFLAGS += -mvle -eabi=vle
ARCH_AFLAGS += -DVLE_ON
ARCH_USER_CFLAGS += -mvle -eabi=vle
ARCH_USER_CFLAGS += -DVLE_ON
ARCH_USER_AFLAGS += -mvle -eabi=vle
ARCH_USER_AFLAGS += -DVLE_ON
endif

ifeq ($(BSP),mpc5748g)
ARCH_CFLAGS += -DMPC5748G
ARCH_AFLAGS += -DMPC5748G
ARCH_USER_CFLAGS += -DMPC5748G
ARCH_USER_AFLAGS += -DMPC5748G
INCLUDES += -I../bsp/$(BSP)/include
endif

ifeq ($(USE_MPU),yes)
ARCH_CFLAGS += -DUSE_MPU
ARCH_AFLAGS += -DUSE_MPU
ARCH_USER_CFLAGS += -DUSE_MPU
ARCH_USER_AFLAGS += -DUSE_MPU
endif

# for relocatable linking
ARCH_USER_LDFLAGS := -melf32ppc
