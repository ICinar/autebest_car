# README_QEMU.TXT
#
# azuepke, 2013-03-26: initial
# azuepke, 2013-08-08: include 64-bit x86 version
# azuepke, 2013-09-15: add ARM and PPC
# azuepke, 2013-11-25: QEMU 1.7
# azuepke, 2015-03-26: QEMU 2.2
# azuepke, 2015-06-12: ARM IPI problems


The kernel requires at QEMU 2.2 or newer for testing
due to a bug in ARM multicore support in earlier versions.
Download latest QEMU from http://wiki.qemu.org/Download

You also need to apply the following patch:

diff --git a/target-arm/translate.c b/target-arm/translate.c
index 39692d7..ef80e89 100644
--- a/target-arm/translate.c
+++ b/target-arm/translate.c
@@ -4084,6 +4084,7 @@ static void gen_nop_hint(DisasContext *s, int val)
         gen_set_pc_im(s, s->pc);
         s->is_jmp = DISAS_WFI;
         break;
+    case 1: /* yield */
     case 2: /* wfe */
         gen_set_pc_im(s, s->pc);
         s->is_jmp = DISAS_WFE;



The following instruction shows how to install a local copy of QEMU
in your home directory, e.g. /home/user/.qemu
Do not change the $HOME/.qemu path unless you have a good reason, as the
build infrastructure of the kernel also depend on this.



*** Compile ***

Untar first, then:

$ ./configure --target-list="arm-softmmu ppc-softmmu" \
              --prefix=$HOME/.qemu --disable-vnc --disable-xen --disable-kvm
$ make
$ make install


The QEMU binaries will be installed here:

$ ls -la ~/.qemu/bin/
drwxr-xr-x 2 zuepke promi    4096 Nov 25 18:21 .
drwxr-xr-x 7 zuepke promi    4096 Sep 23 13:48 ..
-rwxr-xr-x 1 zuepke promi 6344400 Nov 25 18:21 qemu-system-arm
-rwxr-xr-x 1 zuepke promi 6418704 Nov 25 18:21 qemu-system-ppc
...



*** Troubleshooting ***

QEMU requires zlib:
  # (Ubuntu 12.04 specific!!!)
  $ sudo apt-get install zlib1g-dev

If configure errs with:
  glib-2.12 required to compile QEMU
install the missing packages + SDL developer pacakges:
  # (Ubuntu 12.04 specific!!!)
  $ sudo apt-get install libglib2.0-dev libsdl1.2-dev

If the build fails with "/bin/sh: 1: autoreconf: not found", then add
the necessary build tools to your host
  # (Ubuntu 12.04 specific!!!)
  $ sudo apt-get install automake libtool

Sometimes, also libpixman is required:
  # (Ubuntu 12.04 specific!!!)
  $ sudo apt-get install libpixman-1-dev

And libfdt:
  # (Ubuntu 14.04 specific!!!)
  $ sudo apt-get install libfdt-dev

NOTE: when compiling QEMU from GIT source, you can fetch GIT submodules with:
  $ git submodule update --init pixman
  $ git submodule update --init dtc
