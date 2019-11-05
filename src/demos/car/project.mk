# project.mk
#
# Makefile include for "network" project, for STM32F4 and qemu-arm
#
# azuepke, 2015-11-24: split from master Makefile

# all directories are relative to us
APPDIR := $(PROJECT)
BSP_CONFIG := $(BSP)_config.xml
APP_CONFIG := $(APPDIR)/gen_config_$(BSP).xml
APPS := app1 app2
CONFIGOUTDIR := $(APPDIR)/cfg
OUTDIR := $(APPDIR)/output
