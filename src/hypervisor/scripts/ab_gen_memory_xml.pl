#!/usr/bin/perl -w
#
# ab_gen_memory_xml.pl - generate default memory requirements from system.xml
#
# NOTE: requires the XML::Simple CPAN module
#       on Ubuntu 12.04, try:  sudo apt-get install libxml-simple-perl
#
# Usage: ab_gen_memory_xml.pl -s nm-tool -o memory.xml system.xml
#
# azuepke, 2014-08-18: initial (cloned from genconfig.pl)
# azuepke, 2014-08-19: don't do relocation ourself
# azuepke, 2014-09-29: SHM support
# azuepke, 2015-02-23: fixed I/O mappings in system.xml
# azuepke, 2015-03-24: per-CPU RAM blocks
# azuepke, 2015-06-15: ignore sections for unconfigured cores
# azuepke, 2015-06-26: configurable section names
# azuepke, 2015-08-04: have MPU-architecture
# azuepke, 2015-08-05: handle cached attribute


use strict;
use warnings "all";
use XML::Simple;
use IPC::Open2;
use Data::Dumper;
use POSIX;

# tool version ID
my $VERSION = "ab_gen_memory_xml.pl 2015-08-05";

my $nm = 'nm';
my $num_cpus;
my $mpu_arch;

# Read symbols from ELF file (uses "nm" internally)
# Usage: my %symhash = sym_readelf('/path/to/nm', 'file.elf');
sub sym_readelf
{
	my @inputstream;
	my %h;

	# get all symbols
	open2(\*INPUTSTREAM, undef, $_[0], '--extern-only', '--defined-only', '--print-size', $_[1]);
	@inputstream = <INPUTSTREAM>;
	close INPUTSTREAM;

	# filter and prepare
	foreach (@inputstream) {
		chomp $_;
		my @l = split(/\s/, $_);
		my $n;
		my $a;
		my $s;
		if (scalar(@l) == 4) {
			$n = $l[3];
			$a = hex $l[0];
			$s = hex $l[1];
		} else {
			# object has no known size, assume 0
			$n = $l[2];
			$a = hex $l[0];
			$s = 0;
		}
		#print $n, ": ", $a, " sz ",  $s, "\n";
		@l = ($a, $s);
		$h{$n} = \@l;
	}

	return %h;
}

# Evaluate symbol expression
# Usage: my $addr = sym_eval(\%symhash, $expression);
# NOTE: the parser evaluates expressions from left to right in a single pass
#   printf                  <- address of printf
#   main + 0x1000 + 48      <- address of main + 4144 bytes
#   /stack - 16             <- end of stack - 16 bytes
#   0x12345                 <- const address 0x12345
# NOTE: spaces are required between the sub-expressions!
sub sym_eval
{
	my $href = shift;
	my $expr = shift;
	my $result = 0;
	my $neg = 1;
	my @s;

	if (!defined $expr) {
		die "error: unnamed symbol, assuming 0\n";
		return 0;
	}

	my @l = split(/\s/, $expr);
	foreach (@l) {
		if (substr ($_, 0, 1) eq '/') {
			# /symbol
			$_ = substr ($_, 1);
			if (!defined $href->{$_}) {
				die "error: undefined symbol '", $_, "', assuming 0\n";
				return 0;
			}
			@s = $href->{$_};
			#print "sz: ", $s[0][0], ", ", $s[0][1], "\n";
			$result += $neg * ($s[0][0] + $s[0][1]);
			$neg = 1;
		} elsif ($_ eq '+') {
			# plus
			$neg = 1;
		} elsif ($_ eq '-') {
			# minus
			$neg = -1;
		} elsif (substr ($_, 0, 2) eq '0x') {
			# hex digits
			$result += $neg * hex($_);
			$neg = 1;
		} elsif (/^\d/) {
			# dec digits
			$result += $neg * int($_);
			$neg = 1;
		} else {
			# symbol
			if (!defined $href->{$_}) {
				die "error: undefined symbol '", $_, "', assuming 0\n";
				return 0;
			}
			@s = $href->{$_};
			#print "sz: ", $s[0][0], ", ", $s[0][1], "\n";
			$result += $neg * $s[0][0];
			$neg = 1;
		}
	}

	return $result;
}

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

# Get ROM and RAM sizes from ELF binary
# Usage: @sizes = get_required_rom_and_ram_from_elf($filename, $layout, $default_cpu)
sub get_required_rom_and_ram_from_elf {
	my $elf_file = shift;
	my $layout = shift;
	my $default_cpu = shift;

	my %symhash = sym_readelf($nm, $elf_file);

	my @sizes;

	for my $memory (@{$layout->{section}}) {
		my $name = $memory->{name};
		my $type = $memory->{type};
		my $start_sym = $memory->{start};
		my $end_sym = $memory->{end};
		my $cpu = $default_cpu;
		if (defined $memory->{cpu}) {
			$cpu = number $memory->{cpu};
		}

		# ignore sections of unconfigured CPUs
		if ($cpu >= $num_cpus) {
			next;
		}

		my $start_addr = sym_eval(\%symhash, $start_sym);
		my $end_addr = sym_eval(\%symhash, $end_sym);
		my $required_size = $end_addr - $start_addr;

		#print "type: $name, $type, cpu: $cpu, size: $required_size \n";

		push @sizes, [ $name, $type, $cpu, $required_size ];
	}

	return @sizes;
}

sub usage
{
	my $ret = shift;
	if (!defined $ret) {
		$ret = 1;
	}

	print "usage:\n";
	print "  ab_gen_memory_xml.pl [-h|--help] [--version]\n";
	print "                       -o <memory.xml>\n";
	print "                       <system.xml>\n";
	print "\n";
	print "options:\n";
	print "  -h|--help       print this help text and exit\n";
	print "  --version       print version information and exit\n";
	print "  -o <memory.xml> memory.xml to create\n";
	print "  -p <directory> path to application\n";
	print "  <system.xml>    system description\n";

	exit $ret;
}

my $outfile;
my $xmlfile;
my $appdir = '.';

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
		$outfile = shift;
	} elsif ($ARGV[0] eq '-s') {
		shift;
		$nm = shift;
	} elsif ($ARGV[0] eq '-p') {
		shift;
		$appdir = shift;
	} else {
		if (defined $xmlfile) {
			die "error: invalid argument '", $ARGV[0], "'\n";
		}
		$xmlfile = shift;
	}
}

if (!defined $xmlfile) {
	die "error: no xml-file specified\n";
}

if (!defined $outfile) {
	die "error: no output file specified\n";
}

my $rom_align;
my $ram_align;
my @partitions;
my @shms;

{
	my $all = XMLin($xmlfile,
					KeyAttr => { },
					ForceArray => ['layout', 'section', 'partition', 'shm', 'shm_access', 'rq'],
					) or die "opening and parsing of '$xmlfile' failed!\n";

	my $sys = $all->{system};
	my $target = $all->{target};
	$mpu_arch = $target->{mpu_arch};

	$rom_align = number $target->{rom_align};
	$ram_align = number $target->{ram_align};
	$num_cpus = number $target->{cpus};

	# analyse kernel binary to get the ROM and RAM sizes
	{
		my $partname = '__KERNEL__';
		# UUUUGLY HACK -- Take layout and file names from $BSP part of config.xml
		my $layout = $target->{kernel}->{layout}[0];
		my $elf = $target->{kernel}->{layout}[0]->{dummy_elf};
		my $cpu = 0;

		if (!defined $layout->{section}) {
			die "error: <section> not found in kernel's <layout> config.xml\n";
		}
		my @sizes = get_required_rom_and_ram_from_elf($elf, $layout, $cpu);

		my @other_rqs;
		# iterate <rq> in <kernel> partition
		for my $rq (@{$target->{kernel}->{rq}}) {
			my $name = $rq->{name};
			my $resource = $rq->{resource};
			my $r = "";
			my $w = "";
			my $x = "";
			my $c = "";

			# "resource" is mandatory, but "r", "w", and "x" are optional
			if (defined $rq->{read}) {
				$r = number $rq->{read};
			}
			if (defined $rq->{write}) {
				$w = number $rq->{write};
			}
			if (defined $rq->{exec}) {
				$x = number $rq->{exec};
			}
			if (defined $rq->{cached}) {
				$c = number $rq->{cached};
			}

			push @other_rqs, [ $name, $resource, $r, $w, $x, $c ];
		}

		push @partitions, [$partname, $cpu, \@sizes, \@other_rqs];
	}

	# iterate partitions and get ROM and RAM sizes from ELF files
	for my $part (@{$sys->{partition}}) {
		my $partname = $part->{name};
		my $layout = $part->{layout}[0];
		my $elf = $appdir . "/" . $layout->{dummy_elf};
		my $cpu = 0;
		if (defined $part->{cpu}) {
			$cpu = number $part->{cpu};
		}

		if (!defined $layout->{section}) {
			die "error: <section> not found in partition's '", $partname, "' <layout> config.xml\n";
		}
		my @sizes = get_required_rom_and_ram_from_elf($elf, $layout, $cpu);

		my @other_rqs;
		# iterate <rq> in partition
		for my $rq (@{$part->{rq}}) {
			my $name = $rq->{name};
			my $resource = $rq->{resource};
			my $r = "";
			my $w = "";
			my $x = "";
			my $c = "";

			# "resource" is mandatory, but "r", "w", and "x" are optional
			if (defined $rq->{read}) {
				$r = number $rq->{read};
			}
			if (defined $rq->{write}) {
				$w = number $rq->{write};
			}
			if (defined $rq->{exec}) {
				$x = number $rq->{exec};
			}
			if (defined $rq->{cached}) {
				$c = number $rq->{cached};
			}

			push @other_rqs, [ $name, $resource, $r, $w, $x, $c ];
		}
		# iterate shm_access in partitions
		for my $rq (@{$part->{shm_access}}) {
			my $s = $rq->{shm};
			my $r = "";
			my $w = "";
			my $x = "";
			my $c = "";

			# "shm" is mandatory, but "r", "w", and "x" are optional
			if (defined $rq->{read}) {
				$r = number $rq->{read};
			}
			if (defined $rq->{write}) {
				$w = number $rq->{write};
			}
			if (defined $rq->{exec}) {
				$x = number $rq->{exec};
			}
			if (defined $rq->{cached}) {
				$c = number $rq->{cached};
			}

			push @other_rqs, [ $s, $s, $r, $w, $x, $c ];
		}

		push @partitions, [ $partname, $cpu, \@sizes, \@other_rqs ];
	}

	# iterate SHMs
	for my $shm (@{$sys->{shm}}) {
		my $name = $shm->{name};
		my $type = $shm->{type};
		my $size = number $shm->{size};
		my $desc = $shm->{description};
		my $cpu = $shm->{cpu};
		if (!defined $cpu) {
			$cpu = 0;
		}
		my $res;
		my $align;

		if (uc $type eq "RAM") {
			$res = "__RAM_CORE" . $cpu . "__";
			$align = $ram_align;
		} elsif (uc $type eq "ROM") {
			$res = "__ROM__";
			$align = $rom_align;
		} else {
			die "shm '$name' invalid type '$type', must be 'RAM' or 'ROM'\n";
		}

		push @shms, [ $name, $res, $size, $align, $desc ];
	}
}


# Generate minimalisic memory.xml
open(my $OUTFILE, ">$outfile") or die "Couldn't open $outfile file for writing, $!\n";

print $OUTFILE "<!-- generated from $xmlfile -->\n";
print $OUTFILE "<!-- ", $VERSION, " -->\n";
print $OUTFILE "<memory_layout mpu_arch=\"", $mpu_arch, "\">\n";

foreach (@shms) {
	my ($name, $res, $size, $align, $desc) = @{$_};

	print $OUTFILE "\t<shm name=\"". $name ."\" resource=\"" . $res . "\"";
	print $OUTFILE " minsize=\"" . hexify($size). "\"";
	print $OUTFILE " align=\"" . hexify($align). "\"";
	print $OUTFILE " description=\"" . $desc . "\"";
	print $OUTFILE "/>\n";
}

foreach (@partitions) {
	my ($partname, $cpu, $sizes, $shm_accesses) = @{$_};

	print $OUTFILE "\t<part name=\"" . $partname . "\">\n";

	foreach (@{$sizes}) {
		my ($name, $type, $cpu, $required_size) = @{$_};

		if ($type eq "rom") {
			print $OUTFILE "\t\t<rq name=\"".$name."\" resource=\"__ROM__\"";
			print $OUTFILE " minsize=\"" . hexify($required_size). "\"";
			print $OUTFILE " align=\"" . hexify($rom_align). "\"";
			print $OUTFILE " read=\"1\" write=\"0\" exec=\"1\" cached=\"1\"";
			print $OUTFILE "/>\n";
		} else {
			print $OUTFILE "\t\t<rq name=\"".$name."\" resource=\"__RAM_CORE" . $cpu . "__\"";
			print $OUTFILE " minsize=\"" . hexify($required_size). "\"";
			print $OUTFILE " align=\"" . hexify($ram_align). "\"";
			print $OUTFILE " read=\"1\" write=\"1\" exec=\"0\" cached=\"1\"";
			print $OUTFILE "/>\n";
		}
	}

	foreach (@{$shm_accesses}) {
		my ($name, $res, $r, $w, $x, $c) = @{$_};

		print $OUTFILE "\t\t<rq name=\"".$name."\" resource=\"".$res."\"";
		if ($r ne "") {
			print $OUTFILE " read=\"".$r."\"";
		}
		if ($w ne "") {
			print $OUTFILE " write=\"".$w."\"";
		}
		if ($x ne "") {
			print $OUTFILE " exec=\"".$x."\"";
		}
		if ($c ne "") {
			print $OUTFILE " cached=\"".$c."\"";
		}
		print $OUTFILE "/>\n";
	}

	print $OUTFILE "\t</part>\n";
}

print $OUTFILE "</memory_layout>\n";

close($OUTFILE) or die "Couldn't close $outfile, $!\n";
