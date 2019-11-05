#!/usr/bin/perl -w
#
# ab_gen_mpu_c.pl - generate mpu.c with MPU config from XML files
#
# NOTE: requires the XML::Simple CPAN module
#       on Ubuntu 12.04, try:  sudo apt-get install libxml-simple-perl
#
# Usage: ab_gen_mpu_c.pl system.xml -r -o config.c
#
# azuepke, 2014-08-21: fleshed out of configgen.pl
# azuepke, 2014-09-29: SHM support
# azuepke, 2014-10-01: emit MPU config for tasks, ISRs and hooks as well
# azuepke, 2015-02-23: TriCore support
# azuepke, 2015-08-03: ARM v7 page tables
# azuepke, 2015-08-05: handle cached attribute
# azuepke, 2016-01-13: RPC


use strict;
use warnings "all";
use XML::Simple;
use Data::Dumper;

# tool version ID
my $VERSION = "ab_gen_mpu_c.pl 2016-01-13";

# global variables
my $sysxmlfile;
my $memxmlfile;
my $mpu_arch;
my $cfgfile;
my $CFGFILE;
my $split_code_data = 0;
my $gen_pagetables = 0;
my $final_run = 0;

# Evaluate hex or decimal number
sub number
{
	$_ = shift;
	if (substr ($_, 0, 2) eq '0x') {
		return hex $_;
	}
	return int $_;
}

# Return hex string of argument
sub hexify
{
	return sprintf("0x%08x", shift);
}

# Integer log to base 2
sub ilog {
	return int((log shift) / log(2));
}

# print an MPU window in $ARCH-specific format
# with level:
# 0 -> kern
# 1 -> part
# 2 -> task
sub print_mpu_window
{
	my $window = shift;
	my $level = shift;

	my $id = number $window->{id};
	my $start = number $window->{start};
	my $size = number $window->{size};
	my $r = !!$window->{read}+0;
	my $w = !!$window->{write}+0;
	my $x = !!$window->{exec}+0;
	my $c = !!$window->{cached}+0;
	my $arch = number $window->{arch};

	print $CFGFILE "\t\t\t/* start: ", hexify($start), ", size: ", hexify($size);
	print $CFGFILE " rwx: $r$w$x, arch: ", hexify($arch), " */\n";

	if ($mpu_arch =~ /arm_cortexr4/) {
		my $bas = $start;
		my $siz = (ilog($size)-1 << 1) | 1;
		my $acc = !$x << 12;	# XN bit
		if ($level == 0) {
			# kernel window: user has no access
			if ($w) {
				$acc |= 0x1 << 8;	# read-write
			} else {
				$acc |= 0x5 << 8;	# read-only
			}
		} else {
			# user window: supervisor has same access than user
			if ($w) {
				$acc |= 0x3 << 8;	# read-write
			} else {
				$acc |= 0x6 << 8;	# read-only
			}
		}

		# cache attributes
		if ($c) {
			# FIXME: hardcoded: WBWA caching, non-shared
			$acc |= 0xb;	# TEX S CB = 001 0 11
		} else {
			# uncached, strongly ordered
			$acc |= 0x0;	# TEX S CB = 000 0 00
		}

		# ignore kernel windows
		if ($level == 0) {
			print $CFGFILE "\t\t\t/* -- not used for kernel -- */\n";
		} else {
			print $CFGFILE "\t\t\t{ .base = ", hexify($bas),
			                   ", .size_enable = ", hexify($siz),
			                   ", .access_control = ", hexify($acc), " },\n";
		}
	} elsif ($mpu_arch =~ /arm_cortexm/) {
		my $bas = $start;
		my $siz = (ilog($size)-1 << 1) | 1;
		my $acc = !$x << 28;	# XN bit
		# user window: supervisor has same access than user
		if ($w) {
			$acc |= 0x3 << 24;	# read-write
		} else {
			$acc |= 0x6 << 24;	# read-only
		}

		if ($bas >= 0x00000000 && $bas < 0x20000000) {
			# internal Flash
			$acc |= 0x2 << 16;	# TEX S CB = 000 0 10
		} elsif ($bas >= 0x20000000 && $bas < 0x40000000) {
			# internal SRAM
			$acc |= 0x6 << 16;	# TEX S CB = 000 1 10
		} elsif ($bas >= 0x60000000 && $bas < 0xa0000000) {
			# external RAM
			$acc |= 0x7 << 16;	# TEX S CB = 000 1 11
		} else {
			# peripheral
			$acc |= 0x5 << 16;	# TEX S CB = 000 1 01
		}

		if ($level == 2) {
			# task window has ID 7, we only have 8 windows
			$id = 7;
		}
		$bas |= 0x10 | $id;	 # set ENABLE and id

		# ignore kernel windows
		if ($level == 0) {
			print $CFGFILE "\t\t\t/* -- not used for kernel -- */\n";
		} else {
			print $CFGFILE "\t\t\t{ .base_region_valid = ", hexify($bas),
			                     ", .attrib_size_enable = ", hexify($siz | $acc), " },\n";
		}
	} elsif ($mpu_arch =~ /ppc_e200/) {
		my $mas1 = 0x80000000 | (0 << 16) | ((ilog($size)-10) << 7);
		my $mas2 = $start;
		my $mas3 = $start;

		# use translation space #1 for user mappings and #0 for kernel mappings
		if ($level > 0) {
			$mas1 |= 0x1000;
		}

		# cache attributes
		if ($c) {
			# hardcoded: WB caching, no memory coherency, big-endian
			$mas2 |= 0x00;
		} else {
			# WIMGE == 01110 (uncached)
			$mas2 |= 0x0e;
		}

		# access permissions
		if ($r) {
			$mas3 |= 0x02;	# user exec, supervisor no read
		}
		if ($w) {
			$mas3 |= 0x08;	# user exec, supervisor no write
		}
		if ($x) {
			$mas3 |= 0x20;	# user exec, supervisor no exec
		}

		if ($level == 0) {
			print $CFGFILE "\t\t\t/* -- not used for kernel -- */\n";
		} else {
			print $CFGFILE "\t\t\t{ .mas1 = ", hexify($mas1),
			                   ", .mas2 = ", hexify($mas2),
			                   ", .mas3 = ", hexify($mas3), " },\n";
		}
	} elsif ($mpu_arch =~ /tricore_tc161_/) {
		my $lower = $start;
		my $upper = $start + $size;
		print $CFGFILE "\t\t\t{ .lower = ", hexify($lower),
		                     ", .upper = ", hexify($upper),," },\n";
	}
}

sub usage
{
	my $ret = shift;
	if (!defined $ret) {
		$ret = 1;
	}

	print "usage:\n";
	print "  ab_gen_mpu_c.pl [-h|--help] [--version]\n";
	print "                  [-m <memory.xml>]\n";
	print "                  -o <mpu.c>\n";
	print "                  <system.xml>\n";
	print "\n";
	print "options:\n";
	print "  -h|--help       print this help text and exit\n";
	print "  --version       print version information and exit\n";
	print "  -m <memory.xml> memory map description (for MPU config)\n";
	print "  -o <mpu.c>      file to create with MPU configuration\n";
	print "  <system.xml>    system description\n";
	print "\n";
	print "note:\n";
	print "  if no memory.xml is provided, only a dummy configuration is generated\n";

	exit $ret;
}

while (defined $ARGV[0]) {
	if ($ARGV[0] eq '--help') {
		usage(0);
	} elsif ($ARGV[0] eq '-h') {
		usage(0);
	} elsif ($ARGV[0] eq '--version') {
		print "version: ", $VERSION, "\n";
		exit 0;
	} elsif ($ARGV[0] eq '-o') {
		shift;
		$cfgfile = shift;
	} elsif ($ARGV[0] eq '-m') {
		shift;
		$memxmlfile = shift;
	} else {
		if (defined $sysxmlfile) {
			die "error: invalid argument '", $ARGV[0], "'\n";
		}
		$sysxmlfile = shift;
	}
}

if (!defined $sysxmlfile) {
	die "error: no xml-file specified\n";
}

if (!defined $cfgfile) {
	die "error: no output file specified\n";
}

# Read MPU Windows
my $mem;
if (defined $memxmlfile) {
	$mem = XMLin($memxmlfile,
				KeyAttr => { },
				ForceArray => ['shm', 'part', 'task', 'rq', 'mpu_window'],
				) or die "opening and parsing of '$memxmlfile' failed!\n";

	if (!defined $mem->{mpu_arch}) {
		die "error: 'memory.xml' does not define MPU architecture\n";
	}
	$mpu_arch = $mem->{mpu_arch};
	$final_run = 1;
}


my $all = XMLin($sysxmlfile,
				KeyAttr => { },
				ForceArray => ['partition', 'task', 'isr', 'hook', 'invokable',
				               'layout', 'invoke',
				               'sched_table', 'expiry', 'action_task', 'action_hook',
				               'action_event', 'action_counter', 'action_invoke',
				               'defaultisr', 'shm', 'shm_access',
				               'kldd', 'ipev', 'alarm', 'counter', 'counter_access'],
				) or die "opening and parsing of '$sysxmlfile' failed!\n";

my $sys = $all->{system};
my $target = $all->{target};

if (!defined $memxmlfile) {
	# fetch mpu_arch from target
	$mpu_arch = $target->{mpu_arch};
}
if (!defined $mpu_arch) {
	die "error: mpu_arch unknown\n";
}

if ($mpu_arch =~ /tricore_tc161_/) {
	$split_code_data = 1;
}
if ($mpu_arch =~ /arm_cortexa/) {
	$gen_pagetables = 1;
}

my $num_cpus = number $target->{cpus};
my $num_parts = 0;
if ($sys->{partition}) {
	$num_parts += @{$sys->{partition}};
}

# iterate partitions to get the overall number of tasks
my $num_tasks = $num_cpus;	# plus idle tasks in idle partitions
for my $part (@{$sys->{partition}}) {
	if (defined $part->{task}) {
		$num_tasks += @{$part->{task}};
	}
	if (defined $part->{isr}) {
		$num_tasks += @{$part->{isr}};
	}
	if (defined $part->{hook}) {
		$num_tasks += @{$part->{hook}};
	}
	if (defined $part->{invokable}) {
		$num_tasks += @{$part->{invokable}};
	}
}


open($CFGFILE, ">$cfgfile") or die "Couldn't open $cfgfile file for writing, $!\n";

print $CFGFILE "/* mpu.c -- AUTOGENERATED -- DO NOT EDIT -- */\n";
print $CFGFILE "/* ", $VERSION, " */\n";
print $CFGFILE "\n";
if ($final_run) {
	print $CFGFILE "/* NOTE: this file contains the final MPU window configuration */\n";
} else {
	print $CFGFILE "/* WARNING: THIS FILE LACKS THE FINAL MPU WINDOW CONFIGURATION */\n";
	print $CFGFILE "/* WARNING: rerun ab_gen_mpu_c.pl with the final memory.xml for a valid configuration */\n";
}
print $CFGFILE "\n";

print $CFGFILE "#include <arch_mpu_state.h>\n";
print $CFGFILE "\n";

# Partition page tables
my $num_pt2_tables = 0;
if ($gen_pagetables) {
	# MMU version -- generate page tables
	print $CFGFILE "/* MMU page tables */\n";

	# ARM uses two-level page tables with a 16K aligned first-level directory
	# and 1K aligned second level directories

	# iterate all partitions
	my $part_id = 0;
	for my $part (@{$sys->{partition}}) {
		my @ws;

		# go thru <part> list ins memory.xml and fetch all matching windows
		# locate according partition in memory.xml
		for my $p (@{$mem->{part}}) {
			next if $p->{name} ne $part->{name} && $p->{name} ne "__KERNEL__";

			# dump all windows
			for my $window (@{$p->{mpu_window}}) {
				my $start = number $window->{start};
				my $size = number $window->{size};
				my $u = $p->{name} ne "__KERNEL__";
				my $r = !!$window->{read}+0;
				my $w = !!$window->{write}+0;
				my $x = !!$window->{exec}+0;
				my $c = !!$window->{cached}+0;
				my $arch = number $window->{arch};

				# NOTE: sometimes, SHMs are not properly aligned ...
				if (($start & 0xfff) != 0 || ($size & 0xfff) != 0) {
					die "partition '", $part->{name}, "' window id '", $window->{id}, "' has unaligned start/size\n";
				}

				#push @ws, [ $start, $size, $u, $r, $w, $x, $c, $arch ];
				push @ws, { start => $start, size => $size, user => $u,
				            read => $r, write => $w, exec => $x,
				            cached => $c, arch => $arch};
			}
		}

		# sort by start address, ascending
		my @wss = sort { $a->{start} <=> $b->{start} } @ws;

		# the following algorithm scans linearly thru the address space
		# and creates page tables for all windows it finds.
		# it does this in two passes: the first pass generates all 2nd level
		# page tables, then generates the first level page table.
		# the page tables are kept in @pts and are referenced by their address
		my $addr = 0;
		my $pos;
		my $level = 1;
		my @pts;
		for my $w (@wss) {
			my $start = $w->{start};
			my $size = $w->{size};
			# ARM page table bits
			# PTE_XN		0x001	/* execute never */
			# PTE_P			0x002	/* present, type: 4K page */
			# PTE_B			0x004	/* buffered */
			# PTE_C			0x008	/* cached */
			# PTE_AP0		0x010	/* AP0 */
			# PTE_AP1		0x020	/* AP1 */
			# PTE_TEX0		0x040	/* TEX0 */
			# PTE_TEX1		0x080	/* TEX1 */
			# PTE_TEX2		0x100	/* TEX2 */
			# PTE_AP2		0x200	/* AP2 */
			# PTE_S			0x400	/* shareable */
			# PTE_NG		0x800	/* non global */
			my $bits = 0x002;	# PTE_P

			# Memory protection (user+rwx)
			#
			# AP 2..0 values
			#     kernel   user usage
			#  000  --      --
			#  001  rw      --  kernel RW
			#  010  rw      ro
			#  011  rw      rw  user RW
			#  100  ??      ??
			#  101  ro      --  kernel R-
			#  110  ro      --
			#  111  ro      ro  user R-
			$bits |= 0x010;	# PTE_AP0
			if ($w->{user}) {
				$bits |= 0x020;	# PTE_AP1
			}
			if (!$w->{write}) {
				$bits |= 0x200;	# PTE_AP2
			}
			if (!$w->{exec}) {
				$bits |= 0x001;	# PTE_XN
			}

			# Cache mode
			#
			# TEX C B  memory type           shareable
			# 000 0 0  UC, strongly ordered  shareable
			# 000 0 1  UC, device            shareable
			# 000 1 0  WT, normal            S
			# 000 1 1  WB, normal            S
			# 001 0 0  UC, normal            S
			# 001 1 1  WBWA, normal          S
			# 010 0 0  non-shared device     not shareable

			my $cachemode = "";
			if ($w->{cached}) {
				if ($mpu_arch =~ /arm_cortexa8/) {
					# Cortex-A8 is always a single core and does not support WBWA
					$cachemode = "WB unshared";
					$bits |= 0x008|0x004;	# PTE_C | PTE_B
				} else {
					# Assume: all others are SMP cores
					$cachemode = "WBWA shareable";
					$bits |= 0x040|0x008|0x004|0x400;	# PTE_TEX0 | PTE_C | PTE_B | PTE_S
				}
			} else {
				$cachemode = "UC strongly ordered";
			}

			#print "HERE, addr:", hexify($addr), ", s:", hexify($start), ", s:", hexify($size), ", $level\n";

			if ($level > 1) {
				# fill up currently open page table with invalid entries
				while ($addr < $start && $pos < 256) {
					print $CFGFILE "\t/* ", hexify($addr)," */\t0,\n";
					$addr += 0x1000;	# 4K
					$pos++;
				}
				if ($pos == 256) {
					print $CFGFILE "};\n\n";
					$level = 1;
				}
			}
			if ($level == 1) {
				die if ($addr & 0xfffff) != 0;
				while ($addr < ($start & 0xfff00000)) {
					$addr += 0x100000;	# 1M
				}
			}
			if ($level == 1 && ($addr & 0xfffff) == 0) {
				# open new page table
				print $CFGFILE "const uint32_t _pt2_part", $part_id, "_", hexify($addr),"[256] __section_cfg_pt2 __aligned(1024) = {\n";
				push @pts, { start => $addr };
				$pos = 0;
				$level = 2;
				$num_pt2_tables++;
			}
			die if ($level == 1);

			# fill with meaning-ful entries
			if ($addr < $start) {
				redo;
			}

			while ($addr < ($start + $size) && $pos < 256) {
				print $CFGFILE "\t/* ", hexify($addr)," */\t", hexify($addr);
				print $CFGFILE " | ", sprintf("0x%03x", $bits), ", /* ";
				print $CFGFILE $w->{user}?'u':'-';
				print $CFGFILE $w->{read}?'r':'-';
				print $CFGFILE $w->{write}?'w':'-';
				print $CFGFILE $w->{exec}?'x':'-';
				print $CFGFILE " ", $cachemode, " */\n";
				$addr += 0x1000;	# 4K
				$pos++;
			}
			if ($pos == 256) {
				redo;
			}
		}
		# close last open page table
		if ($level > 1) {
			print $CFGFILE "};\n\n";
		}

		# now emit 1st level page table
		my @ptss = sort { $a->{start} <=> $b->{start} } @pts;
		$addr = 0;
		print $CFGFILE "const uint32_t _pt1_part", $part_id, "[4096] __section_cfg_pt1 __aligned(16384) = {\n";
		for my $p (@ptss) {
			my $start = $p->{start};
			while ($addr < $start) {
				print $CFGFILE "\t/* ", hexify($addr)," */\t0,\n";
				$addr += 0x100000;	# 1M
			}
			# NOTE: cannot use "|" in relocs, must use "+" to concat address and page table bits!
			print $CFGFILE "\t/* ", hexify($addr)," */\t(uint32_t)_pt2_part", $part_id, "_", hexify($addr), " + 0x1,\n";
			$addr += 0x100000;	# 1M
		}
		print $CFGFILE "};\n\n";
		$part_id++;
	}

	# generate dummy level 2 tables for proper alignment
	# FIXME: $num_parts * 8 is a rough metric to get the number of page tables,
	# FIXME: but better add a config element to configure this number!
	while ($num_pt2_tables < $num_parts * 8) {
		print $CFGFILE "const uint32_t _pt2_unused_", $num_pt2_tables, "[256] __section_cfg_pt2 __aligned(1024) = {\n";
		print $CFGFILE "};\n\n";
		$num_pt2_tables++;
	}
}

print $CFGFILE "/* MPU settings for all partitions (one per partition) */\n";
print $CFGFILE "const struct arch_mpu_part_cfg mpu_part_cfg[", $num_cpus + $num_parts, "] = {\n";
if ($gen_pagetables) {
	# MMU version
	# idle partitions share the first partition's page table
	for (my $i = 0; $i < $num_cpus; $i++) {
		print $CFGFILE "\t/* idle partition on CPU $i */ {\n";
		print $CFGFILE "\t\t.ttbr0 = (uint32_t)&_pt1_part0,\n";
		print $CFGFILE "\t\t.asid = 0,\n";
		print $CFGFILE "\t},\n";
	}

	# iterate all partitions
	my $part_id = 0;
	for my $part (@{$sys->{partition}}) {
		print $CFGFILE "\t/* Partition '", $part->{name}, "' */ {\n";
		print $CFGFILE "\t\t.ttbr0 = (uint32_t)&_pt1_part", $part_id, ",\n";
		print $CFGFILE "\t\t.asid = ", ($part_id + 1), ",\n";
		print $CFGFILE "\t},\n";
		$part_id++;
	}
} elsif ($final_run) {
	# MPU version
	# generate empty entries for idle partitions
	for (my $i = 0; $i < $num_cpus; $i++) {
		print $CFGFILE "\t/* idle partition on CPU $i */ {\n";
		# emit at least a .region/.data entry, or the compiler warns!
		if ($split_code_data) {
			print $CFGFILE "\t\t.data = {\n";
			print $CFGFILE "\t\t},\n";
		} else {
			print $CFGFILE "\t\t.region = {\n";
			print $CFGFILE "\t\t},\n";
		}
		print $CFGFILE "\t},\n";
	}

	# iterate all partitions
	for my $part (@{$sys->{partition}}) {
		print $CFGFILE "\t/* Partition '", $part->{name}, "' */ {\n";
		if ($split_code_data) {
			print $CFGFILE "\t\t.data = {\n";
			# locate according partition in memory.xml
			for my $p (@{$mem->{part}}) {
				next if $p->{name} ne $part->{name};

				# dump all windows
				for my $window (@{$p->{mpu_window}}) {
					# filter out exec-only windows
					my $r = !!$window->{read};
					my $w = !!$window->{write};
					next if !$r && !$w;

					print_mpu_window($window, 1);
				}
			}
			print $CFGFILE "\t\t},\n";

			print $CFGFILE "\t\t.code = {\n";
			# locate according partition in memory.xml
			for my $p (@{$mem->{part}}) {
				next if $p->{name} ne $part->{name};

				# dump all windows
				for my $window (@{$p->{mpu_window}}) {
					# filter out non-exec windows
					my $x = !!$window->{exec};
					next if !$x;

					print_mpu_window($window, 1);
				}
			}
			print $CFGFILE "\t\t},\n";
		} else {
			print $CFGFILE "\t\t.region = {\n";
			# locate according partition in memory.xml
			for my $p (@{$mem->{part}}) {
				next if $p->{name} ne $part->{name};

				# dump all windows
				for my $window (@{$p->{mpu_window}}) {
					print_mpu_window($window, 1);
				}
			}
			print $CFGFILE "\t\t},\n";
		}
		print $CFGFILE "\t},\n";
	}
} else {
	print $CFGFILE "\t/* empty MPU configuration */\n";
}
print $CFGFILE "};\n";
print $CFGFILE "\n";


# Task Config
print $CFGFILE "/* MPU settings for all tasks (one per task) */\n";
print $CFGFILE "const struct arch_mpu_task_cfg mpu_task_cfg[", $num_tasks, "] = {\n";
if ($final_run && !$gen_pagetables) {
	# generate empty entries for idle partitions
	for (my $i = 0; $i < $num_cpus; $i++) {
		print $CFGFILE "\t/* idle task on CPU $i */ {\n";
		# emit at least a .region/.data entry, or the compiler warns!
		if ($split_code_data) {
			print $CFGFILE "\t\t.data = {\n";
			print $CFGFILE "\t\t},\n";
			if ($mpu_arch =~ /tricore_tc161_/) {
				print $CFGFILE "\t\t.dpwe = 0,\n";
			}
		} else {
			print $CFGFILE "\t\t.region = {\n";
			print $CFGFILE "\t\t},\n";
		}
		print $CFGFILE "\t},\n";
	}

	# iterate all partitions
	for my $part (@{$sys->{partition}}) {
		# locate according partition in memory.xml
		for my $p (@{$mem->{part}}) {
			next if $p->{name} ne $part->{name};

			# iterate all partition windows again to find writeable ones
			my $tricore_part_bits = 0;
			my $tricore_part_shift = 0;
			if ($mpu_arch =~ /tricore_tc161_/) {
				for my $window (@{$p->{mpu_window}}) {
					# filter out exec-only windows
					my $r = !!$window->{read};
					my $w = !!$window->{write};
					next if !$r && !$w;

					$tricore_part_bits |= ($w << $tricore_part_shift);
					$tricore_part_shift++;
				}
			}

			# iterate all tasks
			for my $task (@{$part->{task}}) {
				print $CFGFILE "\t/* Partition '", $part->{name}, "' Task '", $task->{name}, "' */ {\n";
				if ($split_code_data) {
					print $CFGFILE "\t\t.data = {\n";
				} else {
					print $CFGFILE "\t\t.region = {\n";
				}

				my $tricore_task_bits = 0;
				my $tricore_task_shift = 10;

				# locate according task in memory.xml
				for my $t (@{$p->{task}}) {
					next if $t->{name} ne $task->{name};

					# dump all windows
					for my $window (@{$t->{mpu_window}}) {
						# filter out exec-only windows in split models
						my $r = !!$window->{read};
						my $w = !!$window->{write};
						next if $split_code_data && !$r && !$w;

						$tricore_task_bits |= ($w << $tricore_task_shift);
						$tricore_task_shift++;

						print_mpu_window($window, 2);
					}
				}
				print $CFGFILE "\t\t},\n";
				if ($mpu_arch =~ /tricore_tc161_/) {
					my $u_bits = $tricore_task_bits | $tricore_part_bits;
					print $CFGFILE "\t\t.dpwe = ", hexify($u_bits), ",\n";
				}
				print $CFGFILE "\t},\n";
			}

			# iterate all ISRs
			for my $isr (@{$part->{isr}}) {
				print $CFGFILE "\t/* Partition '", $part->{name}, "' ISR '", $isr->{name}, "' */ {\n";
				if ($split_code_data) {
					print $CFGFILE "\t\t.data = {\n";
				} else {
					print $CFGFILE "\t\t.region = {\n";
				}

				my $tricore_task_bits = 0;
				my $tricore_task_shift = 10;

				# locate according ISR-task in memory.xml
				for my $t (@{$p->{task}}) {
					next if $t->{name} ne $isr->{name};

					# dump all windows
					for my $window (@{$t->{mpu_window}}) {
						# filter out exec-only windows in split models
						my $r = !!$window->{read};
						my $w = !!$window->{write};
						next if $split_code_data && !$r && !$w;

						$tricore_task_bits |= ($w << $tricore_task_shift);
						$tricore_task_shift++;

						print_mpu_window($window, 2);
					}
				}
				print $CFGFILE "\t\t},\n";
				if ($mpu_arch =~ /tricore_tc161_/) {
					my $u_bits = $tricore_task_bits | $tricore_part_bits;
					print $CFGFILE "\t\t.dpwe = ", hexify($u_bits), ",\n";
				}
				print $CFGFILE "\t},\n";
			}

			# iterate all hooks
			for my $hook (@{$part->{hook}}) {
				print $CFGFILE "\t/* Partition '", $part->{name}, "' hook '", $hook->{name}, "' */ {\n";
				if ($split_code_data) {
					print $CFGFILE "\t\t.data = {\n";
				} else {
					print $CFGFILE "\t\t.region = {\n";
				}

				my $tricore_task_bits = 0;
				my $tricore_task_shift = 10;

				# locate according hook-task in memory.xml
				for my $t (@{$p->{task}}) {
					next if $t->{name} ne $hook->{name};

					# dump all windows
					for my $window (@{$t->{mpu_window}}) {
						# filter out exec-only windows in split models
						my $r = !!$window->{read};
						my $w = !!$window->{write};
						next if $split_code_data && !$r && !$w;

						$tricore_task_bits |= ($w << $tricore_task_shift);
						$tricore_task_shift++;

						print_mpu_window($window, 2);
					}
				}
				print $CFGFILE "\t\t},\n";
				if ($mpu_arch =~ /tricore_tc161_/) {
					my $u_bits = $tricore_task_bits | $tricore_part_bits;
					print $CFGFILE "\t\t.dpwe = ", hexify($u_bits), ",\n";
				}
				print $CFGFILE "\t},\n";
			}

			# iterate all invokables
			for my $invokable (@{$part->{invokable}}) {
				print $CFGFILE "\t/* Partition '", $part->{name}, "' invokable '", $invokable->{name}, "' */ {\n";
				if ($split_code_data) {
					print $CFGFILE "\t\t.data = {\n";
				} else {
					print $CFGFILE "\t\t.region = {\n";
				}

				my $tricore_task_bits = 0;
				my $tricore_task_shift = 10;

				# locate according invokable-task in memory.xml
				for my $t (@{$p->{task}}) {
					next if $t->{name} ne $invokable->{name};

					# dump all windows
					for my $window (@{$t->{mpu_window}}) {
						# filter out exec-only windows in split models
						my $r = !!$window->{read};
						my $w = !!$window->{write};
						next if $split_code_data && !$r && !$w;

						$tricore_task_bits |= ($w << $tricore_task_shift);
						$tricore_task_shift++;

						print_mpu_window($window, 2);
					}
				}
				print $CFGFILE "\t\t},\n";
				if ($mpu_arch =~ /tricore_tc161_/) {
					my $u_bits = $tricore_task_bits | $tricore_part_bits;
					print $CFGFILE "\t\t.dpwe = ", hexify($u_bits), ",\n";
				}
				print $CFGFILE "\t},\n";
			}
		}
	}
} else {
	print $CFGFILE "\t/* empty MPU configuration */\n";
}
print $CFGFILE "};\n";
print $CFGFILE "\n";

# Generate SHM configuration
print $CFGFILE "/* SHM configuration */\n";
my $num_shms = 0;
if ($sys->{shm}) {
	$num_shms += @{$sys->{shm}};
}
print $CFGFILE "const struct shm_cfg shm_cfg[", $num_shms, "] = {\n";
if ($final_run && $num_shms > 0) {
	# iterate all partitions
	for my $shm (@{$sys->{shm}}) {
		print $CFGFILE "\t/* SHM '", $shm->{name}, "' */ {\n";
		# locate according SHM in memory.xml
		for my $s (@{$mem->{shm}}) {
			next if $s->{name} ne $shm->{name};

			print $CFGFILE "\t\t.base = ", $s->{start}, ",\n";
			print $CFGFILE "\t\t.size = ", $s->{size}, ",\n";
		}
		print $CFGFILE "\t},\n";
	}
} else {
	print $CFGFILE "\t/* empty SHM configuration */\n";
}
print $CFGFILE "};\n";


# config file generation complete
close($CFGFILE) or die "Couldn't close $cfgfile, $!\n";
