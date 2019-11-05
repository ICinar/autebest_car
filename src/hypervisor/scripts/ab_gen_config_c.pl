#!/usr/bin/perl -w
#
# ab_gen_config_c.pl - config.c generator
#
# NOTE: requires the XML::Simple CPAN module
#       on Ubuntu 12.04, try:  sudo apt-get install libxml-simple-perl
#
# Usage: ab_gen_config_c.pl system.xml -r -o config.c
#
# azuepke, 2013-12-06: initial
# azuepke, 2013-12-08: relocations working
# azuepke, 2013-12-09: fixed parameters, errors exit with die()
# azuepke, 2014-03-09: added KLDDs and ISRs
# azuepke, 2014-04-25: pass $ARCH_$SUBARCH to relocate.sh; relocate kernel
# azuepke, 2014-04-29: relocation creates .map-file
# azuepke, 2014-05-01: add user scheduling area
# azuepke, 2014-06-04: use <all> namespace
# azuepke, 2014-06-05: alarms and counters
# azuepke, 2014-06-07: defined <hook> class of tasks
# azuepke, 2014-06-10: schedule tables
# azuepke, 2014-08-19: refactored from configgen.pl to use memory.xml
# azuepke, 2014-09-09: partitions have now an explicit init_hook
# azuepke, 2014-09-29: SHM support
# azuepke, 2014-12-15: per task/partition Register Contexts for TriCore
# azuepke, 2015-03-06: per partition error and exception hooks
# azuepke, 2015-03-22: per core data structures
# azuepke, 2015-04-02: IPI configuration
# azuepke, 2015-04-24: time partition configuration
# azuepke, 2015-05-04: more time partitioning
# azuepke, 2015-05-08: memory ranges
# azuepke, 2015-05-12: back to single kernel stack model
# azuepke, 2015-05-18: generate HM tables
# azuepke, 2015-06-16: generate default time partition schedule and HM table
# azuepke, 2015-05-18: new stack handling
# azuepke, 2016-01-06: task state and flags reorga
# azuepke, 2016-01-13: RPC


use strict;
use warnings "all";
use XML::Simple;
use IPC::Open2;
use Data::Dumper;

# tool version ID
my $VERSION = "ab_gen_config_c.pl 2016-01-29";

my $nm = 'nm';
my $appdir = '.';

# match with limits in hm_state.h
my $NUM_HM_ERROR_IDS = 36;

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
	my $task_stack_size = 0;
	my $task_arg0_sym = "NULL";

	my $task_entry = 0;
	my $task_stack = 0;
	my $task_arg0 = 0;

	if ($reloc && defined $invoke) {
		my %symhash_part = sym_readelf($nm, $elf_file);

		# the entry point is a mandatory attribute
		$task_entry_sym = $invoke->{entry};
		$task_entry = sym_eval(\%symhash_part, $invoke->{entry});

		if (defined $invoke->{stack} && length($invoke->{stack}) > 0) {
			$task_stack_sym = $invoke->{stack};
			$task_stack = sym_eval(\%symhash_part, $invoke->{stack});
		}
		if (defined $invoke->{stack_size}) {
			$task_stack_size = $invoke->{stack_size};
		}
		if (defined $invoke->{arg} && length($invoke->{arg}) > 0) {
			$task_arg0_sym = $invoke->{arg};
			$task_arg0 = sym_eval(\%symhash_part, $invoke->{arg});
		}
	}

	print $CFGFILE "\t\t.entry = ", hexify($task_entry), ", /* ", $task_entry_sym, " */\n";
	print $CFGFILE "\t\t.stack = ", hexify($task_stack), ", /* ", $task_stack_sym, " */\n";
	print $CFGFILE "\t\t.stack_size = ", hexify($task_stack_size), ",\n";
	print $CFGFILE "\t\t.arg0  = ", hexify($task_arg0),  ", /* ", $task_arg0_sym,  " */\n";

	# returns 1 if valid start values were supplied
	return defined $invoke;
}

# Generate a system config
# Usage: gen_config($xmlfile, $reloc, $cfgfile)
# - settings $reloc to 1 enables relocation
# - setting $cfgfile to 0 disables the generation of config.c
sub gen_config
{
	my $xmlfile = shift;
	my $reloc = shift;
	my $cfgfile = shift;

	my $all = XMLin($xmlfile,
					KeyAttr => { },
					ForceArray => ['partition', 'task', 'isr', 'hook', 'layout', 'invoke',
					               'sched_table', 'expiry', 'action_task', 'action_hook',
					               'action_event', 'action_counter', 'action_invoke',
					               'defaultisr', 'wait_queue', 'shm', 'shm_access',
					               'schedule', 'window', 'range',
					               'hm_table', 'error',
					               'rpc', 'invokable',
					               'kldd', 'ipev', 'alarm', 'counter', 'counter_access'],
					) or die "opening and parsing failed!\n";

	my $sys = $all->{system};
	my $target = $all->{target};

	if (!defined $target->{cpus}) {
		die "target has undefined number of CPUs\n";
	}
	my $num_cpus = number $target->{cpus};

	if (!defined $sys->{period}) {
		die "system has undefined period\n";
	}
	my $system_period = number $sys->{period};

	if (!defined $sys->{timeparts}) {
		die "system has undefined number of timeparts\n";
	}
	my $num_timeparts = number $sys->{timeparts};

 	my $num_isrs = number $target->{isrs};

	my $kern_contexts = 0;	# Register contexts for the kernel
	if (defined $sys->{kern_contexts}) {
		$kern_contexts = number $sys->{kern_contexts};
	}

	my $idle_contexts = 0;	# Register contexts for the idle partition
	if (defined $sys->{idle_contexts}) {
		$idle_contexts = number $sys->{idle_contexts};
 	}

	my $nmi_contexts = 0;	# Register contexts for NMI handling
	if (defined $sys->{nmi_contexts}) {
		$nmi_contexts = number $sys->{nmi_contexts};
 	}

	if (!defined $sys->{kern_stack_size}) {
		die "system has undefined size of kern_stack_size\n";
	}
	my $kern_stack_size = number $sys->{kern_stack_size};

	if (!defined $sys->{idle_stack_size}) {
		die "system has undefined size of idle_stack_size\n";
	}
	my $idle_stack_size = number $sys->{idle_stack_size};

	if (!defined $sys->{nmi_stack_size}) {
		die "system has undefined size of nmi_stack_size\n";
	}
	my $nmi_stack_size = number $sys->{nmi_stack_size};

	my $orig_rom_base = number $target->{rom};
	my @ram_bases = split(',', $target->{ram});
	my $orig_ram_base = number (shift @ram_bases);
	my $num_parts = 0 + $num_cpus;	# plus idle partitions
	if ($sys->{partition}) {
		$num_parts += @{$sys->{partition}};
	}
	my %symhash_kern;
	my %symhash_part;
	my $CFGFILE;
	my @known_isrs_user;
	my @known_isrs_kern;
	my %known_partitions;
	my %known_partition_cpus;
	my %known_partition_tasks;
	my %known_partition_alarms;
	my %known_partition_schedtabs;
	my %known_partition_wqs;
	my %known_partition_invokables;
	my %known_schedtab_cpus;
	my %known_entities;
	my %known_tasks;
	my %known_isrs;
	my %known_hooks;
	my %known_invokables;
	my %known_local_tasks;
	my %known_local_isrs;
	my %known_local_hooks;
	my %known_local_invokables;
	my %known_counters;
	my %known_schedules;
	my %sta_comment;
	my %sta_actions;
	my %sta_arg1s;
	my %sta_arg2s;
	my %sta_arg3s;
	my %sta_arg4s;
	my %core_reg_frames;
	my %core_fpu_frames;
	my %core_ctxt_frames;

	if ($cfgfile) {
		open($CFGFILE, ">$cfgfile") or die "Couldn't open $cfgfile file for writing, $!\n";
	} else {
		open($CFGFILE, ">/dev/null") or die "Couldn't open $cfgfile file for writing, $!\n";
	}

	print $CFGFILE "/* config.c -- AUTOGENERATED -- DO NOT EDIT -- */\n";
	print $CFGFILE "/* ", $VERSION, " */\n";
	print $CFGFILE "\n";
	if ($reloc) {
		print $CFGFILE "/* NOTE: this file contains the final relocation information */\n";
	} else {
		print $CFGFILE "/* WARNING: THIS FILE LACKS THE FINAL RELOCATION INFORMATION */\n";
		print $CFGFILE "/* WARNING: rerun ab_gen_config_c.pl with -r for the final relocation pass */\n";
	}
	print $CFGFILE "\n";

	print $CFGFILE "#include <kernel.h>\n";
	print $CFGFILE "#include <assert.h>\n";
	print $CFGFILE "#include <sched_state.h>\n";
	print $CFGFILE "#include <core_state.h>\n";
	print $CFGFILE "#include <part_state.h>\n";
	print $CFGFILE "#include <task_state.h>\n";
	print $CFGFILE "#include <kldd_state.h>\n";
	print $CFGFILE "#include <ipev_state.h>\n";
	print $CFGFILE "#include <counter_state.h>\n";
	print $CFGFILE "#include <alarm_state.h>\n";
	print $CFGFILE "#include <schedtab_state.h>\n";
	print $CFGFILE "#include <isr_state.h>\n";
	print $CFGFILE "#include <wq_state.h>\n";
	print $CFGFILE "#include <shm_state.h>\n";
	print $CFGFILE "#include <arch_mpu_state.h>\n";
	print $CFGFILE "#include <ipi_state.h>\n";
	print $CFGFILE "#include <tp_state.h>\n";
	print $CFGFILE "#include <hm_state.h>\n";
	print $CFGFILE "#include <hv_error.h>\n";
	print $CFGFILE "#include <rpc_state.h>\n";
	print $CFGFILE "\n";

	print $CFGFILE "/* forward declaration */\n";
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "extern struct task task_dyn_idle_", $cpu, "[];\n";
	}
	for (my $p = $num_cpus; $p < $num_parts; $p++) {
		print $CFGFILE "extern struct task task_dyn_part_", $p, "[];\n";
		print $CFGFILE "extern struct alarm alarm_dyn_part_", $p, "[];\n";
		print $CFGFILE "extern struct schedtab schedtab_dyn_part_", $p, "[];\n";
		print $CFGFILE "extern struct wq wq_dyn_part_", $p, "[];\n";
		print $CFGFILE "extern struct rpc rpc_dyn_part_", $p, "[];\n";
	}
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "extern struct arch_reg_frame kernel_reg_frames_core_", $cpu, "[];\n";
		$core_reg_frames{$cpu} = 0;
		print $CFGFILE "extern struct arch_fpu_frame kernel_fpu_frames_core_", $cpu, "[];\n";
		$core_fpu_frames{$cpu} = 0;
		print $CFGFILE "extern struct arch_ctxt_frame kernel_ctxt_frames_core_", $cpu, "[];\n";
		$core_ctxt_frames{$cpu} = 0;
	}
	print $CFGFILE "extern const struct wq_cfg wq_cfg[];\n";
	print $CFGFILE "extern const struct counter_access counter_access[];\n";
	print $CFGFILE "extern const struct shm_access shm_access[];\n";
	print $CFGFILE "extern const struct shm_cfg shm_cfg[];\n";
	print $CFGFILE "extern const struct kldd_cfg kldd_cfg[];\n";
	print $CFGFILE "extern const struct ipev_cfg ipev_cfg[];\n";
	print $CFGFILE "extern const struct arch_mpu_task_cfg mpu_task_cfg[];\n";
	print $CFGFILE "extern const struct arch_mpu_part_cfg mpu_part_cfg[];\n";
	print $CFGFILE "extern const struct tpwindow_cfg tpwindow_cfg[];\n";
	print $CFGFILE "extern const struct rpc_cfg rpc_cfg[];\n";
	print $CFGFILE "\n";

	print $CFGFILE "/* scheduler configuration */\n";
	print $CFGFILE "const uint8_t num_cpus __section_cfg = ", $num_cpus, ";\n";
	print $CFGFILE "const uint8_t num_timeparts __section_cfg = ", $num_timeparts, ";\n";
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "struct sched_state sched_state_core_", $cpu, " __section_sched_state_core(", $cpu, ");\n";
		print $CFGFILE "struct timepart_state timepart_states_core_", $cpu, "[", $num_cpus * $num_timeparts ,"] __section_bss_core(", $cpu, ");\n";
		print $CFGFILE "struct core_state core_state_core_", $cpu, " __section_bss_core(", $cpu, ");\n";
	}

	# kernel stacks
	print $CFGFILE "/* kernel stacks per CPU for ISRs and exceptions */\n";
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "unsigned char kern_stack_core_", $cpu, "[", $kern_stack_size, "] __aligned(ARCH_KERN_STACK_ALIGN) __section_kern_stack_core(", $cpu, ");\n";
		print $CFGFILE "unsigned char nmi_stack_core_", $cpu, "[", $nmi_stack_size, "] __aligned(ARCH_KERN_STACK_ALIGN) __section_nmi_stack_core(", $cpu, ");\n";
		print $CFGFILE "unsigned char idle_stack_core_", $cpu, "[", $idle_stack_size, "] __aligned(ARCH_KERN_STACK_ALIGN) __section_stack_core(", $cpu, ");\n";
	}
	print $CFGFILE "\n";

	# kernel register contexts
	print $CFGFILE "/* per-CPU kernel register contexts */\n";
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "struct arch_ctxt_frame kern_ctxts_core_", $cpu, "[", $kern_contexts, "] __section_context_core(", $cpu, ");\n";
		print $CFGFILE "struct arch_ctxt_frame nmi_ctxts_core_",  $cpu, "[", $nmi_contexts,  "] __section_context_core(", $cpu, ");\n";
		print $CFGFILE "struct arch_ctxt_frame idle_ctxts_core_", $cpu, "[", $idle_contexts, "] __section_context_core(", $cpu, ");\n";
	}
	print $CFGFILE "\n";

	# per-CPU configuration
	print $CFGFILE "/* per-CPU configuration */\n";
	print $CFGFILE "const struct core_cfg core_cfg[", $num_cpus, "] = {\n";
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "\t/* CPU ", $cpu, " */ {\n";
		print $CFGFILE "\t\t.sched = &sched_state_core_", $cpu, ",\n";
		print $CFGFILE "\t\t.timeparts = timepart_states_core_", $cpu, ",\n";
		print $CFGFILE "\t\t.core_state = &core_state_core_", $cpu, ",\n";

		print $CFGFILE "\t\t.kern_stack = kern_stack_core_", $cpu, ",\n";
		print $CFGFILE "\t\t.nmi_stack =  nmi_stack_core_",  $cpu, ",\n";
		print $CFGFILE "\t\t.idle_stack = idle_stack_core_", $cpu, ",\n";

		print $CFGFILE "\t\t.kern_ctxts = kern_ctxts_core_", $cpu, ",\n";
		print $CFGFILE "\t\t.nmi_ctxts =  nmi_ctxts_core_",  $cpu, ",\n";
		print $CFGFILE "\t\t.idle_ctxts = idle_ctxts_core_", $cpu, ",\n";

		print $CFGFILE "\t},\n";
	}
	print $CFGFILE "};\n";
	print $CFGFILE "\n";

	print $CFGFILE "const size_t kern_stack_size = ", $kern_stack_size, ";\n";
	print $CFGFILE "const size_t nmi_stack_size = ", $nmi_stack_size, ";\n";
	print $CFGFILE "const size_t idle_stack_size = ", $idle_stack_size, ";\n";

	print $CFGFILE "const uint8_t kern_num_ctxts = ", $kern_contexts, ";\n";
	print $CFGFILE "const uint8_t nmi_num_ctxts = ", $nmi_contexts, ";\n";
	print $CFGFILE "const uint8_t idle_num_ctxts = ", $idle_contexts, ";\n";

	print $CFGFILE "\n";

	if ($reloc) {
		# analyse kernel binary to get the final ROM addresses

		my $kernel = $target->{kernel};
		my $layout = $kernel->{layout}[0];
		# NOTE: we do the analysis of the kernel based on the dummy ELF file
		%symhash_kern = sym_readelf($nm, $layout->{dummy_elf});
	}

	my $num_tasks = $num_cpus;	# plus idle tasks in idle partitions;
	my $next_task = $num_tasks;
	my $num_kldds = 0;
	my $next_kldd = 0;
	my $num_ipevs = 0;
	my $next_ipev = 0;
	my $num_counters = 0;
	if ($sys->{counter}) {
		$num_counters += @{$sys->{counter}};
	}
	my $num_ctr_accs = 0;
	my $next_ctr_acc = 0;
	my $num_alarms = 0;
	my $next_alarm = 0;
	my $num_schedtabs = 0;
	my $next_schedtab = 0;
	my $num_waitqueues = 0;
	my $next_waitqueue = 0;
	my $num_shm_accs = 0;
	my $next_shm_acc = 0;
	my $num_rpcs = 0;
	my $next_rpc = 0;
	my %known_shms;

	# known SHMs
	my $num_shms = 0;
	for my $shm (@{$sys->{shm}}) {
		if (defined $known_shms{$shm->{name}}) {
			die "SHM '" . $shm->{name} . "' already exists\n";
		}
		$known_shms{$shm->{name}} = $num_shms;
		$num_shms++;
	}

	# Build a global index of all task, ISR, hook, and invokable IDs
	my $task_array_index = $num_cpus;	# skip idle tasks in idle partitions
	for my $part (@{$sys->{partition}}) {
		my $local_task_array_index = 0;
		# Tasks
		for my $task (@{$part->{task}}) {
			if (defined $known_entities{$part->{name}."::".$task->{name}}) {
				die "task '" . $task->{name} . "' already exists in partition '" . $part->{name} . "'\n";
			}
			$known_entities{$part->{name}."::".$task->{name}} = $task_array_index;
			$known_tasks{$part->{name}."::".$task->{name}} = $task_array_index;
			$known_local_tasks{$part->{name}."::".$task->{name}} = $local_task_array_index;

			$task_array_index++;
			$local_task_array_index++;
		}

		# ISRs
		for my $isr (@{$part->{isr}}) {
			if (defined $known_entities{$part->{name}."::".$isr->{name}}) {
				die "ISR '" . $isr->{name} . "' already exists in partition '" . $part->{name} . "'\n";
			}
			$known_entities{$part->{name}."::".$isr->{name}} = $task_array_index;
			$known_isrs{$part->{name}."::".$isr->{name}} = $task_array_index;
			$known_local_isrs{$part->{name}."::".$isr->{name}} = $local_task_array_index;

			$task_array_index++;
			$local_task_array_index++;
		}

		# Hooks
		for my $hook (@{$part->{hook}}) {
			if (defined $known_entities{$part->{name}."::".$hook->{name}}) {
				die "Hook '" . $hook->{name} . "' already exists in partition '" . $part->{name} . "'\n";
			}
			$known_entities{$part->{name}."::".$hook->{name}} = $task_array_index;
			$known_hooks{$part->{name}."::".$hook->{name}} = $task_array_index;
			$known_local_hooks{$part->{name}."::".$hook->{name}} = $local_task_array_index;

			$task_array_index++;
			$local_task_array_index++;
		}

		# Invokables
		my $invokable_id = 0;
		for my $invokable (@{$part->{invokable}}) {
			if (defined $known_entities{$part->{name}."::".$invokable->{name}}) {
				die "Invokable '" . $invokable->{name} . "' already exists in partition '" . $part->{name} . "'\n";
			}
			$known_entities{$part->{name}."::".$invokable->{name}} = $task_array_index;
			$known_invokables{$part->{name}."::".$invokable->{name}} = $task_array_index;
			$known_local_invokables{$part->{name}."::".$invokable->{name}} = $local_task_array_index;

			$task_array_index++;
			$local_task_array_index++;
			$invokable_id++;
		}
		$known_partition_invokables{$part->{name}} = $invokable_id;

		$known_partition_tasks{$part->{name}} = $local_task_array_index;
	}

	print $CFGFILE "/* idle partition scheduling state */\n";
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "user_sched_state_t idle_user_sched_state_core_", $cpu, " __section_bss_core(", $cpu, ");\n";
	}
	print $CFGFILE "\n";

	print $CFGFILE "/* partition configuration */\n";
	print $CFGFILE "const uint8_t num_partitions __section_cfg = ", $num_parts, ";\n";

	# dynamic partition data
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "struct part part_dyn_idle_", $cpu, " __section_bss_core(", $cpu, ");\n";
	}

	my $part_id = 0;
	for my $part (@{$sys->{partition}}) {
		my $cpu = $part->{cpu};
		if (!defined $cpu) {
			$cpu = 0;
		}
		print $CFGFILE "struct part part_dyn_part_", $part_id, " __section_bss_core(", $cpu, ");\n";
		$part_id++;
	}


	print $CFGFILE "const struct part_cfg part_cfg[", $num_parts, "] = {\n";

	# generate idle partitions (one for each core)
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		my $timepart = 0;

		print $CFGFILE "\t/* idle partition on CPU ", $cpu, " */ {\n";

		print $CFGFILE "\t\t.part = &part_dyn_idle_", $cpu, ",\n";
		print $CFGFILE "\t\t.cpu_id = ", $cpu, ",\n";
		print $CFGFILE "\t\t.tp_id = ", $timepart, ",\n";

		# mandatory magic symbols
		print $CFGFILE "\t\t.user_sched_state = &idle_user_sched_state_core_", $cpu, ",\n";
		print $CFGFILE "\t\t.user_error_state = NULL, /* not used */\n";
		print $CFGFILE "\t\t.user_exception_state = NULL, /* not used */\n";
		print $CFGFILE "\n";

		# partition state
		print $CFGFILE "\t\t.name = \"idle part ", $cpu, "\", /* empty name */\n";

		# partition's tasks, ISRs, hooks, and invokables -> all accounted as tasks!!!
		print $CFGFILE "\t\t.tasks = task_dyn_idle_", $cpu, ",\n";
		print $CFGFILE "\t\t.init_hook_id = 0,\n";
		print $CFGFILE "\t\t.error_hook_id = 0xffff, /* not used */\n";
		print $CFGFILE "\t\t.exception_hook_id = 0xffff, /* not used */\n";
		print $CFGFILE "\t\t.num_error_states = 0, /* not used */\n";
		print $CFGFILE "\t\t.num_tasks = 1,\n";

		print $CFGFILE "\t\t.mpu_part_cfg = &mpu_part_cfg[", $cpu, "],\n";

		print $CFGFILE "\t\t.part_id = ", $cpu, ",\n";
		print $CFGFILE "\t\t.flags = 0,\n";
		print $CFGFILE "\t\t.initial_operating_mode = PART_OPERATING_MODE_NORMAL,\n";
		print $CFGFILE "\t},\n";
	}

	# iterate partitions
	my $part_cnt = $num_cpus;	# skip idle partitions
	$part_id = 0;

	for my $part (@{$sys->{partition}}) {

		if (defined $known_partitions{$part->{name}}) {
			die "partition '" . $part->{name} . "' already exists\n";
		}
		$known_partitions{$part->{name}} = $part_cnt;
		my $cpu = $part->{cpu};
		if (!defined $cpu) {
			$cpu = 0;
		}
		$known_partition_cpus{$part->{name}} = $cpu;
		my $timepart = $part->{timepart};
		if (!defined $timepart) {
			$timepart = 0;
		}

		print $CFGFILE "\t/* partition '", $part->{name}, "' on CPU ", $cpu, " */ {\n";

		print $CFGFILE "\t\t.part = &part_dyn_part_", $part_id, ",\n";
		print $CFGFILE "\t\t.cpu_id = ", $cpu, ",\n";
		print $CFGFILE "\t\t.tp_id = ", $timepart, ",\n";

		my $user_sched_state = 0;
		my $user_error_state = 0;
		my $user_exception_state = 0;
		my $user_sched_state_name = "NULL";
		my $user_error_state_name = "NULL";
		my $user_exception_state_name = "NULL";
		my $sda1_base = 0;
		my $sda2_base = 0;
		my $sda1_name = "NULL";
		my $sda2_name = "NULL";

		my $layout = $part->{layout}[0];
		if ($reloc) {
			%symhash_part = sym_readelf($nm, $appdir . "/" . $layout->{final_elf});

			# Magic symbols
			$user_sched_state_name = $part->{sched_state};
			$user_sched_state = sym_eval(\%symhash_part, $user_sched_state_name);
			if (defined $part->{error_state}) {
				$user_error_state_name = $part->{error_state};
				$user_error_state = sym_eval(\%symhash_part, $user_error_state_name);
			}
			if (defined $part->{exception_state}) {
				$user_exception_state_name = $part->{exception_state};
				$user_exception_state = sym_eval(\%symhash_part, $user_exception_state_name);
			}
			if (defined $part->{sda1_base}) {
				$sda1_name = $part->{sda1_base};
				$sda1_base = sym_eval(\%symhash_part, $sda1_name);
			}
			if (defined $part->{sda2_base}) {
				$sda2_name = $part->{sda2_base};
				$sda2_base = sym_eval(\%symhash_part, $sda2_name);
			}

			# Memory ranges
			print $CFGFILE "\t\t.mem_ranges = {\n";
			for my $range (@{$layout->{range}}) {
				my $start = $range->{start};
				my $end = $range->{end};

				my $start_sym = sym_eval(\%symhash_part, $start);
				my $end_sym = sym_eval(\%symhash_part, $end);
				print $CFGFILE "\t\t\t{ .start = ", hexify($start_sym), ", .end = ", hexify($end_sym), " }, /* ", $start, " -- ", $end, " */\n";

			}
			print $CFGFILE "\t\t},\n";
		}

		# ARINC attributes
		my $period = $system_period;
		if (defined $part->{period}) {
			$period = $part->{period};
		}
		print $CFGFILE "\t\t.period = ", $period, ",\n";
		my $duration = $period;
		if (defined $part->{duration}) {
			$duration = $part->{duration};
		}
		print $CFGFILE "\t\t.duration = ", $duration, ",\n";

		# magic symbols
		print $CFGFILE "\t\t.user_sched_state = (user_sched_state_t *)", hexify($user_sched_state), ", /* ", $user_sched_state_name, " */\n";
		print $CFGFILE "\t\t.user_error_state = (user_error_state_t *)", hexify($user_error_state), ", /* ", $user_error_state_name, " */\n";
		print $CFGFILE "\t\t.user_exception_state = (user_exception_state_t *)", hexify($user_exception_state), ", /* ", $user_exception_state_name, " */\n";
		print $CFGFILE "\n";

		# partition's tasks, ISRs, hooks, and invokables -> all accounted as tasks!!!
		print $CFGFILE "\t\t.tasks = task_dyn_part_", $part_cnt, ",\n";
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
		print $CFGFILE "\t\t.num_tasks = ", $num_tasks - $next_task, ",\n";
		$next_task = $num_tasks;

		print $CFGFILE "\t\t.mpu_part_cfg = &mpu_part_cfg[", $part_cnt, "],\n";

		# partition's KLDDs
		print $CFGFILE "\t\t.kldds = &kldd_cfg[", $next_kldd, "],\n";
		if (defined $part->{kldd}) {
			$num_kldds += @{$part->{kldd}};
		}
		print $CFGFILE "\t\t.num_kldds = ", $num_kldds - $next_kldd, ",\n";
		$next_kldd = $num_kldds;

		# partition's IPEVs
		print $CFGFILE "\t\t.ipevs = &ipev_cfg[", $next_ipev, "],\n";
		if (defined $part->{ipev}) {
			$num_ipevs += @{$part->{ipev}};
		}
		print $CFGFILE "\t\t.num_ipevs = ", $num_ipevs - $next_ipev, ",\n";
		$next_ipev = $num_ipevs;

		# partition's counter accesses
		print $CFGFILE "\t\t.ctr_accs = &counter_access[", $next_ctr_acc, "],\n";
		if (defined $part->{counter_access}) {
			$num_ctr_accs += @{$part->{counter_access}};
		}
		print $CFGFILE "\t\t.num_ctr_accs = ", $num_ctr_accs - $next_ctr_acc, ",\n";
		$next_ctr_acc = $num_ctr_accs;

		# partition's alarms
		print $CFGFILE "\t\t.alarms = alarm_dyn_part_", $part_cnt, ",\n";
		if (defined $part->{alarm}) {
			$num_alarms += @{$part->{alarm}};
		}
		$known_partition_alarms{$part->{name}} = $num_alarms - $next_alarm;
		print $CFGFILE "\t\t.num_alarms = ", $num_alarms - $next_alarm, ",\n";
		$next_alarm = $num_alarms;

		# partition's schedtabs
		print $CFGFILE "\t\t.schedtabs = schedtab_dyn_part_", $part_cnt, ",\n";
		if (defined $part->{sched_table}) {
			for my $schedtab (@{$part->{sched_table}}) {
				$known_schedtab_cpus{$num_schedtabs} = $cpu;
				$num_schedtabs++;
			}
		}
		$known_partition_schedtabs{$part->{name}} = $num_schedtabs - $next_schedtab;
		print $CFGFILE "\t\t.num_schedtabs = ", $num_schedtabs - $next_schedtab, ",\n";
		$next_schedtab = $num_schedtabs;

		# partition's wait queues
		print $CFGFILE "\t\t.wq_cfgs = &wq_cfg[", $next_waitqueue, "],\n";
		print $CFGFILE "\t\t.wqs = wq_dyn_part_", $part_cnt, ",\n";
		if (defined $part->{wait_queue}) {
			$num_waitqueues += @{$part->{wait_queue}};
		}
		$known_partition_wqs{$part->{name}} = $num_waitqueues - $next_waitqueue;
		print $CFGFILE "\t\t.num_wqs = ", $num_waitqueues - $next_waitqueue, ",\n";
		$next_waitqueue = $num_waitqueues;

		# partition's SHM accesses
		print $CFGFILE "\t\t.shm_accs = &shm_access[", $next_shm_acc, "],\n";
		if (defined $part->{shm_access}) {
			$num_shm_accs += @{$part->{shm_access}};
		}
		print $CFGFILE "\t\t.num_shm_accs = ", $num_shm_accs - $next_shm_acc, ",\n";
		$next_shm_acc = $num_shm_accs;

		# partition's RPC
		print $CFGFILE "\t\t.rpcs = &rpc_cfg[", $next_rpc, "],\n";
		if (defined $part->{rpc}) {
			$num_rpcs += @{$part->{rpc}};
		}
		print $CFGFILE "\t\t.num_rpcs = ", $num_rpcs - $next_rpc, ",\n";
		$next_rpc = $num_rpcs;

		# Partition's init hook
		my $init_hook_name = $part->{name} . "::" . $part->{init_hook};
		if (!defined $known_local_hooks{$init_hook_name}) {
			die "init_hook '" . $init_hook_name . "' does not exists\n";
		}
		print $CFGFILE "\t\t.init_hook_id = ", $known_local_hooks{$init_hook_name}, ",\n";

		# Partition's error hook
		if (defined $part->{error_hook}) {
			my $error_hook_name = $part->{name} . "::" . $part->{error_hook};
			my $num_error_states = $part->{error_records};
			if (!defined $known_local_hooks{$error_hook_name}) {
				die "error_hook '" . $error_hook_name . "' does not exists\n";
			}
			print $CFGFILE "\t\t.error_hook_id = ", $known_local_hooks{$error_hook_name}, ",\n";
			print $CFGFILE "\t\t.num_error_states = ", $num_error_states,",\n";
		} else {
			print $CFGFILE "\t\t.error_hook_id = 0xffff, /* not used */\n";
			print $CFGFILE "\t\t.num_error_states = 0, /* not used */\n";
		}

		# Partition's exception hook
		if (defined $part->{exception_hook}) {
			my $exception_hook_name = $part->{name} . "::" . $part->{exception_hook};
			if (!defined $known_local_hooks{$exception_hook_name}) {
				die "exception_hook '" . $exception_hook_name . "' does not exists\n";
			}
			print $CFGFILE "\t\t.exception_hook_id = ", $known_local_hooks{$exception_hook_name}, ",\n";
		} else {
			print $CFGFILE "\t\t.exception_hook_id = 0xffff, /* not used */\n";
		}

		# Partition's max_prio
		print $CFGFILE "\t\t.max_prio = ", $part->{max_prio}, ",\n";

		# partition state
		print $CFGFILE "\t\t.name = \"", $part->{name}, "\",\n";
		print $CFGFILE "\t\t.part_id = ", $part_cnt, ",\n";
		print $CFGFILE "\t\t.flags = ";
		if ($part->{flags} ne "") {
			print $CFGFILE $part->{flags};
		} else {
			print $CFGFILE "0";
		}
		print $CFGFILE ",\n";
		my $operating_mode = "PART_OPERATING_MODE_COLD_START";
		if (defined $part->{mode}) {
			if (uc $part->{mode} eq "COLD_START") {
				# already set as default
			} elsif (uc $part->{mode} eq "IDLE") {
				$operating_mode = "PART_OPERATING_MODE_IDLE";
			} else {
				die "partition '" . $part->{name} . "' has invalid mode\n";
			}
		}
		print $CFGFILE "\t\t.initial_operating_mode = ", $operating_mode, ",\n";

		print $CFGFILE "\t\t.sda1_base = ", hexify($sda1_base), ", /* ", $sda1_name, " */\n";
		print $CFGFILE "\t\t.sda2_base = ", hexify($sda2_base), ", /* ", $sda2_name, " */\n";
		print $CFGFILE "\t},\n";

		$part_cnt++;
		$part_id++;
	}
	print $CFGFILE "};\n";
	print $CFGFILE "\n";

	# iterate tasks in partitions
	print $CFGFILE "/* task configuration */\n";
	print $CFGFILE "const uint16_t num_tasks __section_cfg = ", $num_tasks, ";\n";
	print $CFGFILE "const struct task_cfg task_cfg[", $num_tasks, "] = {\n";

	# generate idle tasks in idle partitions (one for each core)
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "\t/* idle task on CPU $cpu */ {\n";

		print $CFGFILE "\t\t.task = &task_dyn_idle_", $cpu, "[0],\n";
		print $CFGFILE "\t\t.name = \"idle task ", $cpu, "\",\n";
		print $CFGFILE "\t\t.task_id = 0,\n";
		print $CFGFILE "\t\t.cpu_id = ", $cpu, ",\n";
		print $CFGFILE "\t\t.part_cfg = &part_cfg[$cpu],\n";
		print $CFGFILE "\t\t.timepart = &timepart_states_core_", $cpu, "[0],\n";


		print $CFGFILE "\t\t.base_prio = 0,\n";
		print $CFGFILE "\t\t.elev_prio = 0, /* not used */\n";
		print $CFGFILE "\t\t.regs = &kernel_reg_frames_core_", $cpu, "[", $core_reg_frames{$cpu}, "],\n";
		$core_reg_frames{$cpu}++;
		print $CFGFILE "\t\t.fpu = NULL, /* not used */\n";

		# register contexts for Tricore (idle tasks use partition context)
		print $CFGFILE "\t\t.ctxt = &kernel_ctxt_frames_core_", $cpu, "[", $core_ctxt_frames{$cpu}, "],\n";
		print $CFGFILE "\t\t.num_ctxts = ", $idle_contexts, ",\n";
		$core_ctxt_frames{$cpu} += $idle_contexts;

		print $CFGFILE "\t\t.mpu_task_cfg = &mpu_task_cfg[", $cpu, "],\n";

		print $CFGFILE "\t\t.irq = 0, /* not used */\n";
		print $CFGFILE "\t\t.max_activations = 1, /* not used */\n";

		print $CFGFILE "\t\t.cfgflags_type = TASK_TYPE_HOOK,\n";

		print $CFGFILE "\t},\n";
	}

	$part_cnt = $num_cpus;	# skip idle partitions
	$task_array_index = $num_cpus;	# skip idle tasks in idle partitions
	for my $part (@{$sys->{partition}}) {
		my $layout = $part->{layout}[0];
		my $elf_file = $appdir . "/" . $layout->{final_elf};
		my $task_cnt = 0;
		my $cpu = $part->{cpu};
		if (!defined $cpu) {
			$cpu = 0;
		}
		my $timepart = $part->{timepart};
		if (!defined $timepart) {
			$timepart = 0;
		}

		# Tasks
		for my $task (@{$part->{task}}) {
			my $elev_prio;
			my $max_activations = 1;
			my $period = -1;	# default: infinite time (aperiodic process)
			my $capacity = -1;	# default: infinite time

			my $blocking = 0;
			if (defined $task->{blocking} && $task->{blocking} eq "yes") {
				$blocking = 1;
			} else {
				if (defined $task->{activations}) {
					$max_activations = $task->{activations};
				}
			}
			if ($max_activations < 1 || $max_activations > 255) {
				die "error: basic task '", $task->{name}, "' in partition '", $part->{name},
				    "' number of activations ", $max_activations, " out of bounds (1..255)\n";
			}

			if (defined $task->{period}) {
				$period = $task->{period};
			}
			if (defined $task->{capacity}) {
				$capacity = $task->{capacity};
			}

			print $CFGFILE "\t/* #", $task_array_index, ": ", ($blocking?"blocking ":""), "task '", $task->{name}, "' in partition '", $part->{name}, "' */ {\n";
			print $CFGFILE "\t\t.task = &task_dyn_part_", $part_cnt, "[", $task_cnt, "],\n";
			print $CFGFILE "\t\t.name = \"", $task->{name}, "\",\n";
			print $CFGFILE "\t\t.task_id = ", $task_cnt, ",\n";
			print $CFGFILE "\t\t.cpu_id = ", $cpu, ",\n";
			print $CFGFILE "\t\t.part_cfg = &part_cfg[", $part_cnt, "],\n";
			print $CFGFILE "\t\t.timepart = &timepart_states_core_", $cpu, "[", $timepart, "],\n";
			print $CFGFILE "\n";

			# ARINC attributes
			print $CFGFILE "\t\t.period = ", $period, ",\n";
			print $CFGFILE "\t\t.capacity = ", $capacity, ",\n";
			print $CFGFILE "\n";

			print $CFGFILE "\t\t.base_prio = ", $task->{prio}, ",\n";
			print $CFGFILE "\t\t.elev_prio = ";
			# elevated prios are used for two purposes:
			# - non preemptible tasks
			# - group scheduling
			if (defined $task->{eprio}) {
				print $CFGFILE $task->{eprio}, ", /* elevated prio */\n";
				$elev_prio = 1;
			} else {
				print $CFGFILE $task->{prio}, ",\n";
				$elev_prio = 0;
			}

			print $CFGFILE "\t\t.regs = &kernel_reg_frames_core_", $cpu, "[", $core_reg_frames{$cpu}, "],\n";
			$core_reg_frames{$cpu}++;
			print $CFGFILE "\t\t.fpu = ";
			if ($task->{fpu} eq "yes") {
				print $CFGFILE "&kernel_fpu_frames_core_", $cpu, "[", $core_fpu_frames{$cpu}, "],\n";
				$core_fpu_frames{$cpu}++;
			} else {
				print $CFGFILE "NULL, /* not used */\n";
			}
			# register contexts for Tricore
			my $contexts = 0;
			if (defined $task->{contexts}) {
				$contexts = number $task->{contexts};
			}
			print $CFGFILE "\t\t.ctxt = &kernel_ctxt_frames_core_", $cpu, "[", $core_ctxt_frames{$cpu}, "],\n";
			print $CFGFILE "\t\t.num_ctxts = ", $contexts, ",\n";
			$core_ctxt_frames{$cpu} += $contexts;

			print $CFGFILE "\t\t.mpu_task_cfg = &mpu_task_cfg[", $task_array_index, "],\n";

			print $CFGFILE "\t\t.irq = 0, /* not used */\n";
			print $CFGFILE "\t\t.max_activations = ", $max_activations, ",\n";
			print $CFGFILE "\n";

			# invoke block
			my $activatable = gen_invoke($CFGFILE, $reloc, $task->{invoke}[0], $elf_file);

			print $CFGFILE "\n";
			print $CFGFILE "\t\t.cfgflags_type = TASK_TYPE_TASK";
			if ($activatable != 0) {
				print $CFGFILE " | TASK_CFGFLAG_ACTIVATABLE";
			}
			if ($blocking != 0) {
				print $CFGFILE " | TASK_CFGFLAG_MAYBLOCK";
			}
			if ($elev_prio != 0) {
				print $CFGFILE " | TASK_CFGFLAG_ELEV_PRIO";
			}
			print $CFGFILE ",\n";
			print $CFGFILE "\t},\n";
			$task_cnt++;
			$task_array_index++;
		}

		# ISRs
		for my $isr (@{$part->{isr}}) {
			my $vector;

			my $blocking = 0;
			if (defined $isr->{blocking} && $isr->{blocking} eq "yes") {
				$blocking = 1;
			}

			print $CFGFILE "\t/* #", $task_array_index, ": ", ($blocking?"blocking ":""), "ISR '", $isr->{name}, "' in partition '", $part->{name}, "' */ {\n";
			print $CFGFILE "\t\t.task = &task_dyn_part_", $part_cnt, "[", $task_cnt, "],\n";
			print $CFGFILE "\t\t.name = \"", $isr->{name}, "\",\n";
			print $CFGFILE "\t\t.task_id = ", $task_cnt, ",\n";
			print $CFGFILE "\t\t.cpu_id = ", $cpu, ",\n";
			print $CFGFILE "\t\t.part_cfg = &part_cfg[", $part_cnt, "],\n";
			print $CFGFILE "\t\t.timepart = &timepart_states_core_", $cpu, "[", $timepart, "],\n";
			print $CFGFILE "\n";

			print $CFGFILE "\t\t.base_prio = ", $isr->{prio}, ",\n";
			print $CFGFILE "\t\t.elev_prio = ", $isr->{prio}, ", /* not used */\n";
			print $CFGFILE "\t\t.regs = &kernel_reg_frames_core_", $cpu, "[", $core_reg_frames{$cpu}, "],\n";
			$core_reg_frames{$cpu}++;
			print $CFGFILE "\t\t.fpu = ";
			if ($isr->{fpu} eq "yes") {
				print $CFGFILE "&kernel_fpu_frames_core_", $cpu, "[", $core_fpu_frames{$cpu}, "],\n";
				$core_fpu_frames{$cpu}++;
			} else {
				print $CFGFILE "NULL, /* not used */\n";
			}
			# register contexts for Tricore
			my $contexts = 0;
			if (defined $isr->{contexts}) {
				$contexts = number $isr->{contexts};
			}
			print $CFGFILE "\t\t.ctxt = &kernel_ctxt_frames_core_", $cpu, "[", $core_ctxt_frames{$cpu}, "],\n";
			print $CFGFILE "\t\t.num_ctxts = ", $contexts, ",\n";
			$core_ctxt_frames{$cpu} += $contexts;

			print $CFGFILE "\t\t.mpu_task_cfg = &mpu_task_cfg[", $task_array_index, "],\n";

			$vector = $isr->{vector};
			if ($vector >= $num_isrs) {
				die "error: ISR '", $isr->{name}, "' in partition '", $part->{name},
				    "' vector ", $vector, " out of bounds (0..", $num_isrs-1, ")\n";
			}
			if (defined $known_isrs_user[$vector]) {
				die "error: ISR 'h", $isr->{name}, "' in partition '", $part->{name},
				    "' vector ", $vector, " already allocated\n";
			}
			$known_isrs_user[$vector] = $task_array_index;
			print $CFGFILE "\t\t.irq = ", $vector, ",\n";
			print $CFGFILE "\t\t.max_activations = 1, /* not used */\n";
			print $CFGFILE "\n";

			# invoke block
			my $activatable = gen_invoke($CFGFILE, $reloc, $isr->{invoke}[0], $elf_file);

			print $CFGFILE "\n";
			print $CFGFILE "\t\t.cfgflags_type = TASK_TYPE_ISR";
			if ($activatable != 0) {
				print $CFGFILE " | TASK_CFGFLAG_ACTIVATABLE";
			}
			if ($blocking != 0) {
				print $CFGFILE " | TASK_CFGFLAG_MAYBLOCK";
			}
			if (defined $isr->{unmask} && $isr->{unmask} eq "yes") {
				print $CFGFILE " | TASK_CFGFLAG_ISR_UNMASK";
			}
			print $CFGFILE ",\n";
			print $CFGFILE "\t},\n";
			$task_cnt++;
			$task_array_index++;
		}

		# Hooks
		for my $hook (@{$part->{hook}}) {
			my $blocking = 0;
			if (defined $hook->{blocking} && $hook->{blocking} eq "yes") {
				$blocking = 1;
			}

			print $CFGFILE "\t/* #", $task_array_index, ": ", ($blocking?"blocking ":""), "Hook '", $hook->{name}, "' in partition '", $part->{name}, "' */ {\n";
			print $CFGFILE "\t\t.task = &task_dyn_part_", $part_cnt, "[", $task_cnt, "],\n";
			print $CFGFILE "\t\t.name = \"", $hook->{name}, "\",\n";
			print $CFGFILE "\t\t.task_id = ", $task_cnt, ",\n";
			print $CFGFILE "\t\t.cpu_id = ", $cpu, ",\n";
			print $CFGFILE "\t\t.part_cfg = &part_cfg[", $part_cnt, "],\n";
			print $CFGFILE "\t\t.timepart = &timepart_states_core_", $cpu, "[", $timepart, "],\n";
			print $CFGFILE "\n";

			print $CFGFILE "\t\t.base_prio = ", $hook->{prio}, ",\n";
			print $CFGFILE "\t\t.elev_prio = ", $hook->{prio}, ", /* not used */\n";
			print $CFGFILE "\t\t.regs = &kernel_reg_frames_core_", $cpu, "[", $core_reg_frames{$cpu}, "],\n";
			$core_reg_frames{$cpu}++;
			print $CFGFILE "\t\t.fpu = ";
			if ($hook->{fpu} eq "yes") {
				print $CFGFILE "&kernel_fpu_frames_core_", $cpu, "[", $core_fpu_frames{$cpu}, "],\n";
				$core_fpu_frames{$cpu}++;
			} else {
				print $CFGFILE "NULL, /* not used */\n";
			}
			# register contexts for Tricore
			my $contexts = 0;
			if (defined $hook->{contexts}) {
				$contexts = number $hook->{contexts};
			}
			print $CFGFILE "\t\t.ctxt = &kernel_ctxt_frames_core_", $cpu, "[", $core_ctxt_frames{$cpu}, "],\n";
			print $CFGFILE "\t\t.num_ctxts = ", $contexts, ",\n";
			$core_ctxt_frames{$cpu} += $contexts;

			print $CFGFILE "\t\t.mpu_task_cfg = &mpu_task_cfg[", $task_array_index, "],\n";

			print $CFGFILE "\t\t.irq = 0, /* not used */\n";

			if (defined $part->{error_hook} && $part->{error_hook} eq $hook->{name}) {
				print $CFGFILE "\t\t.max_activations = ", $part->{error_records}, ", /* error hook */\n";
			} else {
				print $CFGFILE "\t\t.max_activations = 1, /* not used */\n";
			}
			print $CFGFILE "\n";

			# invoke block
			my $activatable = gen_invoke($CFGFILE, $reloc, $hook->{invoke}[0], $elf_file);

			print $CFGFILE "\n";
			print $CFGFILE "\t\t.cfgflags_type = TASK_TYPE_HOOK";
			if ($activatable != 0) {
				print $CFGFILE " | TASK_CFGFLAG_ACTIVATABLE";
			}
			if ($blocking != 0) {
				print $CFGFILE " | TASK_CFGFLAG_MAYBLOCK";
			}
			print $CFGFILE ",\n";
			print $CFGFILE "\t},\n";
			$task_cnt++;
			$task_array_index++;
		}

		# Invokables (very similar to hooks)
		my $invokable_id = 0;
		for my $invokable (@{$part->{invokable}}) {
			my $blocking = 0;
			if (defined $invokable->{blocking} && $invokable->{blocking} eq "yes") {
				$blocking = 1;
			}

			print $CFGFILE "\t/* #", $task_array_index, ": ", ($blocking?"blocking ":""), "Invokable '", $invokable->{name}, "' in partition '", $part->{name}, "' */ {\n";
			print $CFGFILE "\t\t.task = &task_dyn_part_", $part_cnt, "[", $task_cnt, "],\n";
			print $CFGFILE "\t\t.name = \"", $invokable->{name}, "\",\n";
			print $CFGFILE "\t\t.task_id = ", $task_cnt, ",\n";
			print $CFGFILE "\t\t.cpu_id = ", $cpu, ",\n";
			print $CFGFILE "\t\t.part_cfg = &part_cfg[", $part_cnt, "],\n";
			print $CFGFILE "\t\t.timepart = &timepart_states_core_", $cpu, "[", $timepart, "],\n";
			print $CFGFILE "\n";

			print $CFGFILE "\t\t.base_prio = ", $invokable->{prio}, ",\n";
			print $CFGFILE "\t\t.elev_prio = ", $invokable->{prio}, ", /* not used */\n";
			print $CFGFILE "\t\t.regs = &kernel_reg_frames_core_", $cpu, "[", $core_reg_frames{$cpu}, "],\n";
			$core_reg_frames{$cpu}++;
			print $CFGFILE "\t\t.fpu = ";
			if ($invokable->{fpu} eq "yes") {
				print $CFGFILE "&kernel_fpu_frames_core_", $cpu, "[", $core_fpu_frames{$cpu}, "],\n";
				$core_fpu_frames{$cpu}++;
			} else {
				print $CFGFILE "NULL, /* not used */\n";
			}
			# register contexts for Tricore
			my $contexts = 0;
			if (defined $invokable->{contexts}) {
				$contexts = number $invokable->{contexts};
			}
			print $CFGFILE "\t\t.ctxt = &kernel_ctxt_frames_core_", $cpu, "[", $core_ctxt_frames{$cpu}, "],\n";
			print $CFGFILE "\t\t.num_ctxts = ", $contexts, ",\n";
			$core_ctxt_frames{$cpu} += $contexts;

			print $CFGFILE "\t\t.mpu_task_cfg = &mpu_task_cfg[", $task_array_index, "],\n";

			print $CFGFILE "\t\t.irq = 0, /* not used */\n";

			print $CFGFILE "\t\t.rpc = &rpc_dyn_part_", $part_cnt, "[", $invokable_id, "],\n";
			print $CFGFILE "\t\t.max_activations = 1, /* not used */\n";
			print $CFGFILE "\n";

			# invoke block
			my $activatable = gen_invoke($CFGFILE, $reloc, $invokable->{invoke}[0], $elf_file);

			print $CFGFILE "\n";
			print $CFGFILE "\t\t.cfgflags_type = TASK_TYPE_INVOKABLE";
			if ($activatable != 0) {
				print $CFGFILE " | TASK_CFGFLAG_ACTIVATABLE";
			}
			if ($blocking != 0) {
				print $CFGFILE " | TASK_CFGFLAG_MAYBLOCK";
			}
			print $CFGFILE ",\n";
			print $CFGFILE "\t},\n";
			$task_cnt++;
			$task_array_index++;
			$invokable_id++;
		}

		$part_cnt++;
	}
	print $CFGFILE "};\n";

	# generate dynamic task data, one array per partitions
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "struct task task_dyn_idle_", $cpu, "[1] __section_bss_core(", $cpu, ");\n";
	}
	$part_cnt = $num_cpus;	# skip idle partitions
	for my $part (@{$sys->{partition}}) {
		my $cpu = $known_partition_cpus{$part->{name}};
		my $tasks = $known_partition_tasks{$part->{name}};

		print $CFGFILE "struct task task_dyn_part_", $part_cnt, "[", $tasks, "] __section_bss_core(", $cpu, ");\n";

		$part_cnt++;
	}
	print $CFGFILE "\n";

	# space for register frames
	print $CFGFILE "/* registers contexts (one for each partition + shared regs for tasks) */\n";
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "struct arch_reg_frame kernel_reg_frames_core_", $cpu, "[", $core_reg_frames{$cpu}, "] __section_reg_core(", $cpu, ");\n";
	}
	print $CFGFILE "\n";

	# space for FPU contexts
	print $CFGFILE "/* FPU contexts (tasks can disable or share FPU registers) */\n";
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "struct arch_fpu_frame kernel_fpu_frames_core_", $cpu, "[", $core_fpu_frames{$cpu}, "] __section_reg_core(", $cpu, ");\n";
	}
	print $CFGFILE "\n";

	# space for registers contexts (Tricore)
	print $CFGFILE "/* Hardware registers contexts (variable per task) */\n";
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "struct arch_ctxt_frame kernel_ctxt_frames_core_", $cpu, "[", $core_ctxt_frames{$cpu}, "] __section_context_core(", $cpu, ");\n";
	}
	print $CFGFILE "\n";

	# iterate KLDDs in partitions
	print $CFGFILE "/* KLDD calltables */\n";
	print $CFGFILE "const struct kldd_cfg kldd_cfg[", $num_kldds, "] = {\n";
	for my $part (@{$sys->{partition}}) {
		my $kldd_id = 0;
		for my $kldd (@{$part->{kldd}}) {
			my $kldd_func = 0;
			my $kldd_arg0 = 0;

			if ($reloc) {
				$kldd_func = sym_eval(\%symhash_kern, $kldd->{entry});
				$kldd_arg0 = sym_eval(\%symhash_kern, $kldd->{arg});
			}

			print $CFGFILE "\t/* kldd '", $kldd_id, "' in partition '", $part->{name}, "' */ {\n";
			print $CFGFILE "\t\t.func = (void*)", hexify($kldd_func), ", /* ", $kldd->{entry}, " */\n";
			print $CFGFILE "\t\t.arg0 = (void*)", hexify($kldd_arg0), ", /* ", $kldd->{arg}, " */\n";
			print $CFGFILE "\t},\n";
		}
		$kldd_id++;
	}
	print $CFGFILE "};\n";
	print $CFGFILE "\n";

	# iterate IPEVs in partitions
	print $CFGFILE "/* IPEV table */\n";
	print $CFGFILE "const struct ipev_cfg ipev_cfg[", $num_ipevs, "] = {\n";
	for my $part (@{$sys->{partition}}) {
		my $ipev_id = 0;
		for my $ipev (@{$part->{ipev}}) {
			my $ipev_part = $ipev->{partition};
			my $ipev_task = $ipev->{task};
			my $ipev_bit  = $ipev->{bit};

			if ($ipev_bit < 0 || $ipev_bit > 31) {
				die "error: IPEV ", $ipev_id, " in partition '", $part->{name},
				    "': bit out of bounds (0..31)\n";
			}
			if (!defined $known_tasks{$ipev_part . "::" . $ipev_task}) {
				die "error: IPEV ", $ipev_id, " in partition '", $part->{name},
				    "': partition or task '", $ipev_part, "' not found\n";
			}

			my $ipev_global_task_id = $known_tasks{$ipev_part . "::" . $ipev_task};
			print $CFGFILE "\t/* ipev ", $ipev_id, " in partition '", $part->{name}, "' */ {\n";
			print $CFGFILE "\t\t.global_task_id = ", $ipev_global_task_id, ", /* part '", $ipev_part, "' task '", $ipev_task, "' */\n";
			print $CFGFILE "\t\t.mask_bit = ", $ipev_bit, ", /* bit ", $ipev_bit, " */\n";
			print $CFGFILE "\t},\n";
		}
		$ipev_id++;
	}
	print $CFGFILE "};\n";
	print $CFGFILE "\n";

	# iterate counters
	print $CFGFILE "/* global counter table */\n";
	print $CFGFILE "const uint8_t num_counters __section_cfg = ", $num_counters, ";\n";

	my $counter_cnt = 0;
	for my $cnt (@{$sys->{counter}}) {
		my $cpu = $cnt->{cpu};
		if (!defined $cpu) {
			$cpu = 0;
		}
		print $CFGFILE "struct counter counter_dyn_", $counter_cnt, " __section_bss_core(", $cpu, ");\n";
		$counter_cnt++;
	}

	print $CFGFILE "const struct counter_cfg counter_cfg[", $num_counters, "] = {\n";

	$counter_cnt = 0;
	for my $cnt (@{$sys->{counter}}) {
		if (defined $known_counters{$cnt->{name}}) {
			die "counter '" . $cnt->{name} . "' already exists\n";
		}
		$known_counters{$cnt->{name}} = $counter_cnt;

		my $counter_maxallowedvalue = $cnt->{maxallowedvalue};
		my $counter_ticksperbase = $cnt->{ticksperbase};
		my $counter_mincycle = $cnt->{mincycle};
		my $counter_type = $cnt->{type};
		my $cpu = $cnt->{cpu};
		if (!defined $cpu) {
			$cpu = 0;
		}

		if ($counter_maxallowedvalue == 0) {
			die "counter '" . $cnt->{name} . "' maxallowedvalue is zero\n";
		}
		if ($counter_ticksperbase > $counter_maxallowedvalue) {
			die "counter '" . $cnt->{name} . "' ticksperbase out of range\n";
		}
		if ($counter_mincycle > $counter_maxallowedvalue) {
			die "counter '" . $cnt->{name} . "' mincycle out of range\n";
		}
		if ($counter_type eq 'hw') {
			$counter_type = 'COUNTER_TYPE_HW';
		} elsif ($counter_type eq 'sw') {
			$counter_type = 'COUNTER_TYPE_SW';
		} else {
			die "counter '" . $cnt->{name} . "' unknown type\n";
		}

		print $CFGFILE "\t/* counter ", $counter_cnt, " '", $cnt->{name}, "' */ {\n";
		print $CFGFILE "\t\t.counter = &counter_dyn_", $counter_cnt, ",\n";
		if ($reloc) {
			if ($cnt->{type} eq 'hw') {
				my $r = sym_eval(\%symhash_kern, $cnt->{register});
				my $q = sym_eval(\%symhash_kern, $cnt->{query});
				my $c = sym_eval(\%symhash_kern, $cnt->{change});
				print $CFGFILE "\t\t.reg = (void*)", hexify($r), ", /* ", $cnt->{register}, " */\n";
				print $CFGFILE "\t\t.query = (void*)", hexify($q), ", /* ", $cnt->{query}, " */\n";
				print $CFGFILE "\t\t.change = (void*)", hexify($c), ", /* ", $cnt->{change}," */\n";
			} else {
				print $CFGFILE "\t\t.reg = NULL, /* not used */\n";
				print $CFGFILE "\t\t.query = NULL, /* not used */\n";
				print $CFGFILE "\t\t.change = NULL, /* not used */\n";
			}
		}
		print $CFGFILE "\t\t.maxallowedvalue = ", $counter_maxallowedvalue, ",\n";
		print $CFGFILE "\t\t.ticksperbase = ", $counter_ticksperbase, ",\n";
		print $CFGFILE "\t\t.mincycle = ", $counter_mincycle, ",\n";
		print $CFGFILE "\t\t.type = ", $counter_type, ",\n";
		print $CFGFILE "\t\t.cpu_id = ", $cpu, ",\n";
		print $CFGFILE "\t},\n";

		$counter_cnt++;
	}

	print $CFGFILE "};\n";
	print $CFGFILE "\n";

	# iterate counter accesses per partition
	print $CFGFILE "/* partition counter access table */\n";
	print $CFGFILE "const struct counter_access counter_access[", $num_ctr_accs, "] = {\n";

	for my $part (@{$sys->{partition}}) {
		my $ctr_acc_id = 0;

		for my $ctr_acc (@{$part->{counter_access}}) {
			my $c = $ctr_acc->{counter};
			if (!defined $known_counters{$c}) {
				die "counter '" . $c . "' does not exists\n";
			}

			print $CFGFILE "\t/* partition '", $part->{name}, "' index ", $ctr_acc_id, " '", $c, "' */ {\n";
			print $CFGFILE "\t\t.counter_cfg = &counter_cfg[", $known_counters{$c}, "],\n";
			print $CFGFILE "\t},\n";
			$ctr_acc_id++;
		}
	}

	print $CFGFILE "};\n";
	print $CFGFILE "\n";

	# iterate alarms per partition
	print $CFGFILE "/* alarm table */\n";
	print $CFGFILE "const struct alarm_cfg alarm_cfg[", $num_alarms + $num_schedtabs, "] = {\n";

	$part_cnt = $num_cpus;	# skip idle partitions, no alarms
	for my $part (@{$sys->{partition}}) {
		my $alarm_id = 0;
		my $cpu = $part->{cpu};
		if (!defined $cpu) {
			$cpu = 0;
		}

		for my $alarm (@{$part->{alarm}}) {
			my $c = $alarm->{counter};
			if (!defined $known_counters{$c}) {
				die "counter '" . $c . "' does not exists\n";
			}

			my $action;

			if (defined($alarm->{action_event})) {
				$action = "ALARM_ACTION_EVENT";
			} elsif (defined($alarm->{action_task})) {
				$action = "ALARM_ACTION_TASK";
			} elsif (defined($alarm->{action_hook})) {
				$action = "ALARM_ACTION_HOOK";
			} elsif (defined($alarm->{action_counter})) {
				$action = "ALARM_ACTION_COUNTER";
			} elsif (defined($alarm->{action_invoke})) {
				$action = "ALARM_ACTION_INVOKE";
			} else {
				die "invalid alarm '", $alarm->{name}, "'\n";
			}
			print $CFGFILE "\t/* partition '", $part->{name}, "' index ", $alarm_id, " '", $alarm->{name}, "' */ {\n";
			print $CFGFILE "\t\t.counter_id = ", $known_counters{$c}, ", /* ", $c, " */\n";
			print $CFGFILE "\t\t.cpu_id = ", $cpu, ",\n";
			print $CFGFILE "\t\t.action = ", $action, ",\n";

			if (defined($alarm->{action_event})) {
				my $a = $alarm->{action_event}[0];
				my $n = $a->{partition} . "::" . $a->{task};
				my $e = $a->{bit};

				if (!defined $known_tasks{$n}) {
					die "task '" . $n . "' does not exists\n";
				}

				print $CFGFILE "\t\t.u.task = &task_dyn_part_", $part_cnt, "[", $known_local_tasks{$n}, "], /* partition '", $a->{partition} . "' task '" . $a->{task}, "' */\n";
				print $CFGFILE "\t\t.event_bit = ", $e, ",\n";
			} elsif (defined($alarm->{action_task})) {
				my $a = $alarm->{action_task}[0];
				my $n = $a->{partition} . "::" . $a->{task};

				if (!defined $known_tasks{$n}) {
					die "task '" . $n . "' does not exists\n";
				}

				print $CFGFILE "\t\t.u.task = &task_dyn_part_", $part_cnt, "[", $known_local_tasks{$n}, "], /* partition '", $a->{partition} . "' task '" . $a->{task}, "' */\n";
			} elsif (defined($alarm->{action_hook})) {
				my $a = $alarm->{action_hook}[0];
				my $n = $a->{partition} . "::" . $a->{hook};

				if (!defined $known_hooks{$n}) {
					die "hook '" . $n . "' does not exists\n";
				}

				print $CFGFILE "\t\t.u.task = &task_dyn_part_", $part_cnt, "[", $known_local_hooks{$n}, "], /* partition '", $a->{partition} . "' hook '" . $a->{hook}, "' */\n";
			} elsif (defined($alarm->{action_counter})) {
				my $a = $alarm->{action_counter}[0];
				my $c = $a->{counter};

				if (!defined $known_counters{$c}) {
					die "counter '" . $c . "' does not exists\n";
				}
				print $CFGFILE "\t\t.u.counter_cfg = &counter_cfg[", $known_counters{$c}, "],\n";
			} elsif (defined($alarm->{action_invoke})) {
				my $a = $alarm->{action_invoke}[0];
				my $e = $a->{entry};

				if ($reloc) {
					my $s = sym_eval(\%symhash_kern, $e);
					print $CFGFILE "\t\t.u.alarm_callback = (void*)", hexify($s), ", /* ", $e, " */\n";
				}
			}

			print $CFGFILE "\t},\n";
			$alarm_id++;
		}
		$part_cnt++;
	}

	# add pseudo-alarms for schedule tables (inaccessible to user)
	$part_cnt = $num_cpus;	# skip idle partitions, no schedule tables
	for my $part (@{$sys->{partition}}) {
		my $schedtab_id = 0;
		my $cpu = $part->{cpu};
		if (!defined $cpu) {
			$cpu = 0;
		}

		for my $schedtab (@{$part->{sched_table}}) {
			my $c = $schedtab->{counter};
			if (!defined $known_counters{$c}) {
				die "counter '" . $c . "' does not exists\n";
			}

			print $CFGFILE "\t/* partition '", $part->{name}, "' sched_table '", $schedtab->{name}, "' */ {\n";
			print $CFGFILE "\t\t.counter_id = ", $known_counters{$c}, ", /* ", $c, " */\n";
			print $CFGFILE "\t\t.cpu_id = ", $cpu, ",\n";
			print $CFGFILE "\t\t.action = ALARM_ACTION_SCHEDTAB,\n";
			print $CFGFILE "\t\t.u.schedtab = &schedtab_dyn_part_", $part_cnt, "[", $schedtab_id, "],\n";

			print $CFGFILE "\t},\n";
			$schedtab_id++;
		}

		$part_cnt++;
	}

	print $CFGFILE "};\n";
	$part_cnt = $num_cpus;	# skip idle partitions, no alarms
	for my $part (@{$sys->{partition}}) {
		my $cpu = $known_partition_cpus{$part->{name}};
		my $alarms = $known_partition_alarms{$part->{name}};

		print $CFGFILE "struct alarm alarm_dyn_part_", $part_cnt, "[", $alarms, "] __section_bss_core(", $cpu, ");\n";

		$part_cnt++;
	}
	print $CFGFILE "\n";

	for (my $s = 0; $s < $num_schedtabs; $s++) {
		my $cpu = $known_schedtab_cpus{$s};
		print $CFGFILE "struct alarm alarm_dyn_schedtab_", $s, " __section_bss_core(", $cpu, ");\n";
	}
	print $CFGFILE "\n";


	# iterate schedule tables per partition
	print $CFGFILE "/* sched_table table */\n";
	print $CFGFILE "const uint8_t num_schedtabs __section_cfg = ", $num_schedtabs, ";\n";
	print $CFGFILE "const struct schedtab_cfg schedtab_cfg[", $num_schedtabs, "] = {\n";

	my $sta_id = 0;		# sta == sched table action
	my $global_schedtab_id = 0;
	$part_cnt = $num_cpus;	# skip idle partitions, no schedule table
	for my $part (@{$sys->{partition}}) {
		my $schedtab_id = 0;
		for my $schedtab (@{$part->{sched_table}}) {
			my $c = $schedtab->{counter};
			if (!defined $known_counters{$c}) {
				die "counter '" . $c . "' does not exists\n";
			}

			print $CFGFILE "\t/* partition '", $part->{name}, "' sched_table ", $schedtab_id, " '", $schedtab->{name}, "' */ {\n";
			print $CFGFILE "\t\t.counter_id = ", $known_counters{$c}, ", /* ", $c, " */\n";
			print $CFGFILE "\t\t.flags = 0";
			if (defined $schedtab->{repeating}) {
				my $r = $schedtab->{repeating};
				if ($r eq "yes") {
					print $CFGFILE " | SCHEDTAB_FLAG_REPEATING";
				} elsif ($r eq "no") {
					# nothing
				} else {
					die "schedule table '", $schedtab->{name}, "' wrong repeating attribute, must be 'yes' or 'no'\n";
				}
			}
			if (defined $schedtab->{sync}) {
				my $s = $schedtab->{sync};
				if ($s eq "none") {
					# nothing
				} elsif ($s eq "explicit") {
					print $CFGFILE " | SCHEDTAB_FLAG_SYNC_EXPLICIT";
				} elsif ($s eq "implicit") {
					print $CFGFILE " | SCHEDTAB_FLAG_SYNC_IMPLICIT";
				} else {
					die "schedule table '", $schedtab->{name}, "' wrong sync attribute, must be 'none', 'explicit' or 'implicit'\n";
				}
			}

			print $CFGFILE ",\n";
			print $CFGFILE "\t\t.duration = ", $schedtab->{duration}, ",\n";
			if (defined $schedtab->{precision}) {
				print $CFGFILE "\t\t.precision = ", $schedtab->{precision}, ",\n";
			}
			print $CFGFILE "\t\t.alarm = &alarm_dyn_schedtab_", $global_schedtab_id, ",\n";
			print $CFGFILE "\t\t.start_idx = ", $sta_id, ",\n";

			print $CFGFILE "\t},\n";
			$global_schedtab_id++;
			$schedtab_id++;


			# Iterate expiries (FIXME: must be correctly ordered in XML!)
			my $first_expiry_id = $sta_id;
			my $last_o = 0;
			my $oo;

			# Emit initial START indicator
			$sta_comment{$sta_id} = "schedule table '" . $schedtab->{name} . "'";
			$sta_actions{$sta_id} = "SCHEDTAB_ACTION_START";
			$sta_id++;

			for my $expiry (@{$schedtab->{expiry}}) {
				$sta_comment{$sta_id} = "offset " . $expiry->{offset};

				# Emit SHORTEN / LENGTHEN before the wait sequence
				if (defined $expiry->{max_shorten}) {
					my $shorten = $expiry->{max_shorten};
					if ($shorten > 0) {
						$sta_actions{$sta_id} = "SCHEDTAB_ACTION_SHORTEN";
						$sta_arg4s{$sta_id} = $shorten;
						$sta_id++;
					}
				}
				if (defined $expiry->{max_lengthen}) {
					my $lengthen = $expiry->{max_lengthen};
					if ($lengthen > 0) {
						$sta_actions{$sta_id} = "SCHEDTAB_ACTION_LENGTHEN";
						$sta_arg4s{$sta_id} = $lengthen;
						$sta_id++;
					}
				}


				# Emit a WAIT sequence to synchronize to offset
				$oo = $expiry->{offset} - $last_o;
				if ($oo > 0) {
					$sta_actions{$sta_id} = "SCHEDTAB_ACTION_WAIT";
					$sta_arg4s{$sta_id} = $oo;
					$sta_id++;
				}

				# generate in order: tasks, hooks, events (required by SWS_Os_00412)
				for my $a (@{$expiry->{action_task}}) {
					my $n = $a->{partition} . "::" . $a->{task};

					if (!defined $known_tasks{$n}) {
						die "task '" . $n . "' does not exists\n";
					}

					# emit SCHEDTAB_ACTION_TASK
					$sta_actions{$sta_id} = "SCHEDTAB_ACTION_TASK";
					$sta_arg3s{$sta_id} = "&task_dyn_part_" . $part_cnt . "[" . $known_local_tasks{$n} . "] /* partition '" . $a->{partition} . "' task '" . $a->{task} . "' */";
					$sta_id++;
				}
				for my $a (@{$expiry->{action_hook}}) {
					my $n = $a->{partition} . "::" . $a->{hook};

					if (!defined $known_hooks{$n}) {
						die "hook '" . $n . "' does not exists\n";
					}

					# emit SCHEDTAB_ACTION_HOOK
					$sta_actions{$sta_id} = "SCHEDTAB_ACTION_HOOK";
					$sta_arg3s{$sta_id} = "&task_dyn_part_" . $part_cnt . "[" . $known_local_hooks{$n} . "] /* partition '" . $a->{partition} . "' hook '" . $a->{hook} . "' */";
					$sta_id++;
				}
				for my $a (@{$expiry->{action_event}}) {
					my $n = $a->{partition} . "::" . $a->{task};

					if (!defined $known_tasks{$n}) {
						die "task '" . $n . "' does not exists\n";
					}

					# emit SCHEDTAB_ACTION_EVENT
					$sta_actions{$sta_id} = "SCHEDTAB_ACTION_EVENT";
					$sta_arg1s{$sta_id} = $a->{bit};
					$sta_arg3s{$sta_id} = "&task_dyn_part_" . $part_cnt . "[" . $known_local_tasks{$n} . "] /* partition '" . $a->{partition} . "' task '" . $a->{task} . "' */";
					$sta_id++;
				}

				$last_o = $expiry->{offset};
			}

			$oo = $schedtab->{duration} - $last_o;
			# Emit final WAIT + WRAP sequence
			$sta_comment{$sta_id} = "offset " . $schedtab->{duration};
			if ($oo > 0) {
				$sta_actions{$sta_id} = "SCHEDTAB_ACTION_WAIT";
				$sta_arg4s{$sta_id} = $oo;
				$sta_id++;
			}
			$sta_comment{$sta_id} = "wrap around to " . $first_expiry_id;
			$sta_actions{$sta_id} = "SCHEDTAB_ACTION_WRAP";
			$sta_arg2s{$sta_id} = $first_expiry_id;
			$sta_id++;
		}
		$part_cnt++;
	}

	print $CFGFILE "};\n";
	$part_cnt = $num_cpus;	# skip idle partitions, no alarms
	for my $part (@{$sys->{partition}}) {
		my $cpu = $known_partition_cpus{$part->{name}};
		my $schedtabs = $known_partition_schedtabs{$part->{name}};

		print $CFGFILE "struct schedtab schedtab_dyn_part_", $part_cnt, "[", $schedtabs, "] __section_bss_core(", $cpu, ");\n";

		$part_cnt++;
	}
	print $CFGFILE "\n";



	# iterate schedule tables per partition
	print $CFGFILE "/* schedtab_action table */\n";
	print $CFGFILE "const struct schedtab_action_cfg schedtab_action_cfg[", $sta_id, "] = {\n";
	for (my $i = 0; $i < $sta_id; $i++) {
		print $CFGFILE "\t";
		if (defined $sta_comment{$i}) {
			print $CFGFILE "/* #", $i, ": ", $sta_comment{$i}, " */ ";
		}
		print $CFGFILE "{\n";
		print $CFGFILE "\t\t.action = ", $sta_actions{$i}, ",\n";
		if (defined $sta_arg1s{$i}) {
			# arg1 is only used for events
			print $CFGFILE "\t\t.event_bit = ", $sta_arg1s{$i}, ",\n";
		}
		if (defined $sta_arg2s{$i}) {
			# arg2 is used for optional "next" entries
			print $CFGFILE "\t\t.next_idx = ", $sta_arg2s{$i}, ",\n";
		}
		if (defined $sta_arg3s{$i}) {
			# arg3 is used for optional task references
			print $CFGFILE "\t\t.u.task = ", $sta_arg3s{$i}, ",\n";
		}
		if (defined $sta_arg4s{$i}) {
			# arg4 decodes optional time values
			print $CFGFILE "\t\t.u.time = ", $sta_arg4s{$i}, ",\n";
		}
		print $CFGFILE "\t},\n";
	}
	print $CFGFILE "};\n";
	print $CFGFILE "\n";



	# ISR call table
	print $CFGFILE "/* ISR call table */\n";
	print $CFGFILE "const struct isr_cfg isr_cfg[", $num_isrs, "] = {\n";

	# Iterate all cat 1 ISRs and fill in the interrupt table
	{
		my $kernel = $target->{kernel};
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
		print $CFGFILE "\t/* ISR ", $vector, " */ {\n";
		if (defined $known_isrs_user[$vector]) {
			print $CFGFILE "\t\t.func = kernel_wake_isr_task,\n";
			print $CFGFILE "\t\t.arg0 = &task_cfg[", $known_isrs_user[$vector] , "], /* user ISR */\n";
		} else {
			my $isr_func = 0;
			my $isr_arg0 = 0;
			my $isr_func_name = "NULL";
			my $isr_arg0_name = "NULL";

			# default ISR
			if ($reloc && defined $target->{kernel}->{defaultisr}) {
				my $invoke = $target->{kernel}->{defaultisr}[0];

				$isr_func_name = $invoke->{entry};
				$isr_arg0_name = "vector " . $vector;

				$isr_func = sym_eval(\%symhash_kern, $invoke->{entry});
				$isr_arg0 = $vector;
			}

			if ($reloc && defined $known_isrs_kern[$vector]) {
				my $invoke = $known_isrs_kern[$vector];

				$isr_func_name = $invoke->{entry};
				$isr_arg0_name = $invoke->{arg};

				$isr_func = sym_eval(\%symhash_kern, $invoke->{entry});
				$isr_arg0 = sym_eval(\%symhash_kern, $invoke->{arg});
			}

			print $CFGFILE "\t\t.func = (void*)", hexify($isr_func), ", /* ", $isr_func_name, " */\n";
			print $CFGFILE "\t\t.arg0 = (void*)", hexify($isr_arg0), ", /* ", $isr_arg0_name, " */\n";
		}
		print $CFGFILE "\t},\n";
	}
	print $CFGFILE "};\n";
	print $CFGFILE "\n";

	# generate wait queue configuration
	print $CFGFILE "/* Wait queue table */\n";
	print $CFGFILE "const struct wq_cfg wq_cfg[", $num_waitqueues, "] = {\n";

	# index of wait queues by name
	my %known_waitqueue_ids;
	my %known_waitqueue_partitions;
	$part_cnt = $num_cpus;	# skip idle partitions, no alarms
	for my $part (@{$sys->{partition}}) {
		my $wq_local_array_index = 0;
		my $pn = $part->{name};
		for my $waitqueue (@{$part->{wait_queue}}) {
			my $n = $waitqueue->{name};
			if (defined $known_waitqueue_ids{$pn."::".$n}) {
				die "wait_queue '" . $n . "' already exists in partition '" . $pn . "'\n";
			}
			$known_waitqueue_ids{$part->{name}."::".$n} = $wq_local_array_index;
			$known_waitqueue_partitions{$part->{name}."::".$n} = $part_cnt;
			$wq_local_array_index++;
		}
		$part_cnt++;
	}

	# link wait queues
	my $wq_array_index = 0;
	for my $part (@{$sys->{partition}}) {
		my $pn = $part->{name};
		for my $waitqueue (@{$part->{wait_queue}}) {
			my $n = $waitqueue->{name};

			print $CFGFILE "\t/* ", $wq_array_index, ": wait_queue '", $n, "' partition '", $pn, "' */ {\n";
			if (defined $waitqueue->{link}) {
				my $ln = $waitqueue->{link};
				# partition name defaults to own partition
				my $lpn = $part->{name};
				if (defined $waitqueue->{partition}) {
					$lpn = $waitqueue->{partition};
				}
				if (!defined $known_waitqueue_ids{$lpn."::".$ln}) {
					die "wait_queue '" . $n . "' in partition '" . $pn . "' links to unknown wait queue\n";
				}
				my $lid = $known_waitqueue_ids{$lpn."::".$ln};
				my $pid = $known_waitqueue_partitions{$lpn."::".$ln};
				print $CFGFILE "\t\t.link = &wq_dyn_part_", $pid, "[", $lid, "], /* wait_queue '", $ln, "' partition '", $lpn, "' */\n";
			} else {
				print $CFGFILE "\t\t.link = NULL, /* not connected */\n";
			}

			print $CFGFILE "\t},\n";
			$wq_array_index++;
		}
	}

	print $CFGFILE "};\n";

	$part_cnt = $num_cpus;	# skip idle partitions, no wait queues
	for my $part (@{$sys->{partition}}) {
		my $cpu = $known_partition_cpus{$part->{name}};
		my $wqs = $known_partition_wqs{$part->{name}};

		print $CFGFILE "struct wq wq_dyn_part_", $part_cnt, "[", $wqs, "] __section_bss_core(", $cpu, ");\n";

		$part_cnt++;
	}
	print $CFGFILE "\n";


	$part_cnt = $num_cpus;	# skip idle partitions, no RPC
	for my $part (@{$sys->{partition}}) {
		my $cpu = $known_partition_cpus{$part->{name}};
		my $invokables = $known_partition_invokables{$part->{name}};

		print $CFGFILE "struct rpc rpc_dyn_part_", $part_cnt, "[", $invokables, "] __section_bss_core(", $cpu, ");\n";

		$part_cnt++;
	}
	print $CFGFILE "\n";


	# generate SHM configuration
	print $CFGFILE "/* SHM table */\n";
	print $CFGFILE "/* const struct shm_cfg shm_cfg[] is emitted by MPU generator */\n";
	print $CFGFILE "const struct shm_access shm_access[", $num_shm_accs, "] = {\n";

	my $shm_acc_array_index = 0;
	for my $part (@{$sys->{partition}}) {
		my $pn = $part->{name};
		for my $shm_acc (@{$part->{shm_access}}) {
			my $shm = $shm_acc->{shm};

			print $CFGFILE "\t/* ", $shm_acc_array_index, ": SHM '", $shm, "' partition '", $pn, "' */ {\n";

			if (!defined $known_shms{$shm}) {
				die "shm_access '" . $shm . "' in partition '" . $pn . "' refers to unknown SHM\n";
			}
			print $CFGFILE "\t\t.shm_cfg = &shm_cfg[", $known_shms{$shm}, "],\n";

			print $CFGFILE "\t},\n";
			$shm_acc_array_index++;
		}
	}

	print $CFGFILE "};\n";
	print $CFGFILE "\n";


	# generate RPC configuration
	print $CFGFILE "/* RPC table */\n";
	print $CFGFILE "const struct rpc_cfg rpc_cfg[", $num_rpcs, "] = {\n";

	my $rpc_array_index = 0;
	for my $part (@{$sys->{partition}}) {
		my $pn = $part->{name};
		for my $rpc (@{$part->{rpc}}) {
			my $rn = $rpc->{name};

			my $rpc_part = $pn;
			if (defined $rpc->{partition}) {
				$rpc_part = $rpc->{partition};
			}

			if (!defined $known_partitions{$rpc_part}) {
				die "RPC '" . $rn . "' in partition '" . $pn . "' refers to unknown partition '", $rpc_part, "'\n";
			}
			my $rpc_part_id = $known_partitions{$rpc_part};

			my $rpc_invokable = $rpc->{invokable};
			my $name_tuple = $rpc_part . "::" . $rpc_invokable;
			if (!defined $known_invokables{$name_tuple}) {
				die "RPC '" . $rn . "' in partition '" . $pn . "' refers to unknown target '", $rpc_invokable,"'\n";
			}

			my $prio = 0;
			if (defined $rpc->{prio}) {
				$prio = number $rpc->{prio};
			}

			print $CFGFILE "\t/* ", $rpc_array_index, " partition '", $pn, "' RPC '", $rn, "' ";
			print $CFGFILE "calls partition '", $rpc_part, "' invokable '", $rpc_invokable, " */ {\n";

			print $CFGFILE "\t\t.task = &task_dyn_part_", $rpc_part_id, "[", $known_local_invokables{$name_tuple}, "],\n";

			print $CFGFILE "\t\t.prio = ", $prio, ",\n";

			print $CFGFILE "\t},\n";
			$rpc_array_index++;
		}
	}

	print $CFGFILE "};\n";
	print $CFGFILE "\n";


	# generate IPI configuration
	my $num_actions = number $sys->{ipi_actions};

	print $CFGFILE "/** cross-core IPI jobs (num_actions x (num_cpus-1) x num_cpus) */\n";
	print $CFGFILE "const uint8_t num_ipi_actions __section_cfg = ", $num_actions, ";\n";
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "struct ipi_action ipi_actions_core_", $cpu, "[", $num_actions*($num_cpus-1), "] __section_context_core(", $cpu, ");\n";
	}
	print $CFGFILE "\n";

	print $CFGFILE "/** per-core IPI state */\n";
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "struct ipi_state ipi_state_core_", $cpu, " __section_context_core(", $cpu, ");\n";
	}
	print $CFGFILE "\n";

	print $CFGFILE "/** per-CPU IPI configuration */\n";
	print $CFGFILE "const struct ipi_cfg ipi_cfg[", $num_cpus, "] = {\n";
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		my $core_pos = 0;
		print $CFGFILE "\t/* CPU ", $cpu, " */ {\n";
		print $CFGFILE "\t\t.state = &ipi_state_core_", $cpu, ",\n";
		print $CFGFILE "\t\t.actions = {\n";
		for (my $cpu2 = 0; $cpu2 < $num_cpus; $cpu2++) {
			if ($cpu2 == $cpu) {
				print $CFGFILE "\t\t\tNULL, /* no IPIs to self */\n";
			} else {
				print $CFGFILE "\t\t\t&ipi_actions_core_", $cpu, "[", $core_pos, "],\n";
				$core_pos += $num_actions;
			}
		}
		print $CFGFILE "\t\t},\n";
		print $CFGFILE "\t},\n";
	}
	print $CFGFILE "};\n";


	# generate time partition configuration
	print $CFGFILE "/* Time partition schedule tables */\n";
	my $num_schedules = 0;
	my $use_default_schedule = 0;
	if ($sys->{schedule}) {
		$num_schedules += @{$sys->{schedule}};
	}
	if ($num_schedules == 0) {
		$num_schedules = 1;
		$use_default_schedule = 1;
	}
	print $CFGFILE "const uint8_t num_tpschedules __section_cfg = ", $num_schedules, ";\n";
	print $CFGFILE "const struct tpschedule_cfg tpschedule_cfg[", $num_schedules, "] = {\n";

	my $schedule_cnt = 0;
	my $num_windows = 0;
	if ($use_default_schedule) {
		print $CFGFILE "\t/* default schedule */ {\n";
		print $CFGFILE "\t.start = &tpwindow_cfg[0],\n";
		print $CFGFILE "\t},\n";

		$num_windows = 1;
	} else {
		for my $schedule (@{$sys->{schedule}}) {
			my $name = $schedule->{name};

			if (defined $known_schedules{$name}) {
				die "time partition schedule '" . $name . "' already exists\n";
			}
			$known_schedules{$name} = $schedule_cnt;

			my $num_ws = @{$schedule->{window}};
			if ($num_ws == 0) {
				die "time partition schedule '" . $name . "' has no windows\n";
			}

			print $CFGFILE "\t/* schedule '", $name, "' */ {\n";
			print $CFGFILE "\t.start = &tpwindow_cfg[", $num_windows, "],\n";
			print $CFGFILE "\t},\n";

			$schedule_cnt++;
			$num_windows += $num_ws;
		}
	}
	print $CFGFILE "};\n";
	print $CFGFILE "\n";

	print $CFGFILE "const struct tpwindow_cfg tpwindow_cfg[", $num_windows, "] = {\n";
	if ($use_default_schedule) {
		print $CFGFILE "\t/* default window */ {\n";
		print $CFGFILE "\t\t.timepart = 0,\n";
		print $CFGFILE "\t\t.duration = ", $system_period, ",\n";
		print $CFGFILE "\t\t.flags = TPWINDOW_FLAG_FIRST | TPWINDOW_FLAG_LAST,\n";
		print $CFGFILE "\t},\n";
	} else {
		my $windows_cnt = 0;
		for my $schedule (@{$sys->{schedule}}) {
			my $name = $schedule->{name};
			my $num_ws = @{$schedule->{window}};
			my $start_time = 0;
			my $start_idx = $windows_cnt;
			my $this_window = 0;

			for my $window (@{$schedule->{window}}) {
				my $off = number $window->{offset};
				my $dur = number $window->{duration};
				my $release = 0;
				if (defined $window->{release}) {
					$release = !!(number $window->{release});
				}
				my $tp = number $window->{timepart};

				if ($off < $start_time) {
					die "time partition schedule '" . $name . "' window at offset ", $off, " overlaps previous window\n";
				}
				if ($off > $start_time) {
					die "time partition schedule '" . $name . "' window at offset ", $off, " leaves a hole\n";
				}
				if ($dur == 0) {
					die "time partition schedule '" . $name . "' window at offset ", $off, " has zero duration\n";
				}
				$start_time += $dur;

				print $CFGFILE "\t/* window ", $windows_cnt, ": schedule '", $name, "' offset ", $off, " */ {\n";

				print $CFGFILE "\t\t.timepart = ", $tp, ",\n";
				print $CFGFILE "\t\t.duration = ", $dur, ",\n";

				print $CFGFILE "\t\t.flags = 0";
				if ($this_window == 0) {
					print $CFGFILE " | TPWINDOW_FLAG_FIRST";
				}
				if ($this_window == $num_ws-1) {
					print $CFGFILE " | TPWINDOW_FLAG_LAST";
				}
				if ($release) {
					print $CFGFILE " | TPWINDOW_FLAG_RELEASE";
				}
				print $CFGFILE ",\n";

				print $CFGFILE "\t},\n";
				$windows_cnt++;
				$this_window++;
			}

			if ($start_time != $system_period) {
				die "time partition schedule '" . $name . "' duration " . $start_time . " does not match system period " . $system_period . "\n";
			}
		}
	}
	print $CFGFILE "};\n";
	print $CFGFILE "\n";

	# system HM tables
	print $CFGFILE "/* System HM tables */\n";
	my $use_default_hm_table = 0;
	my $num_hm_tables = 0;
	if ($sys->{hm_table}) {
		$num_hm_tables += @{$sys->{hm_table}};
	}
	if ($num_hm_tables == 0) {
		$num_hm_tables = 1;
		$use_default_hm_table = 1;
	}
	print $CFGFILE "const uint8_t num_hm_tables __section_cfg = ", $num_hm_tables, ";\n";
	print $CFGFILE "const struct hm_table hm_table_system_cfg[", $num_hm_tables, "] = {\n";

	if ($use_default_hm_table) {
		print $CFGFILE "\t/* default system HM table */ { .level_action_error_code = {\n";
		# Fill with default
		for (my $error_cnt = 0; $error_cnt < $NUM_HM_ERROR_IDS; $error_cnt++) {
			print $CFGFILE "\t\tHM_ACTION_DEFAULT_SYSTEM_", $error_cnt, ", /* default */\n";
		}
		print $CFGFILE "\t}},\n";
	} else {
		my $table_cnt = 0;
		for my $table (@{$sys->{hm_table}}) {
			my $name = $table->{name};
			print $CFGFILE "\t/* system HM table ", $table_cnt, " '", $name, "' */ { .level_action_error_code = {\n";

			my $error_cnt = 0;
			for my $error (@{$table->{error}}) {
				my $id = number $error->{id};
				my $level = $error->{level};
				my $action = $error->{action};
				my $code = $error->{error_code};

				# Fill gaps with default
				for (; $error_cnt < $id; $error_cnt++) {
					print $CFGFILE "\t\tHM_ACTION_DEFAULT_SYSTEM_", $error_cnt, ", /* default */\n";
				}

				print $CFGFILE "\t\tHM_ACTION_", $level, "_", $action;
				if (defined $code) {
					print $CFGFILE " | ", $code;
				}
				print $CFGFILE ",\n";
				$error_cnt++;
			}

			# Fill remaining gaps with default
			for (; $error_cnt < $NUM_HM_ERROR_IDS; $error_cnt++) {
				print $CFGFILE "\t\tHM_ACTION_DEFAULT_SYSTEM_", $error_cnt, ", /* default */\n";
			}

			print $CFGFILE "\t}},\n";
			$table_cnt++;
		}
	}
	print $CFGFILE "};\n";
	print $CFGFILE "\n";

	# partition HM tables
	print $CFGFILE "/* Partition HM tables */\n";
	print $CFGFILE "const struct hm_table hm_table_part_cfg[", $num_parts, "] = {\n";

	my $table_cnt = 0;
	for (my $cpu = 0; $cpu < $num_cpus; $cpu++) {
		print $CFGFILE "\t/* partition HM table ", $table_cnt, " for idle partition */ { .level_action_error_code = {\n";
		for (my $error_cnt = 0; $error_cnt < $NUM_HM_ERROR_IDS; $error_cnt++) {
			print $CFGFILE "\t\tHM_ACTION_DEFAULT_PARTITION_", $error_cnt, ", /* default */\n";
		}
		print $CFGFILE "\t}},\n";
		$table_cnt++;
	}
	for my $part (@{$sys->{partition}}) {
		my $name = $part->{name};

		print $CFGFILE "\t/* partition HM table ", $table_cnt, " for partition '", $name, "' */ { .level_action_error_code = {\n";

		my $table = $part->{hm_table}[0];
		my $error_cnt = 0;
		if (defined $table) {
			for my $error (@{$table->{error}}) {
				my $id = number $error->{id};
				my $level = $error->{level};
				my $action = $error->{action};
				my $code = $error->{error_code};

				# Fill gaps with default
				for (; $error_cnt < $id; $error_cnt++) {
					print $CFGFILE "\t\tHM_ACTION_DEFAULT_PARTITION_", $error_cnt, ", /* default */\n";
				}

				print $CFGFILE "\t\tHM_ACTION_", $level, "_", $action;
				if (defined $code) {
					print $CFGFILE " | ", $code;
				}
				print $CFGFILE ",\n";
				$error_cnt++;
			}
		}

		# Fill remaining gaps with default
		for (; $error_cnt < $NUM_HM_ERROR_IDS; $error_cnt++) {
			print $CFGFILE "\t\tHM_ACTION_DEFAULT_PARTITION_", $error_cnt, ", /* default */\n";
		}

		print $CFGFILE "\t}},\n";
		$table_cnt++;
	}
	print $CFGFILE "};\n";
	print $CFGFILE "\n";

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
	print "  ab_gen_config_c.pl [-h|--help] [--version]\n";
	print "                     [-o <config.c>]\n";
	print "                     [-p <appdir>]\n";
	print "                     [-r]\n";
	print "                     <system.xml>\n";
	print "\n";
	print "options:\n";
	print "  -h|--help     print this help text and exit\n";
	print "  --version     print version information and exit\n";
	print "  -o <config.c> config.c to create with system configuration\n";
	print "  -p <appdir>   path to application\n";
	print "  -r            read ELF symbols of relocated binaries\n";
	print "  <system.xml>  system description\n";

	exit $ret;
}

my $cfgfile = 0;
my $reloc = 0;
my $xmlfile;

while (defined $ARGV[0]) {
	if ($ARGV[0] eq '--help') {
		usage(0);
	} elsif ($ARGV[0] eq '-h') {
		usage(0);
	} elsif ($ARGV[0] eq '--version') {
		print "version: ", $VERSION, "\n";
		exit 0;
	} elsif ($ARGV[0] eq '-r') {
		shift;
		$reloc = 1;
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
	die "error: no xml-file specified\n";
}

gen_config($xmlfile, $reloc, $cfgfile);
exit 0;
