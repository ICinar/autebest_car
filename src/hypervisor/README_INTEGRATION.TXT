# README_INTEGRATION.TXT
#
# azuepke,  2014-08-07: initial
# mbommert, 2014-08-08: continued
# azuepke,  2014-08-18: workflow + XML spec of memory map
# azuepke,  2015-08-05: added "cached" attribute + updated workflow

System Integration
===================

The system integrator has the role to configure a whole ECU comprising multiple
partitions and other software components. From a high level point of view, the
first thing the integrator does is setting up the partitioning.

We assume we have an abstract description of the ECU hardware, and feed the
system configuration with the partitions and a rough idea of the partition's
requirements to memory, I/O and timing.

Based upon this information, an MPU generator tool generates a memory map
for the system, which results in a proper MPU configuration and linker
defines for the applications.


Workflow System Integration
============================

This describes the overall workflow of system integration: Each step defines
a specific tool or generated artifact(s). The tool in step 0 is a helper
to generate an initial dummy configuration for existing partitions.


                    0. TOOL: dummy compile run to generate initial data
                       - for our internal build tooling only !!!
                       - compile all aplications
                       - compile dummy kernel
                       - extract memory requirements from binaries

                                                 \
                                                   \
                                                     v

1. ARTIFACT: abstract HW desc.              2. ARTIFACT: memory requirements
                                               - partitions (RAM, ROM)
                |                              - stacks                  <--+
                |                              - shared memories            |
                |                              - I/O resources              |
                |                                                           |
                |                            /                              |
                |                          /                                |
                v                        v                                  |
                                                                            |
3. TOOL: generate memory map for the overall system                       rinse
   - allocate and assign all memory requirements                            &
   - fulfill $ARCH-specific rules                                        repeat
   - shuffle elements if necessary                                          |
                                                                            |
                |                                                           |
                |                                                           |
                v                                                           |
                                                                            |
4. ARTIFACT: final memory map                                               |
   - all memory requirements have fixed addresses          -----------------+
   - resulting MPU windows layout

                |                    \
                |                      \
                v                        v

5. TOOL: memory map converter:              12. TOOL: linker script generator
   - checks if memory map is valid              - generates linker defines
   - generates $ARCH-specific MPU cfg

                |                                      |
                |                                      |
                v                                      v

6. ARTIFACT: MPU config (XML)               13. ARTIFACT: linker defines

                |                                          \
                |                                            \
                v                                              v
                                                               [...]
7. TOOL: config generator
   - translates XML -> .c/.h

                |
                |
                v

8. ARTIFACT: MPU config (.c Code)
    - compiled into the final kernel

                |
                |                                              [...]
              [...]                                            /
                |                                            /
                v                                          v

9. ARTIFACT: final kernel binary            14. ARTIFACT: final app binaries

                |                        /
                |                      /
                v                    v

10. TOOL: ROM image generator
    - assembles binary ROM image

                |
                |
                v

11. ARTIFACT: final ROM image binary



Abstract Hardware Description
==============================

The abstract hardware description is an XML file describing the hardware on
the SoC of the ECU and memory resources in terms of RAM and flash memories.

Memory resources can be "fixed" resources like the I/O address region of a
CAN-controller, or "pools" of SRAM and flash memory where a partition allocates
pieces from.

(NOTE: The following specification doesn't use XML structure, but a simple
table to describe an example.)

	Type    Name				Start		End			RWX		Description
	fix		CAN1				0xfffc1000	0xfffc10ff	rw-
	fix		CAN2				0xfffc1100	0xfffc11ff	rw-
	fix		FlexRay1			0xfffc1400	0xfffc17ff	rw-
	fix		CAN3				0xfffc1800	0xfffc18ff	rw-
	fix		CAN4				0xfffc1900	0xfffc19ff	rw-
	fix		CANs_and_FlexRay	0xfffc1000	0xfffc1fff	rw-		all CAN + FR

Note, that in the example above, the last entry "CANs_and_FlexRay" covers all
CAN controllers and the FlexRay unit. Fixed entries can overlap or have
adjacent addresses, a configuration tool should cope with that.

Similarly, pool resources like RAM and Flash can be defined:

	Type    Name				Start		End			RWX		Description
	pool	FLASH1				0x00000000	0x0007ffff	r-x		512K Flash Bank1
	pool	FLASH2				0x00080000	0x001fffff	r-x		1.5M Flash Bank2
	pool	SRAM				0x08000000	0x0003ffff	rw-		512K SRAM
	pool	SDRAM				0x80000000	0x007fffff	rw-		8M SDRAM

The distinction between pools and fixed resources is important, as the
integrator *allocates* pieces of resources from pool (by given size and
alignment), but *assigns* complete fixed resources "as a whole" to a partition.


Specification of "hardware.xml":

	<hardware mpu_arch="arm_cortexr4_12regions">
		<fix  name="..." start="..." size="..."
		      read="1" write="1" exec="0" cached="0"
		      description="My assignable I/O controller memory"/>

		<pool name="..." start="..." size="..."
		      read="1" write="1" exec="0" cached="1"
		      description="My allocatable ROM/RAM memory block"/>
		...
	</hardware>

Attribute default values:
	- read: 1
	- write: 0
	- exec: 0
	- cached: 0
	- description: empty

Constraints:
	- name, start and size are mandatory attributes
	- names must be unique
	- size != 0 for all entries
	- entries of type pool must not overlap
	- entries of type fix may overlap with each other
	- entries of type fix must not overlap with entries of type pool

The architecture attribute "mpu_arch" defines the actual MPU type.
The cache attribute selects between either cached or uncached memory only.
This should be enough for now.


Partition Definition -- Memory Requirements
============================================

The system integrator usually knows some memory requirements of a partition,
such a the expected size or estimated SRAM requirements, so he would usually
start by defining a partition with the following two attributes:

	RAM size, say 24K
	Code size, say 80K

These guessed values are typically upper bounds for a partition, while the
really required values are slightly lower. In the integration process, these
definitions will be refined and fine-tuned in a repeated tooling process.

These attributes are matched to pool resources defined in the abstract
hardware description. Also, the integrator will assign I/O devices or further
memory resources (e.g. for stack) to the partitions.

Additionally, the integrator defines data segments shared between partitions.
These can consist of initialized read-only data, like tables defining
motor characteristics, or they could be shared memory regions which are used
to implement any producer/consumer pattern or even bidirectional communication
protocols.

In short, the whole process defines a set of *memory requirements* for the
partitions. Based upon the abstract hardware description and the partition
definition, an *MPU generator tool* assigns fixed resources and allocates
memory from pool resources to generate an according *memory map*.

This memory map is considered "final" as it defines the resulting memory layout
and MPU windows, "correct" in the sense that it obeys MPU architecture
constraints, and "abstract" as its values are input for further tools, e.g.
to generate linker defines and MPU configuration register settings.


The specification of the memory requirements is:
(NOTE: "requirement" is abbreviated as "rq", and "shm" refers to shared memory)

	<memory_layout>
		<!-- assignment of "fix" resource to a shared memory -->
		<shm name="..."
		     resource="name_of_shm_or_fix_from_hardware.xml"
		     read="1" write="0" exec="0" cached="1"
		     description="optional description ..."/>

		<!-- allocation from "pool" resource to a shared memory -->
		<shm name="..."
		     resource="name_of_pool_from_hardware.xml"
		     minsize="0x100" align="..."
		     read="1" write="1" exec="0" cached="1"
		     description="optional description ..."/>
		...

		<part name="__KERNEL__">
			<!-- assignment of "shm" resource -->
			<rq name="..."
			    resource="name_of_shm"
			    read="1" write="1" exec="0" cached="1"
			    description="optional description ..."/>

			<!-- assignment of "fix" resource -->
			<rq name="..."
			    resource="name_of_fix_from_hardware.xml"
			    read="1" write="0" exec="0" cached="0"
			    description="optional description ..."/>

			<!-- allocation from "pool" resource -->
			<rq name="..."
			    resource="name_of_pool_from_hardware.xml"
			    minsize="..." align="..."
			    read="1" write="1" exec="0" cached="1"
		        linkname=".text"
			    description="optional description ..."/>
			...

			<task name="taskname">
				<!-- task specific memory requirement, same format as above -->
				<rq .../>
			</task>
		</part>
		<part name="a_partition_name">
			...
		</part>
		...
	</memory_layout>

Attribute default values:
	- part.type: "user"
	- {shm|rq}.read: [according value from resource in hardware.xml]
	- {shm|rq}.write: [according value from resource in hardware.xml]
	- {shm|rq}.exec: [according value from resource in hardware.xml]
	- {shm|rq}.cached: [according value from resource in hardware.xml]
	- {shm|rq}.align: 1
	- {shm|rq}.description: empty
	- rq.linkname: empty (linker section is omitted)

Constraints:
	- part.name is mandatory, unique, and refers to an existing partition name
	- by convention, the first partition is special and has name "__KERNEL__"
	- task.name is mandatory, unique, and refers to an existing task in its
	  partition
	- shm.name is a mandatory name and must be unique among other shared memory
	  names and the resource names defined in hardware.xml
	- rq.name is mandatory and unique in its respective partition
	- {shm|rq}.resource are mandatory attributes
	- shm.resource must refer to a valid entry in hardware.xml
	- rq.resource must refer to a SHM name or a valid entry in hardware.xml
	- {shm|rq}.minsize is required if rd.resource refers to a pool resource
	- {shm|rq}.minsize > 0
	- {shm|rq}.align must be a power of two (and > 0)
	- rq.linkname must be a valid linker section name


Memory Map and MPU Windows
===========================

Based upon the abstract hardware description and the partition definition
shown above, the MPU generator tool will generate a memory map from the memory
requirements defined by the system integrator.

The MPU generator tool will perform two steps:
1) Allocate memory from pool resources for shared memories and partition
2) Generate an abstract list of MPU windows for each partition

Basically, the first step reduces the "pool allocation" problem to a
"resource assignment" problem, which will be consumed in the second step.
But unlike pure assignment of I/O resources, the pool allocation process
has some degrees of freedom, e.g. it can increase the size of an allocation
to better fit MPU window alignments, hence the attribute name "minsize"
in the memory requirements above.

So, the expected result of this tooling step are basicalliy two things:

1) All memory requirements are complemented with base addresses and sizes
   of their referenced resources:
  - for each requirement, "start" and "size" attributes will be added in XML
  - for pool allocation, the actually allocated piece of the resource is given
  - may have different (fewer) access rights than the original resource
  - these will be used during compile time to determine the layout of the
    binary file (linker script generation)

2) An abstract set of MPU windows is created, comprising:
  - a set of MPU windows for each partition and for each task
  - base adresses and sizes plus access permissions
  - (relative) order of MPU windows
  - these will be used during runtime by the kernel to program the MPU
    (probably after passing some "switching code generation" stage first)


The resulting extension which adds the global attribute "mpu_arch"
and two attributes "start" and "size"
to the existing memory requirement XML structure is:

	<memory_layout mpu_arch="arm_cortexr4_12regions">
		<shm start="start_addr" size="real_size" .../>
		<part ...>
			<rq start="..." size="real_size" .../>
		</part>
	</memory_layout>


Attribute default values:
	- {shm|rq}.minsize: [according value from resource in hardware.xml]

Constraints:
	- memory_layout.mpu_arch: must match the actual MPU type in hardware.xml
	- {shm|rq}.start: [according value from resource in hardware.xml]
	- {shm|rq}.size: [according value from resource in hardware.xml]
	- allocations from a pool resource must not overlap


Additionally, the abstract specification of the MPU window is:

	<memory_layout ...>
		<part ...>
			<mpu_window id="0"
			     start="0x1000" size="0x600"
			     read="1" write="0" exec="1" cached="1"
			     arch="0x42"/>

			<task name="taskname">
				<!-- task specific MPU window, same format as above -->
				<mpu_window .../>
			</task>
		</part>
	</memory_layout>


Attribute default values:
	none

Constraints:
	- mpu_window.id must be unique in its level (partition or task)
	- mpu_window.{start|size} must follow the MPU's alignment requirements
	- mpu_window.arch is an architecture specific MPU attribute


################################################################################

How to use the new tools (in this order):

FIXME: Build libs etc before:
$ make

Generate config.xml once:
$ make config.xml

Create a dummy hardware.xml:
$ perl scripts/ab_gen_hardware_xml.pl -o hardware.xml config.xml

Create a dummy memory.xml based on real application sizes:
$ perl scripts/ab_gen_memory_xml.pl -o memory.xml config.xml

Generate dummy .c-configuration for the kernel
$ perl scripts/ab_gen_config_c.pl config.xml -o dummy_config.c

Assign memory to partitions, creates final_memory.xml:
$ perl scripts/ab_gen_memory_map.pl -hw hardware.xml memory.xml -o final_memory.xml

Create linker defines for second (final) relocation:
$ perl scripts/ab_gen_lddefines.pl -m final_memory.xml config.xml

Generate final .c-configuration for the kernel
$ perl scripts/ab_gen_config_c.pl config.xml -r -o final_config.c

Create binary ROM image of all relocated binaries:
$ perl scripts/ab_gen_romimage.pl -m final_memory.xml config.xml -o bootfile.bin
