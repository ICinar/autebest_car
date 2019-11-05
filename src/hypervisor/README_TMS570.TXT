# README_TMS570.TXT
#
# azuepke, 2013-11-04: initial
# azuepke, 2014-01-27: reworked, check-in
# azuepke, 2014-04-28: updated to Uniflash 3.1.0.00026

Get the latest manual:
$ wget http://www.ti.com/lit/ug/spnu499b/spnu499b.pdf


The TI TMS570 board eval board is known as "Hercules board".
It provides the TMS570 chip together with an dual-interface FTDI chip
as programming/debug interface via USB.

Board:
- TMS570LS3137
- USB XDS100v2 JTAG
- 20-pin JTAG
- 8 MB SDRAM
- 8 LEDs
- SDCARD
- 180 MHz (16 MHz external quartz)
- 10/100/1000 Ethernet with DP83640 PHY and RMII / MII interface


CPU core
=========
- Cortex R4F
- using BE32 mode (big endian instructions)
- 12 MPU windows
- 95 interrupts


Memory Map
===========
- 0000 0000 Flash (3 MB)
- 0800 0000 internal SRAM (256 KB)
- 2000 0000 Flash Mirror (3 MB)
- 8000 0000 SDRAM


Serial
=======
- internal SCI -> 2nd Port of FDTI for "virtual COM port"
- connect via /dev/ttyUSB0 as usual
- the baudrate defaults to 115200


Flashing from Linux
====================

Flashing natively from Linux is much faster than from a Windows VM.

Download UniFlash (Successor of NowFlash), registration at ti.com required:

  http://processors.wiki.ti.com/index.php/Category:CCS_UniFlash

Click on Release Notes to download the "offline version" installer (220 MB):
Download the software for Linux, extract the .tgz and run the installer:

  $ cd UNIFLASH3.1.0.00026_linux/
  $ ./uniflash_setup_3.1.0.00026.bin

Run the installer as normal user, without root rights, even if it complaints.
It will install the software into ~/ti/

During installation, select "Custom", "Safety MCUs", and then "XDS100 class".

NOTE: The request for driver installation will just set similar UDEV rules
than we did for OpenOCD already. Can be skipped if OpenOCD is properly
configured. Alternatively, the UDEV Rules are (Ubuntu/Debian specific):

  $ sudo nano /etc/udev/rules.d/72-usbdebug.rules
  # FTDI 2232 for TMS570
  SUBSYSTEM=="usb", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="a6d0", MODE:="0666"
  SUBSYSTEM=="usb", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="a6d1", MODE:="0666"
  SUBSYSTEM=="usb", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6010", MODE:="0666"

After changing the rules, re-run udev:

  $ sudo udevadm trigger


Then goto into ~/ti and create the following file ~/ti/tms570.ccxml:

  <?xml version="1.0" encoding="UTF-8" standalone="no"?>
  <configurations XML_version="1.2" id="configurations_0">
    <configuration XML_version="1.2" id="Texas Instruments XDS100v2 USB Emulator_0">
    <instance XML_version="1.2" desc="Texas Instruments XDS100v2 USB Emulator_0" href="connections/TIXDS100v2_Connection.xml" id="Texas Instruments XDS100v2 USB Emulator_0" xml="TIXDS100v2_Connection.xml" xmlpath="connections"/>
    <connection XML_version="1.2" id="Texas Instruments XDS100v2 USB Emulator_0">
      <instance XML_version="1.2" href="drivers/tixds100v2icepick_c.xml" id="drivers" xml="tixds100v2icepick_c.xml" xmlpath="drivers"/>
      <instance XML_version="1.2" href="drivers/tixds100v2cs_dap.xml" id="drivers" xml="tixds100v2cs_dap.xml" xmlpath="drivers"/>
      <instance XML_version="1.2" href="drivers/tixds100v2cortexR.xml" id="drivers" xml="tixds100v2cortexR.xml" xmlpath="drivers"/>
      <platform XML_version="1.2" id="platform_0">
        <instance XML_version="1.2" desc="tms570ls3137_0" href="devices/tms570ls3137.xml" id="tms570ls3137_0" xml="tms570ls3137.xml" xmlpath="devices"/>
      </platform>
    </connection>
    </configuration>
  </configurations>

The XML configuration above contains the settings for the Hercules board.

NOTE: Remove the spaces in front of <?xml ...>, otherwise the flasher fails with:
  Fatal Error at (1, 9): The XML or Text declaration must start at line/column 1/1



Now you can flash ELF files on Linux.
NOTE: Code Composer Studios uses .out as file type.


*** Flashing:

$ cd ~/ti
$ ./uniflash.sh -ccxml tms570.ccxml -program /path/to/your/ccs/file.out

For faster flashing, try this:
$ ./uniflash.sh -setOptions FlashEraseSelection='Necessary Sectors Only (for Program Load)' -ccxml tms570.ccxml -program path/.../file.out

### NOTE: THIS DOES NOT WORK IN LINUX!!!!!!!
### NOTE: See: http://e2e.ti.com/support/development_tools/code_composer_studio/f/81/p/318171/1107218.aspx
### NOTE: Try this instead:

### NOTE: 2014-04-28: This is still required for version 3.1.0.00026


Create the file ~/ti/fast.xml:

  <?xml version="1.0" encoding="UTF-8" standalone="no"?><PartNumber id=""><Core name="CortexR4"><DebugProperties>&lt;?xml version="1.0" encoding="UTF-8" standalone="no" ?&gt;
  &lt;PropertyValues&gt;

    &lt;property id="FlashEraseSelection"&gt;
      &lt;curValue&gt;Necessary Sectors Only (for Program Load)&lt;/curValue&gt;
    &lt;/property&gt;

  &lt;/PropertyValues&gt;
  </DebugProperties><FilePaths/></Core></PartNumber>

Again, remove leading spaces before <?xml ...>


Finally, we create a script named "flash.sh" to do the hard work for us:
  #!/bin/sh
  if [ ! -f "$1" ]; then
    echo "usage: flash.sh <ELF-file>"
    exit 1
  fi
  ./uniflash.sh -ccxml tms570.ccxml -loadSettings fast.xml -program "$1"


*** Interaction with OpenOCD
You can flash while OpenOCD is connected, however, the original connection
of OpenOCD is lost and OpenOCD gets stuck. Restart OpenOCD after flashing.
