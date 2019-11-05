# bsp_defs.mk
#
# Hardware specific definitions for the top-level Makefile
#
# tjordan, 2014-08-01: initial

# Versatile Express Cortex A15
ELF_ARCH = arm
ELF_LOADADDR = 0x80000000

# bsp specific targets to run / debug
BSPTARGETS=run debug autorun
