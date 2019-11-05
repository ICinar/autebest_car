# bsp_defs.mk
#
# Hardware specific definitions for the top-level Makefile
#
# azuepke, 2015-07-01: initial

# STM32F4 with Cortex-M4
ELF_ARCH = arm
ELF_LOADADDR = 0x08000000

# bsp specific targets to run / debug
BSPTARGETS=run debug autorun
