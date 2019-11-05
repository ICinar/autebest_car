# bsp specific definitions

CROSS := powerpc-autobest-elf-
#LD =  mwldeppc.exe
LD = $(CROSS)ld
AS = mwasmeppc.exe -gnu_mode -proc 5674 
CC = mwcceppc.exe -c -proc 5674 -dialect c99 -model absolute -sdatathreshold 0 -sdata 0 -sdata2 0 -fp off -use_lmw_stmw on -use_isel on -enum int -requireprotos
OBJDUMP = $(CROSS)objdump
READELF = $(CROSS)readelf
DEPCC = $(CROSS)gcc -M
CPP = $(CROSS)gcc -E

# enable this to disable cache and branch prediction
#DEFINES := -DDISABLE_HWOPT

#ARCH_CFLAGS := -proc Zen $(DEFINES)
#ARCH_CFLAGS += -g -vle -ppc_asm_to_vle
#ARCH_CFLAGS += -str reuse,pool,readonly
#ARCH_CFLAGS += -ipa file
#ARCH_CFLAGS += -sdata 1024 -sdata2 1024
OPT ?= O2
ifeq ($(OPT),Os)
ARCH_CFLAGS += -O4,s
else
ARCH_CFLAGS += -O4,p
endif
ARCH_AFLAGS :=  -D__ASSEMBLER__ -proc Zen $(DEFINES) -vle

CFLAGS += -w on \
		  -Cpp_exceptions off \
		  -gccinc $(INCLUDES) \
          $(ARCH_CFLAGS)


ASFLAGS += -gccinc $(INCLUDES) \
          $(ARCH_AFLAGS)

