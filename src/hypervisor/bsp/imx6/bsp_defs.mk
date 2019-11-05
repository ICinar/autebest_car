# bsp_defs.mk
#
# Hardware specific definitions for the top-level Makefile
#
# azuepke, 2016-04-18: initial

# i.MX is real hardware, not supported by QEMU
ELF_ARCH = arm
ELF_LOADADDR = 0x10008000

# no bsp spcific targets (yet)
# BSPTARGETS=
