# Makefile
#
# Configuration Makefile
#

TOP = ..

include $(TOP)/rules.mk
include $(TOP)/rules-$(FOOBAR_RULESET).mk
include $(TOP)/kernel/arch/$(ARCH)/$(SUBARCH)-$(FOOBAR_RULESET).mk

# upper layer always calls us setting CONFIGFILES. to allow local tests, we use this fallback
ifeq ("$(CONFIGFILES)", "")
CONFIGFILES := $(BSP)_config.xml gen_config.xml
$(warning using default configuration files "$(CONFIGFILES)" - CONFIGFILES not set)
endif

# upper layer always calls us setting OUTDIR. to allow local tests, we use this fallback
ifeq ("$(OUTDIR)", "")
OUTDIR := cfg
$(warning using default output directory "$(OUTDIR)" - OUTDIR not set)
endif

INCLUDES = -nostdinc -I$(TOP)/$(OUTDIR) -I$(TOP)/$(OUTDIR)/include -I$(TOP)/kernel/arch/$(ARCH)/include \
           -I$(TOP)/kernel/include -I$(TOP)/libos/include
CFLAGS += -D__KERNEL \
          $(INCLUDES) \
          $(ARCH_CFLAGS)
AFLAGS += $(INCLUDES) \
          $(ARCH_AFLAGS)

# modules that don't depend on relocation - these aren't rebuilt for final config
MODS_NOREL = ipev_cfg isr_cfg scheduler_cfg wq_cfg rpc_cfg hm_cfg tp_cfg
ifneq ("$(SMP)", "no")
MODS_NOREL += ipi_cfg
endif
CFILES_NOREL := $(addprefix $(TOP)/$(OUTDIR)/,$(addsuffix .c,$(MODS_NOREL)))
OBJS_NOREL := $(addprefix $(TOP)/$(OUTDIR)/.,$(addsuffix .o,$(MODS_NOREL)))

# modules that depend on relocation - rebuilt every time
MODS_RELOC = alarm_cfg counter_cfg kldd_cfg partition_cfg task_cfg mpu_cfg shm_cfg
CFILES_RELOC := $(addprefix $(TOP)/$(OUTDIR)/,$(addsuffix .c,$(MODS_RELOC)))
OBJS_RELOC := $(addprefix $(TOP)/$(OUTDIR)/.,$(addsuffix .o,$(MODS_RELOC)))

OBJS := $(OBJS_NOREL) $(OBJS_RELOC)

# header with relocation information
ADR_DEFINES := $(TOP)/$(OUTDIR)/address_defines.h
ADR_DEFINES_SRC := address_defines.h.tt

# headers that don't depend on relocation
HDRS = counter_defines partition_defines task_defines isr_defines shm_defines wq_defines rpc_defines
CFG_INCLUDE := $(addprefix $(TOP)/$(OUTDIR)/include/,$(addsuffix .h,$(HDRS)))

LDFLAGS += $(ARCH_LDFLAGS)

# get real config files by adding $(TOP) - paths are relative to $(TOP) to avoid problems in windows tooling
CFG_XMLFILES = $(foreach f, $(CONFIGFILES), $(TOP)/$(f))

# ugly make stuff to generate ECCG call - config files must be separated by commas
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
COMMA := $(EMPTY),$(EMPTY)
ECCG_FILELIST=$(subst $(SPACE),$(COMMA),$(strip $(CFG_XMLFILES)))

ECCG_CALL := $(Q)$(ECCG) -c $(ECCG_FILELIST)

ifneq ("$(filter %final_config.xml, $(CONFIGFILES))", "")
CONFIGTYPE = FINAL
CFG_OBJECT = $(TOP)/$(OUTDIR)/.final_config.o
else
CONFIGTYPE = DUMMY
CFG_OBJECT = $(TOP)/$(OUTDIR)/.dummy_config.o
endif

ifeq ("$(DEBUG)", "no")
CFLAGS += -DNDEBUG $(ARCH_CFLAGS_NDEBUG)
AFLAGS += -DNDEBUG
else
CFLAGS += -g $(ARCH_CFLAGS_DEBUG) -DVERBOSE
AFLAGS +=
endif

ifneq ("$(SMP)", "no")
CFLAGS += -DSMP
AFLAGS += -DSMP
else
CFLAGS +=
AFLAGS +=
endif

.PHONY: all clean distclean .FORCE

# we don't want make to kill our intermediate C files
.PRECIOUS: $(TOP)/$(OUTDIR)/%.c

all: $(CFG_OBJECT)

$(TOP)/$(OUTDIR) $(TOP)/$(OUTDIR)/include:
	@echo "  MKDIR $@"
	$(Q)mkdir -p $@

# this is ugly, but actually, we DO want to ignore all of Make's dependency handling.
# rebuild everything on DUMMY, rebuild stuff depending on relocation on FINAL:
$(OBJS_RELOC): $(ADR_DEFINES) .FORCE
$(CFILES_RELOC): .FORCE
$(OBJS): $(CFG_INCLUDE)
ifeq ($(CONFIGTYPE), DUMMY)
$(OBJS_NOREL): .FORCE
$(CFILES_NOREL): .FORCE
$(CFG_INCLUDE): .FORCE
endif

$(CFG_OBJECT): $(OBJS)
	@echo "  LD    $@"
	$(Q)$(LD) $(LDFLAGS) -r -o $@ $^

$(ADR_DEFINES): $(ADR_DEFINES_SRC) .FORCE | $(TOP)/$(OUTDIR)
	@echo "  GEN1  $@ ($(CONFIGTYPE))"
	$(ECCG_CALL) -t $< -o $(TOP)/$(OUTDIR)

$(TOP)/$(OUTDIR)/include/%.h: include/%.h.tt | $(TOP)/$(OUTDIR)/include
	@echo "  GEN1  $@"
	$(ECCG_CALL) -t $< -o $(TOP)/$(OUTDIR)/include

$(TOP)/$(OUTDIR)/%.c: src/%.c.tt | $(TOP)/$(OUTDIR)
	@echo "  GEN1  $@ $(if $(filter $@, $(CFILES_RELOC)),($(CONFIGTYPE)))"
	$(ECCG_CALL) -t $< -o $(TOP)/$(OUTDIR)

$(TOP)/$(OUTDIR)/.%.o: $(TOP)/$(OUTDIR)/%.c | $(TOP)/$(OUTDIR)
	@echo "  CC    $<"
	$(Q)$(CC) $(CFLAGS) -o $@ $<

clean:
	$(Q)rm -f $(TOP)/$(OUTDIR)/include/*.h $(TOP)/$(OUTDIR)/*.c $(TOP)/$(OUTDIR)/*.h
	$(Q)rm -f $(OBJS) $(TOP)/$(OUTDIR)/.final_config.o $(TOP)/$(OUTDIR)/.dummy_config.o

distclean: clean
