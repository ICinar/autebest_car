# README_NETWORKING.TXT
#
# azuepke, 2015-10-05: initial
# azuepke, 2015-10-21: added STM32F4 example

Example for QEMU ARM
=====================

* Setup tunnel device for QEMU networking:

  #!/bin/sh
  sudo tunctl -u $USER -t tap1
  sudo chmod 666 /dev/net/tun
  sudo ifconfig tap1 192.168.0.1 up

On Ubuntu, tunctl is part of the "uml-utilities" package:

  $ sudo apt-get install uml-utilities

NOTE: we deliberately pick tap1 instead of tap0 to avoid confusion with
VPN tunnels or other software already using the first tunnel.


* Compile

  $ make -f Makefile.alex PROJECT=../demos/network


* Run

Now replace the default networking parameters "-net none" for QEMU with:
  -net nic -net tap,ifname=tap1,script=no,downscript=no

  $ xterm -geom 80x60 -hold -e ~/.qemu/bin/qemu-system-arm -M vexpress-a15 -m 48 -nographic -no-reboot -net nic -net tap,ifname=tap1,script=no,downscript=no -smp 2 -kernel ../demos/network/output/bootfile.elf


* Test

Now you can connect to the TCP echo server on port 7 at 192.168.0.2.

  $ telnet 192.168.0.2 7


Example for STM32F4
====================

The network driver is for the STM32F4 buildin ETHMAC module at 0x40028000.

* Compile

  $ make -f Makefile.alex PROJECT=../demos/network


* Flash

See README_STM32F4.TXT for more details about flashing.

  $ st-flash --reset write ../demos/network/output/bootfile.bin 0x08000000


* Test

After booting the target, you should be able to connect to the TCP echo server
at 192.168.1.21 (assuming your local IP is in the 192.168.1.x range):

  $ telnet 192.168.1.21 7
