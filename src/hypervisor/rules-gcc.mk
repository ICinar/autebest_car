# rules.mk
#
# Common build rules
#
# azuepke, 2013-03-22: initial
# azuepke, 2013-05-26: fixed .d: striped directories

# cross toolchain
LD = $(CROSS)ld
AS = $(CROSS)gcc -c
CC = $(CROSS)gcc -c
OBJCOPY = $(CROSS)objcopy
DEPCC = $(CROSS)gcc -M
CPP = $(CROSS)gcc -E
NM = $(CROSS)nm
STRIP = $(CROSS)strip
AR = $(CROSS)ar

HOSTCC = gcc -Os
HOSTPERL = $(FOOBAR_PERL)

# {cc|as}-option for CFLAGS/AFLAGS
# Usage: CFLAGS += $(call cc-option,-ffeature-to-test,-ffallback-flag-instead)
cc-option = $(if $(shell $(CC) $1 $2 -S -o /dev/null -xc /dev/null \
              >/dev/null 2>&1 && echo OK), $2, $3)
as-option = $(if $(shell $(AS) $1 $2 -S -o /dev/null -xc /dev/null \
              >/dev/null 2>&1 && echo OK), $2, $3)
ld-option = $(if $(shell $(LD) $1 $2 --version \
              >/dev/null 2>&1 && echo OK), $2, $3)


# default build ID
ifeq ("$(BUILDID)", "")
BUILDID := $(USER)@$(shell hostname) $(shell date +'%Y-%m-%d %H:%M:%S')
endif

CFLAGS := -std=c99 -ffreestanding
ifneq ("$(ARCH)", "tricore")
# GCC4 specific options
CFLAGS += -fno-stack-protector
CFLAGS += -fno-delete-null-pointer-checks
CFLAGS += -fconserve-stack
CFLAGS += -Wdeclaration-after-statement
CFLAGS += -Wno-pointer-sign
CFLAGS += -fno-strict-overflow
CFLAGS += -Wextra
endif

NOSTDINC := -nostdinc

CFLAGS += -g
CFLAGS += -W
CFLAGS += -Wall
CFLAGS += -Wshadow
CFLAGS += -Wpointer-arith
CFLAGS += -Wcast-qual
CFLAGS += -Wmissing-prototypes
CFLAGS += -Wstrict-prototypes
CFLAGS += -fno-strict-aliasing
CFLAGS += -fno-common
CFLAGS += -Werror


AFLAGS := -D__ASSEMBLER__

.SUFFIXES: .c .S .o .d

.%.o: %.S
	@echo "  AS    $<"
	$(Q)$(AS) $(AFLAGS) -o $@ $<

.%.o: %.c
	@echo "  CC    $<"
	$(Q)$(CC) $(CFLAGS) -o $@ $<

.%.d: %.S
	@echo "  DEPAS $<"
	$(Q)$(DEPCC) $(AFLAGS) -MT .$(notdir $(patsubst %.S,%.o,$<)) -MF $@ $<

.%.d: %.c
	@echo "  DEPCC $<"
	$(Q)$(DEPCC) $(CFLAGS) -MT .$(notdir $(patsubst %.c,%.o,$<)) -MF $@ $<

.%.dummy.ld: %_$(FOOBAR_RULESET).ld.S
	@echo "  LDS   $<"
	$(Q)$(CPP) -P -C -D__ASSEMBLY__ -o $@ $<

.%.ld: %_$(FOOBAR_RULESET).ld.S %.ld.h
	@echo "  LDS   $<"
	$(Q)$(CPP) -P -C -D__ASSEMBLY__ -DFINAL_ELF -o $@ $<
