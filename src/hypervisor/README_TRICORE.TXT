# README_TRICORE.TXT
#
# azuepke, 2014-10-24: initial
# azuepke, 2014-11-16: removed toolchain description

# see tricore_patches/README_TRICORE_GCC.TXT for the toolchain!


# QEMU for TriCore
# NOTE: using Git checkout of 2014-10-24, relating to QEMU version 2.1.50
git clone git://git.qemu-project.org/qemu.git
cd qemu.git
./configure --target-list="tricore-softmmu" \
              --prefix=$HOME/.qemu --disable-vnc --disable-xen --disable-kvm
make
make install

./qemu-system-tricore -machine tricore_testboard -cpu aurix


********************************************************************************

Special Registers
==================
PSW		Program Status Word
PCXI	Previous Context Information
PC		Program Counter
FCX		Free Context List Head Pointer
LCX		Free Context List Limit Pointer
ISP		Interrupt Stack Pointer
ICR		Interrupt Control Register
PIPN	Pending Interrupt Priority Number
BIV		Base Address of Interrupt Vector Table
BTV		Base Address of Trap Vector Table


Tricore EABI
==============

// lower half
A[0]	system global address register: small data section             -> _SMALL_DATA_ @ section start + 0x8000
A[1]	system global address register: literal data section (ro-data) -> _LITERAL_DATA_ @ section start + 0x8000
A[2]	addr retval0
A[3]	addr retval1
A[4]	addr arg0
A[5]	addr arg1
A[6]	addr arg2
A[7]	addr arg3

// upper half automatically saved on CALL
A[8]	system global address register: OS reserved, data
A[9]	system global address register: OS reserved, 
A[10]	stack pointer
A[11]	return address (RA) for CALL, JL, JLA and JLI and on interrupts (PC)
A[12]
A[13]
A[14]
A[15]	implicit base

B[x]/P[x]: even pair of A[x] regs

// lower half
D[0]	scratch
D[1]	scratch
D[2]	data retval0
D[3]	data retval1
D[4]	data arg0
D[5]	data arg1
D[6]	data arg2
D[7]	data arg3

// upper half automatically saved on CALL
D[8]
D[9]
D[10]
D[11]
D[12]
D[13]
D[14]
D[15]	implicit data

E[x]: even pair of D[x] regs

Lower Context: | PCXI, A11 | A2+  | D0+ | D2+  | A4+  | A6+  | D4+  | D6+  |
Upper Context: | PCXI, PSW | A10+ | D8+ | D10+ | A12+ | A14+ | D12+ | D14+ |


- Alignment like on x86 (64-bit data types aligned to 32-bit boundaries)
- Stack grows down, always aligned to 8 bytes
- no strict order when placing args: "fill up the holes"
- Varargs: variable registers are always placed on stack
- System call number is encoded in the SYSCALL instruction and passed as TIN
- CSA = Context Save Area (16 words, 6-word aligned), save lower or upper half
- CPU Modes:
  - User-0 Mode -> no peripheral access
  - User-1 Mode -> privileged user mode
  - Supervisor Mode
- ENDINIT protected: BTV, BIV, ISP, BMACON, COMPAT, MIECON, PMA0, SMACON

********************************************************************************

AURIX 277 memory map -> see page 349 in manual
PFLASH 4 MB Flash
PSPR Program Scratch-Pad SRAM, per core
DSPR Data Scratch-Pad SRAM, per core

PSPR c000'0000 core local
DSPR d000'0000 core local


0 reserved

1 reserved

2 reserved

3 reserved

4 reserved

5 5000'0000 CPU2.DSPR, 120K
  5001'e000 CPU2.DCACHE, 8K
  500c'0000 CPU2.DTAG
  5010'0000 CPU2.PSPR, 32K
  5010'8000 CPU2.PCACHE, 16K
  501c'0000 CPU2.PTAG

6 6000'0000 CPU1.DSPR, 120K
  6001'e000 CPU1.DCACHE, 8K
  600c'0000 CPU1.DTAG
  6010'0000 CPU1.PSPR, 32K
  6010'8000 CPU1.PCACHE, 16K
  601c'0000 CPU1.PTAG

7 7000'0000 CPU0.DSPR, 112K
  7010'0000 CPU0.PSPR, 24K
  7010'6000 CPU0.PCACHE, 8K
  701c'0000 CPU0.PTAG

8 8000'0000 PFLASH0, 2M, cached
  8020'0000 PFLASH1, 2M, cached
  8fff'8000 Boot ROM, 32K, cached

9 9000'0000 LMU SRAM, 32K, cached

a a000'0000 PFLASH0, 2M, uncached
  a020'0000 PFLASH1, 2M, uncached
  af00'0000 DFLASH0, 1M
  af10'0000 DFLASH0, 16K
  af11'0000 DFLASH1, 64K
  afff'8000 Boot ROM, 32K, uncached

b b000'0000 LMU SRAM, 32K, uncached

c core local PSPR, maps to 5010'0000, 6010'0000, or 7010'0000

d core local DSPR, maps to 5000'0000, 6000'0000, or 7000'0000

e reserved

  (see page 358+)
f f000'0000 System Timer 0 (STM0), 256
  f000'2e00 General Purpose Timer GPT120, 256
  f003'0000 System Peripheral Bus Control Unit (BCU), 256
  f003'6000 System Control Unit (SCU), 1K
  f003'6800 SafetyManagement Unit (SMU), 2K
  f003'7000 Interrupt Router (IR), 4K
  f003'8000 Interrupt Router SRC registers, 8K

  f880'0000 SFR Core 0
  f880'2000 SFR Core 1
  f880'4000 SFR Core 2
