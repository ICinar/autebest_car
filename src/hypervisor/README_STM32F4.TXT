# README_STM32F4.TXT
#
# azuepke, 2015-07-01: initial

The STM32F4 is a micro controller with the following characteristics:
- ARM Cortex-M4
- MPU with 8 configurable windows
- 168 MHz
- 112 KB SRAM
- 1 MB Flash
- Ethernet
- USB OTG

The supported development board is the "stm32f4-discovery" on a base board.

The development board is USB-powered and offers a second microcontroller to
flash and debug the STM chip. Unfortunately, the ST-LINK/V2 interface provides
no serial interface. Instead, the discovery baseboard provides a SUB-D header
for USART6. Or connect a 3.3V USB-serial to the microcontroller pins located
at the bottom right of the board:
- pin PC6 <-> USART6_TX
- pin PC7 <-> USART6_RX
- ... and GND

The serial line is set to 115200.

NOTE: when using the discovery baseboard, don't forget to short jumpers
CON2.JP1 and CON2.JP2, otherwise the USART isn't routed to the header!

Images can be flashed with either OpenOCD or a Linux-based ST-Flash tool.


UDEV rules
===========

Create UDEV rules for the ST-LINK flash tool or OpenOCD.
Edit /etc/udev/rules.d/72-usbdebug.rules

  $ sudo nano /etc/udev/rules.d/72-usbdebug.rules

and add the following line:

  ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE:="0666"

After changing the rules, re-run udev:

  $ sudo udevadm trigger


Compile the ST-LINK Flash Tool
===============================

Download and compile the flash tool.

  $ cd ~/src
  $ git clone git://github.com/texane/stlink.git
  $ ./autogen.sh
  $ ./configure
  $ make
  $ cp st-flash st-info st-term st-util ~/bin

The flasher is now available as "st-flash" in your local ~/bin directory.


Flash Boot Images
==================

Flashing a boot image is dead simple:

  $ st-flash --reset write bootfile.bin 0x08000000



Flashing with OpenOCD
======================

This was tested with OpenOCD 0.9.0 of May 2015.

Start OpenOCD:

  $ openocd -f board/stm32f4discovery.cfg


Open a second terminal to the OpenOCD command shell:

  $ telnet localhost 4444

Unlock the flash bank on the STM32 chip once, then flash:

  > reset init
  > stm32f4x unlock 0
  > reset init
  > flash protect 0 0 11 off
  > flash erase_address 0x08000000 0x40000
  > flash write_image erase bootfile.elf

But unfortunately, this is broken as well ...
It seems that my STM-chip is locked. What worked in the end was:

  > reset halt
  > stm32f2x unlock 0
  > reset halt; mww 0x40023C08 0x08192A3B; mww 0x40023C08 0x4C5D6E7F; mdw 0x40023c14
  > mww 0x40023C14 0x0fffaaec; sleep 200; mdw 0x40023c14; mww 0x40023C14 0x0fffaaef; sleep 200; mdw 0x40023c14; reset halt; mdw 0x40023c14
  > flash write_image erase bootfile.elf
