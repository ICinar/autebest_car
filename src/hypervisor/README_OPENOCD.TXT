# README_OPENOCD.TXT
#
# azuepke, 2014-01-27: initial
# azuepke, 2014-03-04: update: patch http://openocd.zylin.com/#/c/1884/ merged
# azuepke, 2014-04-28: OpenOCD 0.8.0 released, less quirks

OpenOCD is required to handle the JTAG connection to the TMS570 board
on Linux (and Windows) as well.

For the TMS570 board, we need OpenOCD 0.8.0 or later.
Ubuntu's version 0.5.0 is too old.

Get a new version from here:
http://sourceforge.net/projects/openocd/files/openocd/


Required packages to build OpenOCD (specific to Ubuntu 12.04):

  $ sudo apt-get install libusb-1.0-0-dev libusb-dev

Probably libftdi is required as well:

  $ sudo apt-get install libftdi-dev



Configure and build OpenOCD (installed in ~/.openocd locally)
  $ ./configure --prefix=$HOME/.openocd
  $ make
  $ make install



Create a symlink in your local bin directory (optional):

  $ cd ~/bin
  $ ln -s ~/.openocd/bin/openocd



Add necessary UDEV Rules (Ubuntu/Debian specific)

  $ sudo nano /etc/udev/rules.d/72-usbdebug.rules
  # FTDI 2232 for TMS570
  ATTRS{idVendor}=="0403", ATTRS{idProduct}=="a6d0", MODE:="0666"
  ATTRS{idVendor}=="0403", ATTRS{idProduct}=="a6d1", MODE:="0666"
  ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6010", MODE:="0666"

  $ sudo udevadm trigger



Create a configuration file named ``hercules.cfg´´ for the TMS570 board:

  source [find interface/ftdi/xds100v2.cfg]
  set CHIPNAME tms570

  # TMS570LS31x Rev. B
  set DAP_TAPID 0x2b8a002f
  set JRC_TAPID 0x2b8a002f

  source [find target/ti_tms570.cfg]

  init; ftdi_set_signal PWR_RST 1; jtag arp_init



Run OpenOCD:

  $ openocd -f hercules.cfg

  # alternatively, use the full path to your OpenOCD installation
  $ ~/.openocd/bin/openocd -f hercules.cfg



You should see an output similar to the following text:

  zuepke@bc243-2:~$ openocd -f hercules.cfg
  Open On-Chip Debugger 0.8.0-dev-00331-g1137eae-dirty (2014-01-27-13:55)
  Licensed under GNU GPL v2
  For bug reports, read
          http://openocd.sourceforge.net/doc/doxygen/bugs.html
  Info : only one transport option; autoselect 'jtag'

  Info : to use this adapter you MUST add ``init; ftdi_set_signal PWR_RST 1; jtag arp_init'' to the end of your config file!

  adapter speed: 10 kHz
  adapter speed: 1500 kHz
  Info : clock speed 1500 kHz
  Info : JTAG tap: tms570.jrc tap/device found: 0x2b8a002f (mfg: 0x017, part: 0xb8a0, ver: 0x2)
  Info : JTAG tap: tms570.dap enabled
  Info : tms570.cpu: hardware has 6 breakpoints, 2 watchpoints
  Info : number of cache level 1
  Error: mpdir not in multiprocessor format
  Info : JTAG tap: tms570.jrc tap/device found: 0x2b8a002f (mfg: 0x017, part: 0xb8a0, ver: 0x2)
  Info : JTAG tap: tms570.dap enabled

The last two lines are important: ... device found: 0x2b8a002f ...



Now you can connect to OpenOCD's control interface via a local socket:

  $ telnet localhost 4444



Basic comments of OpenOCD's control interface:

  > help                                           Help
  > md{b|h|w} <addr> [<count>]                     Memory Dump
  > mw{b|h|w} <addr> <value> [<count>]             Memory Write
  > reg [<reg name>]                               Register Dump
  > load_image <file> <addr> ['bin'|'elf']         Load binary or ELF file

  > halt                                           Halt target
  > reset halt                                     Reset and halt afterwards
  > step                                           Single stepping
  > resume                                         Continue Execution
  > bp <addr> <length> hw                          Set hardware breakpoint
  > arm disassemble <addr> [<count> ['thumb']]     Disassembler

  > arm mcr cpnum op1 CRn CRm op2 value            write coprocessor register
  > arm mrc cpnum op1 CRn CRm op2                  read coprocessor register



Other tools -- Kermit

  $ sudo apt-get install ckermit
  $ cat ~/.kermrc
  set line /dev/ttyUSB0
  set speed 115200
  set parity none
  set stop-bits 1
  set flow-control none
  set carrier-watch off
  set file type bin
  set file name lit
  set rec pack 1000
  set send pack 1000
  set window 5
  set prompt Kermit>


  At the kermit prompt, press "c" to connect.
  To quit Kermit, press Ctrl+] (Ctrl+AltGr+9) to get back to command mode,
  then press "q" for quit.
