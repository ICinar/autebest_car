#!/usr/bin/perl -w
#
# ab_gen_romimage.pl - generate binary ROM image from config files
#
# NOTE: requires the XML::Simple CPAN module
#       on Ubuntu 12.04, try:  sudo apt-get install libxml-simple-perl
#
# Usage: ab_gen_romimage.pl -m memory.xml system.xml -o binary.bin
# Without the memory map, this script assumes dummy addresses for RAM and ROM.
#
# azuepke, 2014-08-19: initial


use strict;
use warnings "all";
use XML::Simple;
use Data::Dumper;

# tool version ID
my $VERSION = "ab_gen_romimage.pl 2015-06-26";

# global variables
my $verbose = 0;
my %addrhash;
my $orig_rom_base;
my @chunks;
my $chunk;


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

################################################################################

# Read a file and create ROM tuple (pos, filesize, buffer)
# Usage: my $tuple = readbin("filename", $offset_in_file);
sub readbin
{
	my $filename = shift;
	my $rom_offset = shift;
	my $buffer;
	my $FILE;

	open($FILE, "<$filename") or die "Couldn't open $filename file for reading, $!\n";
	binmode($FILE);
	my $filesize = -s $filename;
	my $n = sysread($FILE, $buffer, $filesize);
	if ($n != $filesize) {
		die "Couldn't read $filename properly, $!\n";
	}
	close($FILE) or die "Couldn't close $filename, $!\n";

	my @tuple = ($rom_offset, $filesize, $buffer);
	return \@tuple;
}

################################################################################

sub usage
{
	my $ret = shift;
	if (!defined $ret) {
		$ret = 1;
	}

	print "usage:\n";
	print "  ab_gen_romimage.pl [-h|--help] [--version]\n";
	print "                     [-v]\n";
	print "                     -m <memory.xml>\n";
	print "                     <system.xml>\n";
	print "                     -o <image.bin>\n";
	print "\n";
	print "options:\n";
	print "  -h|--help       print this help text and exit\n";
	print "  --version       print version information and exit\n";
	print "  -v              verbosity level, increases for each -v\n";
	print "  -m <memory.xml> memory map description (for memory layout)\n";
	print "  <system.xml>    system description (for partition layout)\n";
	print "  -o <image.bin>  binary ROM image to create\n";

	exit $ret;
}

################################################################################

my $sysxmlfile;
my $memxmlfile;
my $binfile;
my $appdir = '.';

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
	} elsif ($ARGV[0] eq '-m') {
		shift;
		$memxmlfile = shift;
	} elsif ($ARGV[0] eq '-o') {
		shift;
		$binfile = shift;
	} elsif ($ARGV[0] eq '-p') {
		shift;
		$appdir = shift;
	} else {
		if (defined $sysxmlfile) {
			die "error: invalid argument '", $ARGV[0], "'\n";
		}
		$sysxmlfile = shift;
	}
}

if (!defined $sysxmlfile) {
	die "error: no system.xml-file specified\n";
}

if (!defined $memxmlfile) {
	die "error: no memory.xml-file specified\n";
}

if (!defined $binfile) {
	die "error: no output file specified\n";
}

# Read memory.xml to get partition start addresses in ROM
{
	my $mem = XMLin($memxmlfile,
					KeyAttr => { },
					ForceArray => ['shm', 'part', 'task', 'rq'],
					) or die "opening and parsing of '$memxmlfile' failed!\n";

	# iterate partitions, search for rqs with resource "__ROM__"
	for my $part (@{$mem->{part}}) {
		my $partname = $part->{name};
		my $rom_base = -1;

		for my $rq (@{$part->{rq}}) {
			if (!defined $rq->{start} || !defined $rq->{size}) {
				die "error: memory.xml does not contain final addresses\n";
			}

			if ($rq->{resource} eq "__ROM__") {
				$rom_base = number $rq->{start};
			}
		}
		if ($rom_base == -1) {
			die "error: memory.xml lacks <rq> of resource '__ROM__' for partition $partname\n";
		}

		if ($verbose) {
			print "partition: $partname -> ROM: ", hexify($rom_base), "\n";
		}
		$addrhash{$partname} = $rom_base;
	}
}

# Read system.xml to get application binary names
{
	# Parse system description XML
	my $all = XMLin($sysxmlfile,
					KeyAttr => { },
					ForceArray => ['layout', 'partition'],
					) or die "opening and parsing of '$sysxmlfile' failed!\n";

	my $sys = $all->{system};
	my $target = $all->{target};

	# Get start address in ROM to adjust binary images later
	$orig_rom_base = number $target->{rom};
	if ($verbose) {
		print "ROM start address: ", hexify($orig_rom_base), "\n";
	}

	# read kernel binary
	{
		# UUUUGLY HACK -- Override for file names
		my $bin = $target->{kernel}->{layout}[0]->{bin};
		my $partname = "__KERNEL__";

		if (!defined $addrhash{$partname}) {
			die "error: could not find partition '$partname' in $memxmlfile\n";
		}
		my $rom_base = $addrhash{$partname};

		if ($verbose) {
			print "partition: $partname: reading $bin\n";
		}
		my $chunk = readbin($bin, $rom_base - $orig_rom_base);
		push (@chunks, $chunk);
	}

	# iterate partitions and read partition binaries
	for my $part (@{$sys->{partition}}) {
		my $bin = $appdir . "/" . $part->{layout}[0]->{bin};
		my $partname = $part->{name};

		if (!defined $addrhash{$partname}) {
			die "error: could not find partition '$partname' in $memxmlfile\n";
		}
		my $rom_base = $addrhash{$partname};

		if ($verbose) {
			print "partition: $partname: reading $bin\n";
		}
		my $chunk = readbin($bin, $rom_base - $orig_rom_base);
		push (@chunks, $chunk);
	}
}

# generate a single binary image
if ($binfile) {
	my $FILE;
	open $FILE, ">$binfile" or die "Couldn't open $binfile file for writing, $!\n";
	binmode($FILE);

	foreach (@chunks) {
		if ($verbose) {
			print "write: ", hexify($_->[0]), " -- ", hexify($_->[1]), "\n";
		}
		seek($FILE, $_->[0], 0);
		my $n = syswrite($FILE, $_->[2], $_->[1]);
		if ($n != $_->[1]) {
			die "Couldn't write $binfile properly, $!\n";
		}
	}

	close($FILE) or die "Couldn't close $binfile, $!\n";
}
