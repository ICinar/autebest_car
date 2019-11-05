# bsp_defs.mk
#
# Hardware specific definitions for the top-level Makefile
#
# azuepke, 2014-10-24: initial

# tricore_testboard with Aurix core
ELF_ARCH = tricore
ELF_LOADADDR = 0xa0000000
ELF_ENTRY = 0xa0000020

# bsp specific targets to run / debug
BSPTARGETS=run debug
