# bsp_defs.mk
#
# Hardware specific definitions for the top-level Makefile
#
# tjordan, 2014-08-01: initial

# BeagleBone Black is also real hardware, not supported by QEMU
ELF_ARCH = arm
ELF_LOADADDR = 0x80008000

# no bsp spcific targets (yet)
# BSPTARGETS=
