#!/usr/bin/perl -w
#
# ab_gen_final_config_xml.pl - generate final_config.xml (ELF addr info)
#
# NOTE: requires the XML::Simple CPAN module
#       on Ubuntu 12.04, try:  sudo apt-get install libxml-simple-perl
#
# Usage: ab_gen_final_config_xml.pl -o final_config.xml config.xml
#
# azuepke, 2014-08-19: refactored from genElfConfig.pl for new tooling
# azuepke, 2015-03-06: add error_state, exception_state
# azuepke, 2015-05-09: add memory ranges
# azuepke, 2016-01-13: RPC

use strict;
use warnings "all";
use XML::Simple;
use IPC::Open2;
use Data::Dumper;

# tool version ID
my $VERSION = "ab_gen_final_config_xml.pl 2016-01-13";

my $nm = 'nm';
my $appdir = '.';

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

# Handle an <invoke> block
# Usage: gen_invoke($file, $reloc, $invoke_xml, $elf_file)
sub gen_invoke
{
	my $CFGFILE = shift;
	my $reloc = shift;
	my $invoke = shift;
	my $elf_file = shift;

	my $task_entry_sym = "NULL";
	my $task_stack_sym = "NULL";
	my $task_arg0_sym = "NULL";

	my $task_entry = 0;
	my $task_stack = 0;
	my $task_arg0 = 0;

	if ($reloc && defined $invoke) {
		my %symhash_part = sym_readelf($nm, $elf_file);

		if (defined $invoke->{entry} && length($invoke->{entry}) > 0) {
			$task_entry_sym = $invoke->{entry};
			$task_entry = sym_eval(\%symhash_part, $invoke->{entry});
		}
		if (defined $invoke->{stack} && length($invoke->{stack}) > 0) {
			$task_stack_sym = $invoke->{stack};
			$task_stack = sym_eval(\%symhash_part, $invoke->{stack});
		}
		if (defined $invoke->{arg} && length($invoke->{arg}) > 0) {
			$task_arg0_sym = $invoke->{arg};
			$task_arg0 = sym_eval(\%symhash_part, $invoke->{arg});
		}
	}

	print $CFGFILE "\t\t<entry symbol=\"", $task_entry_sym,"\">", hexify($task_entry), "</entry>\n";
	print $CFGFILE "\t\t<stack symbol=\"", $task_stack_sym,"\">", hexify($task_stack), "</stack>\n";
	print $CFGFILE "\t\t<arg0 symbol=\"", $task_arg0_sym,"\">", hexify($task_arg0),  "</arg0>\n";
}

sub ab_gen_final_config_xml
{
	my $xmlfile = shift;
	my $reloc = shift;
	my $cfgfile = shift;
	my $sys = XMLin($xmlfile,
					KeyAttr => { },
					ForceArray => ['partition', 'task', 'hook', 'invokable', 'layout', 'invoke', 'kldd', 'isr', 'ipev', 'counter', 'range'],
					) or die "opening and parsing failed!\n";

	my $num_isrs = number $sys->{target}->{isrs};
	my $orig_rom_base = number $sys->{target}->{rom};
	my $orig_ram_base = number $sys->{target}->{ram};
	my $rom_align = number $sys->{target}->{rom_align};
	my $ram_align = number $sys->{target}->{ram_align};
	my %symhash_kern;
	my %symhash_part;
	my $layout;
	my $CFGFILE;
	my @known_isrs_user;
	my @known_isrs_kern;
	my $dummy_elf;
	my $final_elf;

	if ($cfgfile) {
		open($CFGFILE, ">$cfgfile") or die "Couldn't open $cfgfile file for writing, $!\n";
	} else {
		open($CFGFILE, ">/dev/null") or die "Couldn't open $cfgfile file for writing, $!\n";
	}
	print $CFGFILE "<!-- generated from $xmlfile -->\n";
	print $CFGFILE "<!-- ", $VERSION, " -->\n";
	print $CFGFILE "<reloc>\n";

	# analyse kernel binary to get the final ROM addresses

	$layout = $sys->{target}->{kernel}->{layout}[0];
	# NOTE: we do the analysis of the kernel based on the dummy ELF file
	$dummy_elf = $layout->{dummy_elf};
	%symhash_kern = sym_readelf($nm, $dummy_elf);


	# iterate partitions
	for my $part (@{$sys->{system}->{partition}}) {
		my $user_sched_state = 0;
		my $user_error_state = 0;
		my $user_exception_state = 0;
		my $sda1_base = 0;
		my $sda2_base = 0;

		my $layout = $part->{layout}[0];

		$final_elf = $appdir . "/" . $layout->{final_elf};
		%symhash_part = sym_readelf($nm, $final_elf);

		print $CFGFILE "\t<partition name=\"", $part->{name}, "\">\n";

		# Magic symbols
		$user_sched_state = sym_eval(\%symhash_part, $part->{sched_state});
		if (defined $part->{error_state}) {
			$user_error_state = sym_eval(\%symhash_part, $part->{error_state});
		}
		if (defined $part->{exception_state}) {
			$user_exception_state = sym_eval(\%symhash_part, $part->{exception_state});
		}
		if (defined $part->{sda1_base}) {
			$sda1_base = sym_eval(\%symhash_part, $part->{sda1_base});
		}
		if (defined $part->{sda2_base}) {
			$sda2_base = sym_eval(\%symhash_part, $part->{sda2_base});
		}

		# partition layout
		print $CFGFILE "\t\t<user_sched_state>", hexify($user_sched_state),"</user_sched_state>\n";
		print $CFGFILE "\t\t<user_error_state>", hexify($user_error_state),"</user_error_state>\n";
		print $CFGFILE "\t\t<user_exception_state>", hexify($user_exception_state),"</user_exception_state>\n";
		print $CFGFILE "\t\t<sda1_base>", hexify($sda1_base),"</sda1_base>\n";
		print $CFGFILE "\t\t<sda2_base>", hexify($sda2_base),"</sda2_base>\n";

		# Memory ranges (for of them)
		my $r = 0;
		for my $range (@{$layout->{range}}) {
			my $start = sym_eval(\%symhash_part, $range->{start});
			my $end   = sym_eval(\%symhash_part, $range->{end});

			print $CFGFILE "\t\t<range_", $r, "_start>", hexify($start), "</range_", $r, "_start>\n";
			print $CFGFILE "\t\t<range_", $r, "_end>",   hexify($end),   "</range_", $r, "_end>\n";
			$r++;
		}
		while ($r < 4) {
			my $start = 0;
			my $end = 0;

			print $CFGFILE "\t\t<range_", $r, "_start>", hexify($start), "</range_", $r, "_start>\n";
			print $CFGFILE "\t\t<range_", $r, "_end>",   hexify($end),   "</range_", $r, "_end>\n";
			$r++;
		}

		print $CFGFILE "\t</partition>\n";
	}

	my $task_array_index = 0;
	for my $part (@{$sys->{system}->{partition}}) {
		my $layout = $part->{layout}[0];
		my $elf_file = $appdir . "/" . $layout->{final_elf};
		my $task_cnt = 0;
		my $hook_cnt = 0;
		my $isr_cnt = 0;
		my $invokable_cnt = 0;

		# Tasks
		for my $task (@{$part->{task}}) {
			my $fpu_mode = "0";
			my $elev_prio;
			my $task_type = "0";
			my $max_activations = 1;


			print $CFGFILE "\t<task name=\"", $task->{name}, "\" part=\"", $part->{name}, "\" id=\"",$task_cnt,"\">\n";

			# invoke block
			gen_invoke($CFGFILE, $reloc, $task->{invoke}[0], $elf_file);

			print $CFGFILE "\t</task>\n";
			$task_cnt++;
			$task_array_index++;
		}

		# Hooks
		for my $task (@{$part->{hook}}) {
			my $fpu_mode = "0";
			my $elev_prio;
			my $task_type = "0";
			my $max_activations = 1;


			print $CFGFILE "\t<task name=\"", $task->{name}, "\" part=\"", $part->{name}, "\" id=\"",$hook_cnt,"\">\n";

			# invoke block
			gen_invoke($CFGFILE, $reloc, $task->{invoke}[0], $elf_file);

			print $CFGFILE "\t</task>\n";
			$hook_cnt++;
			$task_array_index++;
		}

		# ISRs
		for my $isr (@{$part->{isr}}) {
			my $fpu_mode = 0;
			my $vector;
			print $CFGFILE "\t<task name=\"", $isr->{name}, "\" part=\"", $part->{name}, "\" id=\"",$isr_cnt,"\">\n";


			# invoke block
			gen_invoke($CFGFILE, $reloc, $isr->{invoke}[0], $elf_file);

			print $CFGFILE "\t</task>\n";
			$vector = $isr->{vector};
			if ($vector >= $num_isrs) {
				die "error: ISR '", $isr->{name}, "' in partition '", $part->{name},
				    "' vector ", $vector, " out of bounds (0..", $num_isrs-1, ")\n";
			}
			if (defined $known_isrs_user[$vector]) {
				die "error: ISR '", $isr->{name}, "' in partition '", $part->{name},
				    "' vector ", $vector, " already allocated\n";
			}
			$known_isrs_user[$vector] = $task_array_index;
			$isr_cnt++;
			$task_array_index++;
		}

		# Invokables
		for my $task (@{$part->{invokable}}) {
			my $fpu_mode = "0";
			my $elev_prio;
			my $task_type = "0";
			my $max_activations = 1;


			print $CFGFILE "\t<task name=\"", $task->{name}, "\" part=\"", $part->{name}, "\" id=\"",$invokable_cnt,"\">\n";

			# invoke block
			gen_invoke($CFGFILE, $reloc, $task->{invoke}[0], $elf_file);

			print $CFGFILE "\t</task>\n";
			$invokable_cnt++;
			$task_array_index++;
		}


		#$part_cnt++;
	}
	for my $part (@{$sys->{system}->{partition}}) {
		my $kldd_id = 0;
		for my $kldd (@{$part->{kldd}}) {
			my $kldd_func = 0;
			my $kldd_arg0 = 0;

			if ($reloc) {
				$kldd_func = sym_eval(\%symhash_kern, $kldd->{entry});
				$kldd_arg0 = sym_eval(\%symhash_kern, $kldd->{arg});
			}

			print $CFGFILE "\t<kldd id=\"", $kldd_id, "\" part=\"", $part->{name}, "\" entry=\"", $kldd->{entry},"\" arg=\"", $kldd->{arg},"\">\n";
			print $CFGFILE "\t\t<func>", hexify($kldd_func), "</func>\n";
			print $CFGFILE "\t\t<arg0>", hexify($kldd_arg0), "</arg0>\n";
			print $CFGFILE "\t</kldd>\n";
		}
		$kldd_id++;
	}
	# Iterate all cat 1 ISRs and fill in the interrupt table
	{
		my $kernel = $sys->{system}->{kernel};
		for my $isr (@{$kernel->{isr}}) {
			my $vector = $isr->{vector};
			if ($vector >= $num_isrs) {
				die "error: ISR '", $isr->{name}, "' in supervisor mode",
				    " vector ", $vector, " out of bounds (0..", $num_isrs-1, ")\n";
			}
			if (defined $known_isrs_user[$vector] || defined $known_isrs_kern[$vector]) {
				die "error: ISR '", $isr->{name}, "' in supervisor mode",
				    " vector ", $vector, " already allocated\n";
			}
			# Pass XML subtree
			$known_isrs_kern[$vector] = $isr->{invoke}[0];
		}
	}
	for (my $vector = 0; $ vector < $num_isrs; $vector++){
		if ($reloc && defined $known_isrs_kern[$vector]) {
			print $CFGFILE "\t<isr_vector id=\"",$vector,"\">\n";
			my $isr_func = 0;
			my $isr_arg0 = 0;
			my $isr_func_name = "NULL";
			my $isr_arg0_name = "NULL";

			my $invoke = $known_isrs_kern[$vector];

			$isr_func_name = $invoke->{entry};
			$isr_arg0_name = $invoke->{arg};

			$isr_func = sym_eval(\%symhash_kern, $invoke->{entry});
			$isr_arg0 = sym_eval(\%symhash_kern, $invoke->{arg});

			print $CFGFILE "\t\t<func symbol=\"",$isr_func_name,"\">", hexify($isr_func),"</func>\n";
			print $CFGFILE "\t\t<arg0 symbol=\"",$isr_arg0_name,"\">", hexify($isr_arg0),"</arg0>\n";

			print $CFGFILE "\t</isr_vector>\n";
		}
	}
	for my $counter (@{$sys->{system}->{counter}}) {
		if ($reloc) {
			my $r = 0;
			my $q = 0;
			my $c = 0;
			print $CFGFILE "\t<counter name=\"",$counter->{name},"\">\n";
			if ($counter->{type} eq 'hw') {
				$r = sym_eval(\%symhash_kern, $counter->{register});
				$q = sym_eval(\%symhash_kern, $counter->{query});
				$c = sym_eval(\%symhash_kern, $counter->{change});
				print $CFGFILE "\t\t<register>", hexify($r),"</register>\n";
				print $CFGFILE "\t\t<query>", hexify($q),"</query>\n";
				print $CFGFILE "\t\t<change>", hexify($c),"</change>\n";
			}
			else
			{
				print $CFGFILE "\t\t<register>NULL</register>\n";
				print $CFGFILE "\t\t<query>NULL</query>\n";
				print $CFGFILE "\t\t<change>NULL</change>\n";
			}
			print $CFGFILE "\t</counter>\n";
		}
	}
	print $CFGFILE "</reloc>\n";
	# config file generation complete
	close($CFGFILE) or die "Couldn't close $cfgfile, $!\n";
}


sub usage
{
	my $ret = shift;
	if (!defined $ret) {
		$ret = 1;
	}

	print "usage:\n";
	print "  ab_gen_final_config_xml.pl [-h|--help] [--version]\n";
	print "                             -o <final.xml>\n";
	print "                             <system.xml>\n";
	print "\n";
	print "options:\n";
	print "  -h|--help      print this help text and exit\n";
	print "  --version      print version information and exit\n";
	print "  -o <final.xml> final system description to create\n";
	print "  -p <directory> path to application\n";
	print "  <system.xml>   system description\n";

	exit $ret;
}

my $cfgfile;
my $reloc = 1;
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
		$cfgfile = shift;
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
	die "error: no input xml-file specified\n";
}

if (!defined $cfgfile) {
	die "error: no output xml-file specified\n";
}

ab_gen_final_config_xml($xmlfile, $reloc, $cfgfile);
exit 0;
