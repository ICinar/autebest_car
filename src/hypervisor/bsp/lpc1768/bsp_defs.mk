# bsp_defs.mk
#
# Hardware specific definitions for the top-level Makefile
#
# azuepke, 2015-06-28: initial

# NXP LPC1768 with Cortex-M3
ELF_ARCH = arm
ELF_LOADADDR = 0x00000000

# bsp specific targets to run / debug
BSPTARGETS=run debug autorun
