# bsp_defs.mk
#
# Hardware specific definitions for the top-level Makefile
#
# azuepke, 2014-10-24: initial

# tricore_testboard with Aurix core
ELF_ARCH = tricore
ELF_LOADADDR = 0xa0000000
ELF_ENTRY    = 0xa0000020

# bsp specific targets to run / debug
BSPTARGETS=run debug

# Set to yes if you want to use the kernel level device driver (KLDD) provided
# by this BSP, otherwise set to no. Currently only a LED KLDD is implemented.
# If you set this to yes, don't forget to setup a kldd in the configuration file
# gen_config.xml.
USE_KLDD=no

# Set to yes if the serial driver should use real serial hardware for the
# output. Set to no if the serial driver should use fake hardware consisting of
# a set of arrays which you can see/export using the debugger.
USE_REAL_SERIAL_HARDWARE=no

# Set to yes if the stm clock driver should call a LED task every time the timer
# expires. The LED task does nothing but uses the 8 leds of the Aurix board to
# print the number of times it was called.
USE_LED_TASK=yes

# Turn off the demonstration LED task if KLDDs are to be used, otherwise the LED
# demonstration will interfere with the LED KLDD.
ifeq ($(USE_KLDD),yes)
    USE_LED_TASK=no
endif


export USE_KLDD
export USE_REAL_SERIAL_HARDWARE
export USE_LED_TASK

