#!/usr/bin/perl -w
#
# ab_gen_hardware_xml.pl - generate minimal hardware description from system.xml
#
# NOTE: requires the XML::Simple CPAN module
#       on Ubuntu 12.04, try:  sudo apt-get install libxml-simple-perl
#
# Usage: ab_gen_hardware_xml.pl -o hardware.xml system.xml
#
# azuepke, 2014-08-18: initial (cloned from genconfig.pl)
# azuepke, 2014-08-19: read ROM/RAM sizes and mpu_arch from system.xml
# azuepke, 2015-02-23: fixed I/O mappings in system.xml
# azuepke, 2015-03-24: per-CPU RAM blocks
# azuepke, 2015-08-05: handle cached attribute


use strict;
use warnings "all";
use XML::Simple;
use Data::Dumper;

# tool version ID
my $VERSION = "ab_gen_hardware_xml.pl 2015-08-05";

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

# Generate a minimal hardware.xml (to be saved in $outfile)
# Usage: gen_hardware_xml($xmlfile, $outfile)
sub gen_hardware_xml
{
	my $xmlfile = shift;
	my $outfile = shift;

	my $all = XMLin($xmlfile,
					KeyAttr => { },
					ForceArray => ['fix'],
					) or die "opening and parsing of '$xmlfile' failed!\n";

	my $target = $all->{target};

	my $rom_base = number $target->{rom};
	my @ram_bases = map { number $_ } split(',', $target->{ram});
	my $rom_size = number $target->{rom_size};
	my @ram_sizes = map { number $_ } split(',', $target->{ram_size});
	my $mpu_arch = $target->{mpu_arch};

	open(my $OUTFILE, ">$outfile") or die "Couldn't open $outfile file for writing, $!\n";

	print $OUTFILE "<!-- generated from $xmlfile -->\n";
	print $OUTFILE "<!-- ", $VERSION, " -->\n";
	print $OUTFILE "<hardware mpu_arch=\"" . $mpu_arch . "\">\n";

	print $OUTFILE "\t<!-- default pools from target attributes -->\n";
	print $OUTFILE "\t<pool name=\"__ROM__\"";
	print $OUTFILE " start=\"" . hexify($rom_base) . "\"";
	print $OUTFILE " size=\"" . hexify($rom_size) . "\"";
	print $OUTFILE " read=\"1\" write=\"0\" exec=\"1\" cached=\"1\"";
	print $OUTFILE " desc=\"System Flash\"/>\n";

	my $cpu = 0;
	foreach (@ram_bases) {
		print $OUTFILE "\t<pool name=\"__RAM_CORE", $cpu, "__\"";
		print $OUTFILE " start=\"" . hexify($_) . "\"";
		print $OUTFILE " size=\"" . hexify($ram_sizes[$cpu]) . "\"";
		print $OUTFILE " read=\"1\" write=\"1\" exec=\"0\" cached=\"1\"";
		print $OUTFILE " desc=\"System RAM\"/>\n";
		$cpu++;
	}

	print $OUTFILE "\n\t<!-- additional \"pool\" entries from system.xml -->\n";
	for my $window (@{$target->{pool}}) {
		my $name = $window->{name};
		my $desc = $window->{desc};
		my $start = number $window->{start};
		my $size = number $window->{size};
		my $r = !!$window->{read}+0;
		my $w = !!$window->{write}+0;
		my $x = !!$window->{exec}+0;
		my $c = !!$window->{cached}+0;

		print $OUTFILE "\t<pool name=\"".$name."\"";
		print $OUTFILE " start=\"" . hexify($start) . "\"";
		print $OUTFILE " size=\"" . hexify($size) . "\"";
		print $OUTFILE " read=\"".$r."\" write=\"".$w."\" exec=\"".$x."\" cached=\"".$c."\"";
		print $OUTFILE " desc=\"".$desc."\"/>\n";
	}

	print $OUTFILE "\n\t<!-- additional \"fix\" entries from system.xml -->\n";
	for my $window (@{$target->{fix}}) {
		my $name = $window->{name};
		my $desc = $window->{desc};
		my $start = number $window->{start};
		my $size = number $window->{size};
		my $r = !!$window->{read}+0;
		my $w = !!$window->{write}+0;
		my $x = !!$window->{exec}+0;
		my $c = !!$window->{cached}+0;

		print $OUTFILE "\t<fix name=\"".$name."\"";
		print $OUTFILE " start=\"" . hexify($start) . "\"";
		print $OUTFILE " size=\"" . hexify($size) . "\"";
		print $OUTFILE " read=\"".$r."\" write=\"".$w."\" exec=\"".$x."\" cached=\"".$c."\"";
		print $OUTFILE " desc=\"".$desc."\"/>\n";
	}

	print $OUTFILE "</hardware>\n";

	close($OUTFILE) or die "Couldn't close $outfile, $!\n";
}

sub usage
{
	my $ret = shift;
	if (!defined $ret) {
		$ret = 1;
	}

	print "usage:\n";
	print "  ab_gen_hardware_xml.pl [-h|--help] [--version]\n";
	print "                         -o <hardware.xml>\n";
	print "                         <system.xml>\n";
	print "\n";
	print "options:\n";
	print "  -h|--help         print this help text and exit\n";
	print "  --version         print version information and exit\n";
	print "  -o <hardware.xml> hardware.xml to create\n";
	print "  <system.xml>      system description\n";

	exit $ret;
}

my $outfile;
my $xmlfile;

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

gen_hardware_xml($xmlfile, $outfile);
exit 0;
