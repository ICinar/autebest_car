# README_LPC1768.TXT
#
# azuepke, 2015-06-30: initial

The NXP LPC1768 is a micro controller with the following characteristics:
- ARM Cortex-M3
- MPU with 8 configurable windows
- 100 MHz
- 64 KB SRAM
- 512 KB Flash
- Ethernet
- USB OTG

The supported development board is the "mbed", see https://mbed.org/.

The mbed devel board provides two microcontrollers, the the Arduino platform.
The second microcontroller acts as flasher/debug-interface and connects
to the host computer via a micro-USB adaptor. The board is also USB-powered.
The USB-interface provides a serial link and a disk device. The disk devices
shows up as a FAT-formatted disk of 2M size. Each time the LPC1768 is reset,
the flasher checks the root file system of the FAT-file system and loads
the latest file end on *.bin into flash. The file system can remain mounted
during development.

The serial link is connected to UART0 of the LPC1768. Up to three /dev/ttyACMx
devices show up on Linux, with /dev/ttyACM0 or /dev/ttyACM3 being the right one.
The serial interface is configured to a baudrate of 115200.
When the serial port receives a "break" signal, it resets the LPC1768.

Unfortunately, the SRAM is split into two separated areas of 32K each:

Memory map:

0000 0000  512K flash, vector table starts at 0, no boot loader
1000 0000  32K  local SRAM
2007 c000  16K  AHB SRAM bank 0  \___ 32K SRAM for device I/O
2008 0000  16K  AHB SRAM bank 1  /
4000 c000  UART0
4001 0000  UART1
4009 8000  UART2
4009 c000  UART3

The mbed clocks the LPC1768 at 96 MHz, see PLL-setup code in board.c.
