# bsp_defs.mk
#
# Hardware specific definitions for the top-level Makefile
#
# tjordan, 2014-08-01: initial

# TMS570 is real hardware, not supported by QEMU
ELF_ARCH = armbe
ELF_LOADADDR = 0x00000000

# no bsp spcific targets (yet)
# BSPTARGETS=
