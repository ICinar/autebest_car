# README_BUILD.TXT
#
# bjungk, 2014-02-13: initial

The build system currently supports Linux and Windows.

Building on Windows
===================

For Windows, MingW and Strawberry Perl are supported. As cross
compiler, the Linaro gcc is used. A package with all required
dependencies is provided in the autobest_tools repository.
Currently, the package requires a 64-bit Windows version.

Using cmd.exe the following commands are required to check out
the tools and the hypervisor source code:

$ hg clone https://scm.easycore.com/scm/hg/easycore/os/autobest_tools
$ hg clone https://scm.easycore.com/scm/hg/easycore/os/autobest

If the pre-packaged toolchain is not used, the following
variables in BUILDENV.sh-qemu-arm-win32 in src/hypervisor have
to be adapted to point to appropriate locations:

FOOBAR_PERL=../../autobest_tools/strawberry/perl/bin/perl
FOOBAR_CROSS_PATH=../../autobest_tools/gcc-linaro-arm-linux-gnueabihf-4.8-2013.10/bin
FOOBAR_LIBXML2_PATH=../../autobest_tools/strawberry/c/bin
FOOBAR_MSYS_PATH=../../autobest_tools/MinGW/msys/1.0/bin
FOOBAR_QEMU=../../autobest_tools/qemu/qemu-system-arm

After cloning the repository and/or adapting the locations, the
build environment has to be sourced and afterwards the hypervisor
and the demo application can be built. It is necessary to run the
following commands with the shell provided by MSYS
(run autobest_tools/MinGW/msys/1.0/msys.bat).

$ cd autobest/src/hypervisor
$ source BUILDENV.sh-qemu-arm-win32
$ make

Building on Linux
=================

For Linux the tools have to be installed manually. For Ubuntu 12.04:
Install the cross compiler:

$ sudo apt-get install gcc-arm-linux-gnueabihf

Additionally, if not already installed, the perl module XML::Simple
has to be installed:

$ sudo apt-get install libxml-simple-perl

Additionally, to compile the testcases, one needs xsltproc:

$ sudo apt-get install xsltproc

Then the following repositories have to be cloned:

$ hg clone https://scm.easycore.com/scm/hg/easycore/os/autobest_tools
$ hg clone https://scm.easycore.com/scm/hg/easycore/os/autobest

After cloning the repositories, the Linux build environment has to
be sourced and afterwards the hypervisor and the demo application
can be built:

$ source BUILDENV.sh-qemu-arm-linux
$ make


Additional packages required to build the C# tools
===================================================

On Linux, Mono's C# compiler is required to build the C#-based configuration
tools. On Ubuntu 14.04, just install:

$ sudo apt-get install mono-mcs
