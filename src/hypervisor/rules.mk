# rules.mk
#
# Common build rules
#
# azuepke, 2013-03-22: initial
# azuepke, 2013-05-26: fixed .d: striped directories

# overrides from shell environment
ifneq ("$(FOOBAR_ARCH)", "")
ARCH = $(FOOBAR_ARCH)
endif
ifneq ("$(FOOBAR_SUBARCH)", "")
SUBARCH = $(FOOBAR_SUBARCH)
endif
ifneq ("$(FOOBAR_BSP)", "")
BSP = $(FOOBAR_BSP)
endif
ifneq ("$(FOOBAR_CROSS)", "")
CROSS = $(FOOBAR_CROSS)
endif

ifneq ("$(FOOBAR_CROSS)", "")
QEMU = $(FOOBAR_QEMU)
endif

ifneq ("$(FOOBAR_ECCG)", "")
ECCG = $(FOOBAR_ECCG)
endif

ifneq ("$(FOOBAR_MPU_CFG)", "")
MPU_CFG = $(FOOBAR_MPU_CFG)
endif

# Default rules for DEBUG and SMP
ifeq ("$(DEBUG)", "")
DEBUG = yes
endif
ifeq ("$(SMP)", "")
SMP = yes
endif

# Catch errors
ifeq ("$(FOOBAR_ARCH)", "")
$(error Please source a build environment!)
endif
ifeq ("$(FOOBAR_SUBARCH)", "")
$(error Please source a build environment!)
endif
ifeq ("$(FOOBAR_BSP)", "")
$(error Please source a build environment!)
endif

# NOTE: set VERBOSE to disable suppressing of compiler commandlines
ifeq ("$(VERBOSE)", "")
Q=@
endif
