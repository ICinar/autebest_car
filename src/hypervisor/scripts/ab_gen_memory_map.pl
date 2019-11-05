#!/usr/bin/perl -w
#
# ab_gen_memory_map.pl - generate simple memory map in memory.xml
#
# NOTE: requires the XML::Simple CPAN module
#       on Ubuntu 12.04, try:  sudo apt-get install libxml-simple-perl
#
# Usage: ab_gen_memory_map.pl -hw hardware.xml -o new_memory.xml memory.xml
#
# azuepke, 2014-08-18: initial (cloned from genconfig.pl)
# azuepke, 2014-08-19: add 4K-MMU mode and emit MPU windows
# azuepke, 2015-08-05: handle cached attribute


use strict;
use warnings "all";
use XML::Simple;
use Data::Dumper;

# tool version ID
my $VERSION = "ab_gen_memory_map.pl 2015-08-05";

# global variables
my %hwhash;
my %shmhash;
my %rqhash;
my $mpu_arch;

# Window sizing rules
my $minalign;
my $alignedmode;
my $maxalign;
my $alignshift;


my $xmlfile;
my $hwxmlfile;
my $outxmlfile;
my $verbose = 0;

################################################################################

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

# Align a value
# Usage: $val = alignup($val, 0x1000);
sub alignup
{
	my $addr = shift;
	my $align = shift;

	return (($addr) + ($align) - 1) & ~(($align) - 1);
}

# check if two memory regions [x, x+xs) and [y, y+ys) overlap
# Usage: check_overlap(x, xs, y, ys) with xs and ys describing region sizes
sub check_overlap
{
	my $x = shift;
	my $xs = shift;
	my $y = shift;
	my $ys = shift;

	my $xe = $x+$xs;
	my $ye = $y+$ys;

	return ($x  >= $y && $x  < $ye) ||		# is x    in [y, y+ys)
	       ($xe >  $y && $xe < $ye) ||		# is x+xs in [y, y+ys)
	       ($y  >= $x && $y  < $xe) ||		# is y    in [x, x+xs)
	       ($ye >  $x && $ye < $xe);		# is y+ys in [x, x+xs)
}

# find largest aligned chunk, given a start address and size
# Usage: $chunk = find_largest_aligned_chunk($start, $size)
sub find_largest_aligned_chunk
{
	my $start = shift;
	my $size = shift;
	my $chunk = $maxalign;

	while ($chunk) {
		my $mask = $chunk - 1;

		if ((($start & $mask) == 0) && ($size >= $chunk)) {
			return $chunk;
		}

		$chunk >>= $alignshift;
	}

	die "internal error, cannot happen";
}

################################################################################

# Add hardware element to list (fix or pool)
# Usage: add_hw_node($xml_node, $ispool)
sub add_hw_node
{
	my $node = shift;
	my $ispool = shift;

	my $type = "fix";
	if ($ispool) {
		$type = "pool";
	}

	my $name = $node->{name};
	if (defined $hwhash{$name}) {
		die "error: $hwxmlfile: <$type> '$name' already exists\n";
	}

	# start and size are mandatory attributes
	if (!defined $node->{start}) {
		die "error: $hwxmlfile: <$type> '$name' lacks 'start'\n";
	}
	if (!defined $node->{size}) {
		die "error: $hwxmlfile: <$type> '$name' lacks 'size'\n";
	}

	my $start = number $node->{start};
	my $size = number $node->{size};
	my $allocptr = $start;

	if ($size == 0) {
		die "error: $hwxmlfile: <$type> '$name' has zero 'size'\n";
	}

	# consistency checks
	while (my ($othername, $otherres) = each %hwhash) {
		my $otherispool = $otherres->[0];

		# fix may overlap with other fixes
		next if !$ispool && !$otherispool;

		# fix must not overlap with other pools
		# pool must not overlap with other fixes
		# pool must not overlap with other pools
		if (check_overlap($start, $size, $otherres->[1], $otherres->[2])) {
			my $type2 = "fix";
			if ($otherispool) {
				$type2 = "pool";
			}
			die "error: $hwxmlfile: <$type> '$name' overlaps with <$type2> '$othername'\n";
		}
	}

	my $read = 1;
	my $write = 0;
	my $exec = 0;
	my $cached = 0;
	if (defined $node->{read}) {
		$read = 0 + !!(number $node->{read});
	}
	if (defined $node->{write}) {
		$write = 0 + !!(number $node->{write});
	}
	if (defined $node->{exec}) {
		$exec = 0 + !!(number $node->{exec});
	}
	if (defined $node->{cached}) {
		$cached = 0 + !!(number $node->{cached});
	}

	if ($verbose) {
		print "hw.$type: $name, start: " . hexify($start) . ", size: " . hexify($size). ", rwx: $read$write$exec, cached: $cached\n";
	}
	$hwhash{$name} = [ $ispool, $start, $size, $allocptr, $read, $write, $exec, $cached ];
}

################################################################################

# Allocate memory from pool (or assign fix)
# usage: ($start, $size) = allocate_from_pool($res, $poolname, $minsize, $align, $errorname)
sub allocate_from_pool
{
	my $res = shift;
	my $poolname = shift;
	my $minsize = shift;
	my $align = shift;
	my $errorname = shift;

	my $start;
	my $size;

	if ($res->[0]) {
		# allocate from pool
		$start = alignup($res->[3], $align);
		$size = alignup($minsize, $align);
		my $realsize = $start + $minsize - $res->[3];
		my $free = $res->[1] + $res->[2] - $res->[3];
		if ($realsize > $free) {
			die "error: $errorname: cannot allocate $realsize bytes from '$poolname', only $free bytes left\n";
		}
		$res->[3] += $realsize;
	} else {
		# not a pool, return
		$start = $res->[1];
		$size = $res->[2];
	}

	return ($start, $size);
}

################################################################################

# Add memory requirement
# usage: add_rq($partname, $node)
sub add_rq
{
	my $partname = shift;
	my $node = shift;

	my $nodename = $node->{name};
	my $name = $partname . "::" . $nodename;

	if (defined $rqhash{$name}) {
		die "error: $xmlfile: <part> '$partname' <rq> '$nodename' already exists\n";
	}

	# resolve resource (HW or SHM)
	my $poolname = $node->{resource};
	my $res;
	if (defined $hwhash{$poolname}) {
		$res = $hwhash{$poolname};
	} elsif (defined $shmhash{$poolname}) {
		$res = $shmhash{$poolname};
	} else {
		die "error: $xmlfile: <part> '$partname' <rq> '$nodename' refers to unknown resource '$poolname'\n";
	}
	my $ispool = $res->[0];

	# check minsize and align
	my $minsize = 0;
	my $align = $minalign;
	if (defined $node->{minsize}) {
		$minsize = number $node->{minsize};
	}
	if (defined $node->{align}) {
		$align = number $node->{align};
	}
	if ($ispool && !defined $node->{minsize}) {
		die "error: $xmlfile: <part> '$partname' <rq> '$nodename' refers to pool, but lacks 'minsize'\n";
	}
	if ($ispool && $minsize < 1) {
		die "error: $xmlfile: <part> '$partname' <rq> '$nodename': 'minsize' too small\n";
	}
	if ($align == 0 || ($align & ($align - 1))) {
		die "error: $xmlfile: <part> '$partname' <rq> '$nodename': 'align' is not a power of 2\n";
	}
	if ($align < $minalign) {
		$align = $minalign;
	}

	# rwx
	my $default_read = $res->[4];
	my $default_write = $res->[5];
	my $default_exec = $res->[6];
	my $default_cached = $res->[7];
	my $read = $default_read;
	my $write = $default_write;
	my $exec = $default_exec;
	my $cached = $default_cached;
	if (defined $node->{read}) {
		$read = 0 + !!(number $node->{read});
		if (!$default_read && $read) {
			die "error: $xmlfile: <part> '$partname' <rq> '$nodename': 'read' is set, but not in referenced resource '$poolname'\n";
		}
	}
	if (defined $node->{write}) {
		$write = 0 + !!(number $node->{write});
		if (!$default_write && $write) {
			die "error: $xmlfile: <part> '$partname' <rq> '$nodename': 'write' is set, but not in referenced resource '$poolname'\n";
		}
	}
	if (defined $node->{exec}) {
		$exec = 0 + !!(number $node->{exec});
		if (!$default_exec && $exec) {
			die "error: $xmlfile: <part> '$partname' <rq> '$nodename': 'exec' is set, but not in referenced resource '$poolname'\n";
		}
	}
	if (defined $node->{cached}) {
		$cached = 0 + !!(number $node->{cached});
		if (!$default_cached && $cached) {
			die "error: $xmlfile: <part> '$partname' <rq> '$nodename': 'cached' is set, but not in referenced resource '$poolname'\n";
		}
	}

	if ($verbose) {
		print "rq: $name, minsize: " . hexify($minsize). ", align: " . hexify($align) . ", rwx: $read$write$exec, cached: $cached\n";
	}

	# allocate memory resources
	my ($start, $size) = allocate_from_pool($res, $poolname, $minsize, $align, "<part> '$partname' <node> '$nodename'");
	my $allocptr = $start;

	if ($verbose) {
		print "rq: $name, start: " . hexify($start) . ", size: " . hexify($size). ", rwx: $read$write$exec, cached: $cached\n";
	}

	# use same structure than %hwhash entries!
	$rqhash{$name} = [ 0, $start, $size, $allocptr, $read, $write, $exec, $cached ];

	# XXX -- add XML attributes for start and size
	$node->{start} = hexify $start;
	$node->{size} = hexify $size;
	$node->{read} = $read;
	$node->{write} = $write;
	$node->{exec} = $exec;
	$node->{cached} = $cached;
}

################################################################################

# Add shared memory
# usage: add_shm($node)
sub add_shm
{
	my $node = shift;

	my $name = $node->{name};
	if (defined $hwhash{$name}) {
		die "error: $xmlfile: <shm> '$name' ambiguous, <fix> or <pool> with same name already exists\n";
	}
	if (defined $shmhash{$name}) {
		die "error: $xmlfile: <shm> '$name' already exists\n";
	}

	# resolve resource
	my $poolname = $node->{resource};
	if (!defined $hwhash{$poolname}) {
		die "error: $xmlfile: <shm> '$name' refers to unknown resource '$poolname'\n";
	}
	my $res = $hwhash{$poolname};
	my $ispool = $res->[0];

	# check minsize and align
	my $minsize = 0;
	my $align = $minalign;
	if (defined $node->{minsize}) {
		$minsize = number $node->{minsize};
	}
	if (defined $node->{align}) {
		$align = number $node->{align};
	}
	if ($ispool && !defined $node->{minsize}) {
		die "error: $xmlfile: <shm> '$name' refers to pool, but lacks 'minsize'\n";
	}
	if ($ispool && $minsize < 1) {
		die "error: $xmlfile: <shm> '$name': 'minsize' too small\n";
	}
	if ($align == 0 || ($align & ($align - 1))) {
		die "error: $xmlfile: <shm> '$name': 'align' is not a power of 2\n";
	}
	if ($align < $minalign) {
		$align = $minalign;
	}

	# rwx
	my $default_read = $res->[4];
	my $default_write = $res->[5];
	my $default_exec = $res->[6];
	my $default_cached = $res->[7];
	my $read = $default_read;
	my $write = $default_write;
	my $exec = $default_exec;
	my $cached = $default_cached;
	if (defined $node->{read}) {
		$read = 0 + !!(number $node->{read});
		if (!$default_read && $read) {
			die "error: $xmlfile: <shm> '$name': 'read' is set, but not in referenced resource '$poolname'\n";
		}
	}
	if (defined $node->{write}) {
		$write = 0 + !!(number $node->{write});
		if (!$default_write && $write) {
			die "error: $xmlfile: <shm> '$name': 'write' is set, but not in referenced resource '$poolname'\n";
		}
	}
	if (defined $node->{exec}) {
		$exec = 0 + !!(number $node->{exec});
		if (!$default_exec && $exec) {
			die "error: $xmlfile: <shm> '$name': 'exec' is set, but not in referenced resource '$poolname'\n";
		}
	}
	if (defined $node->{cached}) {
		$cached = 0 + !!(number $node->{cached});
		if (!$default_cached && $cached) {
			die "error: $xmlfile: <shm> '$name': 'cached' is set, but not in referenced resource '$poolname'\n";
		}
	}


	if ($verbose) {
		print "shm: $name, minsize: " . hexify($minsize). ", align: " . hexify($align) . ", rwx: $read$write$exec, cached: $cached\n";
	}

	# allocate memory resources
	my ($start, $size) = allocate_from_pool($res, $poolname, $minsize, $align, "<shm> '$name'");
	my $allocptr = $start;

	if ($verbose) {
		print "shm: $name, start: " . hexify($start) . ", size: " . hexify($size). ", rwx: $read$write$exec, cached: $cached\n";
	}

	# use same structure than %hwhash entries!
	$shmhash{$name} = [ 0, $start, $size, $allocptr, $read, $write, $exec, $cached ];

	# XXX -- add XML attributes for start and size
	$node->{start} = hexify $start;
	$node->{size} = hexify $size;
	$node->{read} = $read;
	$node->{write} = $write;
	$node->{exec} = $exec;
	$node->{cached} = $cached;
}

################################################################################

# Perform memory allocation on memory.xml, based on hardware.xml
# Usage: do_memory_allocations()
sub do_memory_allocations
{
	# read hardware.xml and fill hwhash from <fix> and <pool> entries

	my $hw = XMLin($hwxmlfile,
					KeyAttr => { },
					ForceArray => ['fix', 'pool'],
					) or die "opening and parsing of '$hwxmlfile' failed!\n";

	# MPU architecture check
	$mpu_arch = $hw->{mpu_arch};
	if ($mpu_arch ne "generic_no_mpu" &&
	    $mpu_arch ne "generic_mmu_4k" &&
	    $mpu_arch ne "arm_cortexr4_8regions" &&
	    $mpu_arch ne "arm_cortexr4_12regions" &&
	    $mpu_arch ne "ppc_e200z4_16tlbs" &&
	    $mpu_arch ne "ppc_e200z6_32tlbs") {
		die "error: unsupported MPU architecture '$mpu_arch', " .
		    "the following types are supported: ".
		    "generic_no_mpu, generic_mmu_4k, " .
		    "arm_cortexm3_8regions, " .
		    "arm_cortexm4_8regions, " .
		    "arm_cortexm7_8regions, " .
		    "arm_cortexm7_16regions, " .
		    "arm_cortexr4_8regions, " .
		    "arm_cortexr4_12regions, " .
		    "arm_cortexr5_12regions, " .
		    "arm_cortexr5_16regions, " .
		    "arm_cortexr7_12regions, " .
		    "arm_cortexr7_16regions, " .
		    "ppc_e200z4_16tlbs, ppc_e200z6_32tlbs\n";
	}
	if ($mpu_arch eq "generic_mmu_4k") {
		$minalign = 4096;
		$alignedmode = 1;
		$maxalign = 0x10000000;	# 256 MB
		$alignshift = 2;
	} elsif ($mpu_arch eq "ppc_e200z4_16tlbs") {
		# newer e200 cores support 1K page size
		$minalign = 1024;
		$alignedmode = 1;
		$maxalign = 0x80000000;	# 2 GB (could be 4 GB)
		$alignshift = 2;
	} elsif ($mpu_arch eq "ppc_e200z6_32tlbs") {
		$minalign = 4096;
		$alignedmode = 1;
		$maxalign = 0x10000000;	# 256 MB
		$alignshift = 2;
	} elsif (substr($mpu_arch, 0, 10) eq "arm_cortex") {
		$minalign = 256;
		$alignedmode = 1;
		$maxalign = 0x80000000;	# 2 GB (could be 4 GB)
		$alignshift = 1;
	} else {
		$minalign = 16;
		$alignedmode = 0;
		$maxalign = 0x80000000;
		$alignshift = 1;
	}

	# iterate <fix> and <pool>
	for my $fix (@{$hw->{fix}}) {
		add_hw_node($fix, 0);
	}
	for my $pool (@{$hw->{pool}}) {
		add_hw_node($pool, 1);
	}

	undef $hw;


	# read memory.xml
	my $mem = XMLin($xmlfile,
					KeyAttr => { },
					ForceArray => ['shm', 'part', 'task', 'rq'],
					) or die "opening and parsing of '$xmlfile' failed!\n";

	if (defined $mem->{mpu_arch}) {
		if ($mem->{mpu_arch} ne $mpu_arch) {
			die "error: '$xmlfile' uses a different MPU architecture than '$hwxmlfile'\n";
		}
	} else {
		# Set mpu_arch for further scripts
		$mem->{mpu_arch} = $mpu_arch;
	}

	# iterate kernel's nodes first (kernel must be first in ROM)
	if ($mem->{part}[0]->{name} eq "__KERNEL__") {
		for my $rq (@{$mem->{part}[0]->{rq}}) {
			add_rq("__KERNEL__", $rq);
		}
	}

	# iterate <shm>
	for my $shm (@{$mem->{shm}}) {
		add_shm($shm);
	}

	# iterate other partitions
	for my $part (@{$mem->{part}}) {
		my $partname = $part->{name};

		# skip over kernel
		next if $partname eq "__KERNEL__";

		for my $rq (@{$part->{rq}}) {
			add_rq($partname, $rq);
		}
	}

	# iterate other partitions and requirements to generate MPU window list
	for my $part (@{$mem->{part}}) {
		my $id = 0;
		my @windows;

		for my $node (@{$part->{rq}}) {
			my $rq = $rqhash{$part->{name} . "::" . $node->{name}};

			my $start = $rq->[1];
			my $size = $rq->[2];
			my $read = $rq->[4];
			my $write = $rq->[5];
			my $exec = $rq->[6];
			my $cached = $rq->[7];
			my $arch = 0;

			while ($size > 0) {
				my $chunk;
				if ($alignedmode) {
					if (($start & ($minalign-1)) != 0) {
						die "error: <part> '", $part->{name}, "' <rq> '",
						    $node->{name}, "': unaligned 'start', ", hexify($start), "\n";
					}
					if ($size & ($minalign-1)) {
						die "error: <part> '", $part->{name}, "' <rq> '",
						    $node->{name}, "': unaligned 'size', ", hexify($size), "\n";
					}
					$chunk = find_largest_aligned_chunk($start, $size);
				} else {
					# no_mpu: just emit a single window
					$chunk = $size;
				}

				push @windows, {id => $id,
				                start => hexify($start), size => hexify($chunk),
				                read => $read, write => $write, exec => $exec,
				                cached => $cached, arch => hexify($arch)};

				$start += $chunk;
				$size -= $chunk;
				$id++;
			}
		}
		$part->{mpu_window} = \@windows;
	}

	# create outxmlfile
	if (defined $outxmlfile) {
		XMLout($mem,
		       OutputFile => $outxmlfile,
		       RootName => 'memory_layout',
		       KeyAttr => { }
		      ) or die "creating of '$outxmlfile' failed!\n";
	}
}

sub usage
{
	my $ret = shift;
	if (!defined $ret) {
		$ret = 1;
	}

	print "usage:\n";
	print "  ab_gen_memory_map.pl [-h|--help] [--version]\n";
	print "                       [-v]\n";
	print "                       -hw <hardware.xml>\n";
	print "                       [-o <memory.xml>]\n";
	print "                       <memory.xml>\n";
	print "\n";
	print "options:\n";
	print "  -h|--help          print this help text and exit\n";
	print "  --version          print version information and exit\n";
	print "  -v                 verbosity level, increases for each -v\n";
	print "  -hw <hardware.xml> referenced hardware description\n";
	print "  -o <memory.xml>    create new, finalized memory requirements\n";
	print "  <memory.xml>       memory requirements\n";

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
	} elsif ($ARGV[0] eq '-v') {
		shift;
		$verbose++;
	} elsif ($ARGV[0] eq '-hw') {
		shift;
		$hwxmlfile = shift;
	} elsif ($ARGV[0] eq '-o') {
		shift;
		$outxmlfile = shift;
	} else {
		if (defined $xmlfile) {
			die "error: invalid argument '", $ARGV[0], "'\n";
		}
		$xmlfile = shift;
	}
}

if (!defined $xmlfile) {
	die "error: no memory.xml-file specified\n";
}

if (!defined $hwxmlfile) {
	die "error: no hardware.xml-file specified\n";
}

do_memory_allocations();
exit 0;
