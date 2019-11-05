# README_GDB.TXT
#
# azuepke, 2013-09-17: initial
# bjungk, 1014-03-12: added comment about make debug and Windows variant

Debugging the kernel in QEMU
=============================

Debugging with Windows
======================

To debug the kernel startup code, you need to supply "-gdb tcp::1234,ipv4 -S" 
to QEMU. With -s alone, qemu tries to bind the gdbserver to the IPv6 port if
IPv6 is available, which does not work with current GDB implementations.

The correct QEMU version is supplied in the autobest_tools hg repository:
(https://scm.easycore.com/scm/hg/easycore/os/autobest_tools)
The correct GDB version is also included in the autobest_tools hg repository.

Pull the repository and invoke QEMU with "make debug" (or manually with the 
additional -gdb tcp::1234,ipv4 -S options). QEMU will then stop at the first 
instruction and will wait until you connect with GDB.

  $ arm-linux-gnueabihf-gdb.exe bsp/qemu-arm/kernel.elf

Connect to the virtual QEMU target:
  (gdb) target remote localhost:1234

Tip: you can omit the localhost, this is faster:
  (gdb) target remote :1234
  
Debugging with Linux
====================

To debug the kernel startup code, you need to supply "-s -S" to QEMU.

But at first, you need an ARM GDB. Ubuntu provides a multiarch-gdb for this:
  # (Ubuntu 12.04 specific!!!)
  $ sudo apt-get install gdb-multiarch

Now invoke QEMU with "make debug" (or manually with the additional -s -S options). 
QEMU will then stop at the first instruction and will wait until you connect with 
GDB.

  $ gdb-multiarch bsp/qemu-arm/kernel.elf

Connect to the virtual QEMU target:
  (gdb) target remote localhost:1234

Tip: you can omit the localhost, this is faster:
  (gdb) target remote :1234

Typical GDB commands
=====================

Quit
  (gdb) q

List source
  (gdb) l

Register dump
  (gdb) info registers

Show breakpoints
  (gdb) info breakpoints

Set (hardware) breakpoint
  (gdb) hbreak function-name | *address

Step (enters function) / Next (does not enter function) / Continue
  (gdb) s | n | c | si | ni

Backtrace
  (gdb) bt
  (gdb) up | down
  (gdb) info frame | args | locals

Print / Hex / Dec / Dump
  (gdb) p variable
  (gdb) p/x variable
  (gdb) p/d variable
  (gdb) x variable

Turn GDB into assembler mode
  (gdb) layout asm


Using a GUI toolkit
====================

  # (Ubuntu 12.04 specific!!!)
  $ sudo apt-get install nemiver

  $ nemiver --gdb-binary=/usr/bin/gdb-multiarch --remote=localhost:1234 path/to/kernel
