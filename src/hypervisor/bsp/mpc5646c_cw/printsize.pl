#!/usr/bin/perl -w

use strict;

# objdump -h output:
# Idx Name          Size      VMA       LMA       File off  Algn
#   0 .text         000010fc  00000000  00000000  00000034  2**2

# we're interested in .text*, .data*, .rodata* and .bss*
my (%sizes, %seen);

while (<>) {
	next unless /^\s*\d+\s+\.(text|data|rodata|sdata(2)?|(s)?bss(2)?)\S*\s+([0-9a-f]{8})\s+/;
	my ($type, $size) = ($1, $5);
	$type = 'rodata' if $type eq 'sdata2';
	$type = 'rodata' if $type eq 'sbss2';
	$type = 'data' if $type eq 'sdata';
	$type = 'bss' if $type eq 'sbss';
	$sizes{$type} += hex($size);
	$seen{$type}++;
}

exit 1 unless (scalar keys %sizes > 0);

print "type\tsize\tseen\n";
foreach my $k (sort keys %sizes) {
	print "$k\t$sizes{$k}\t$seen{$k}\n";
}
