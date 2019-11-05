# README_BEAGLEBONEBLACK.TXT
#
# azuepke, 2013-12-22: initial

What is a BeagleBone Black?
============================

http://beagleboard.org/products/beaglebone%20black
- ARM Cortex A8 based Texas Instruments Sitara AM335x chip
  - L1 cache: I + D 32K each
  - L2 cache: 256K
  - 64K internal SRAM
- 512 MB DDR3 RAM
- 2 GB Flash
- SD-Card
- Ethernet


UART
=====

The UART is available on J1.
Pin 1: Ground
Pin 4: RX
Pin 5: TX

The pins reflect the names on the cable, TX and RX are swapped internally.
The UART adaptor must use 3.3V!

See here for a detailed description on how to connect the board:
http://dave.cheney.net/2013/09/22/


Connect / Startup
==================

- plug in Ethernet
- plug in the UART, 115200 Baud, 8n1
- plug in the 5V / 1A power

- turn on power and connect to U-Boot's console

- Angström Login: root / no password

- modify /media/BEAGLEBONE/uEnv.txt as follows
  (if the file isn't available: wait a second or two,
  until internal flash was mounted)
  $ nano /media/BEAGLEBONE/uEnv.txt

ipaddr=192.168.1.13
serverip=192.168.1.1
bootfile=beagleboneblack
uenvcmd=echo "Booting from network"; tftp 0x80008000; go 0x80008000
optargs=quiet drm.debug=7

- after editing, please reboot:
  $ reboot

The board will now try to load a "beagleboneblack" file from your TFTP server!
I chose 0x80008000 as start address like the Linux kernel, but without a
preloader.


Boot into Linux in NAND flash by hand
======================================

Interrupt autobooting and type the following into the U-Boot prompt
to boot into the Linux installed in NAND flash:

i2c mw 0x24 1 0x3e;
run findfdt;
mmc dev 1;
setenv mmcdev 1;
setenv bootpart 1:2;
run loaduimage;
run loadfdt;
run mmcboot;


Memory Map
===========
- 8000'0000  512 MB RAM
- 44e0'0f00  RPM_DEVICE (reset logic)
- 44e0'5000  DMTimer0
- 44e0'9000  UART0
- 4820'0000  Interrupt Controller  (L4 periphals)


Interrupt Controller
=====================
- 128 Interrupts
- level-sensitive only
- page 204


UART Setup
===========
- 115200 16x oversampling
- divisor=26
- 48 MHz clock for the UART
- page 4010


Original /media/BEAGLEBONE/uEnv.txt in NAND-Flash
==================================================
optargs=quiet drm.debug=7


Original U-Boot Environment
============================

U-Boot# pri
arch=arm
baudrate=115200
board=am335x
board_name=A335BNLT
board_rev=0A5C
bootcmd=gpio set 53; i2c mw 0x24 1 0x3e; run findfdt; mmc dev 0; if mmc rescan ; then echo micro SD card found;setenv mmcdev 0;else echo No micro SD card found, setting mmcdev to 1;setenv mmcdev 1;fi;setenv bootpart ${mmcdev}:2;mmc dev ${mmcdev}; if mmc rescan; then gpio set 54; echo SD/MMC found on device ${mmcdev};if run loadbootenv; then echo Loaded environment from ${bootenv};run importbootenv;fi;if test -n $uenvcmd; then echo Running uenvcmd ...;run uenvcmd;fi;gpio set 55; if run loaduimage; then gpio set 56; run loadfdt;run mmcboot;fi;fi;
bootdelay=1
bootdir=/boot
bootenv=uEnv.txt
bootfile=uImage
bootpart=0:2
console=ttyO0,115200n8
cpu=armv7
dfu_alt_info_emmc=rawemmc mmc 0 3751936
dfu_alt_info_mmc=boot part 0 1;rootfs part 0 2;MLO fat 0 1;MLO.raw mmc 100 100;u-boot.img.raw mmc 300 3C0;u-boot.img fat 0 1;uEnv.txt fat 0 1
dfu_alt_info_nand=SPL part 0 1;SPL.backup1 part 0 2;SPL.backup2 part 0 3;SPL.backup3 part 0 4;u-boot part 0 5;kernel part 0 7;rootfs part 0 8
ethact=cpsw
ethaddr=c8:a0:30:b9:89:4c
fdt_high=0xffffffff
fdtaddr=0x80F80000
fdtfile=am335x-boneblack.dtb
findfdt=if test $board_name = A33515BB; then setenv fdtfile am335x-evm.dtb; fi; if test $board_name = A335X_SK; then setenv fdtfile am335x-evmsk.dtb; fi;if test $board_name = A335BONE; then setenv fdtfile am335x-bone.dtb; fi; if test $board_name = A335BNLT; then setenv fdtfile am335x-boneblack.dtb; fi
importbootenv=echo Importing environment from mmc ...; env import -t $loadaddr $filesize
kloadaddr=0x80007fc0
loadaddr=0x80200000
loadbootenv=load mmc ${mmcdev} ${loadaddr} ${bootenv}
loadfdt=load mmc ${bootpart} ${fdtaddr} ${bootdir}/${fdtfile}
loadramdisk=load mmc ${mmcdev} ${rdaddr} ramdisk.gz
loaduimage=load mmc ${bootpart} ${kloadaddr} ${bootdir}/${bootfile}
mmcargs=setenv bootargs console=${console} ${optargs} root=${mmcroot} rootfstype=${mmcrootfstype}
mmcboot=echo Booting from mmc ...; run mmcargs; bootm ${kloadaddr} - ${fdtaddr}
mmcdev=0
mmcroot=/dev/mmcblk0p2 ro
mmcrootfstype=ext4 rootwait
mtdids=nand0=omap2-nand.0
mtdparts=mtdparts=omap2-nand.0:128k(SPL),128k(SPL.backup1),128k(SPL.backup2),128k(SPL.backup3),1920k(u-boot),128k(u-boot-env),5m(kernel),-(rootfs)
nandargs=setenv bootargs console=${console} ${optargs} root=${nandroot} rootfstype=${nandrootfstype}
nandboot=echo Booting from nand ...; run nandargs; nand read ${loadaddr} ${nandsrcaddr} ${nandimgsize}; bootm ${loadaddr}
nandimgsize=0x500000
nandroot=ubi0:rootfs rw ubi.mtd=7,2048
nandrootfstype=ubifs rootwait=1
nandsrcaddr=0x280000
netargs=setenv bootargs console=${console} ${optargs} root=/dev/nfs nfsroot=${serverip}:${rootpath},${nfsopts} rw ip=dhcp
netboot=echo Booting from network ...; setenv autoload no; dhcp; tftp ${loadaddr} ${bootfile}; tftp ${fdtaddr} ${fdtfile}; run netargs; bootm ${loadaddr} - ${fdtaddr}
nfsopts=nolock
ramargs=setenv bootargs console=${console} ${optargs} root=${ramroot} rootfstype=${ramrootfstype}
ramboot=echo Booting from ramdisk ...; run ramargs; bootm ${loadaddr} ${rdaddr} ${fdtaddr}
ramroot=/dev/ram0 rw ramdisk_size=65536 initrd=${rdaddr},64M
ramrootfstype=ext2
rdaddr=0x81000000
rootpath=/export/rootfs
soc=am33xx
spiargs=setenv bootargs console=${console} ${optargs} root=${spiroot} rootfstype=${spirootfstype}
spiboot=echo Booting from spi ...; run spiargs; sf probe ${spibusno}:0; sf read ${loadaddr} ${spisrcaddr} ${spiimgsize}; bootm ${loadaddr}
spibusno=0
spiimgsize=0x362000
spiroot=/dev/mtdblock4 rw
spirootfstype=jffs2
spisrcaddr=0xe0000
static_ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}:${hostname}::off
stderr=serial
stdin=serial
stdout=serial
usbnet_devaddr=c8:a0:30:b9:89:4c
vendor=ti
ver=U-Boot 2013.04-dirty (Jun 19 2013 - 09:57:14)
