# bsp_defs.mk
#
# Hardware specific definitions for the top-level Makefile
#
# tjordan, 2014-08-01: initial

# mpc85xx board with e500 core (e200z6 not supported by QEMU)
ELF_ARCH = ppc
ELF_LOADADDR = 0x00100000

# bsp specific targets to run / debug
BSPTARGETS=run debug
