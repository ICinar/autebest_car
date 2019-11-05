# bsp_defs.mk
#
# Hardware specific definitions for the top-level Makefile
#
# tjordan, 2014-08-01: initial

# real hardware, no QEMU support
ELF_ARCH = ppc
ELF_LOADADDR = 0x00fc0000

# bsp specific targets
BSPTARGETS=debug
