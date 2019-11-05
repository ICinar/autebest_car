/*
 * mkelf -- append an ELF header to a binary -- dirty!!!!
 *
 * azuepke, 2013-03-26: initial
 * azuepke, 2013-04-03: added architecture selector and endianess handling
 * azuepke, 2014-04-28: more endianesses
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "endian.h"
#include "elf.h"

/* NOTE: update version ID when doing changes! */
#define BUILDID  "2014-12-23"
#define PROGNAME "mkelf"

const struct {
	char *name;
	int big_endian;
	int elf_machine;
} arch_types[] = {
	{ "i386",   0,  3 },
	{ "arm",    0, 40 },
	{ "armle",  0, 40 },
	{ "armbe",  1, 40 },
	{ "sparc",  1,  2 },
	{ "ppc",    1, 20 },
	{ "ppcle",  0, 20 },
	{ "ppcbe",  1, 20 },
	{ "mips",   1,  8 },
	{ "mipsle", 0,  8 },
	{ "mipsbe", 1,  8 },
	{ "tricore",0, 44 },
	/* generic architectures */
	{ "big",    1,  0 },
	{ "be",     1,  0 },
	{ "little", 0,  0 },
	{ "le",     0,  0 },
};

#define NUM_ARCH_TYPES (sizeof(arch_types) / sizeof(arch_types[0]))

static void version(void)
{
	printf(PROGNAME " " BUILDID "\n");
}

static void usage(FILE *f)
{
	unsigned int i;
	fprintf(f,
	        "Usage: " PROGNAME " {--version | <in.bin> <arch> <loadaddr> <out.elf> [<entry>]}\n"
	        "Add an ELF header to a binary file.\n"
	        "  in.bin        specifies a binary boot image\n"
	        "  arch          target architecture, one of: "
	        );

	/* dump supported architectures */
	for (i = 0; i < NUM_ARCH_TYPES; i++) {
		fprintf(f, "%s%s", i > 0 ? ", " : "", arch_types[i].name);
	}

	fprintf(f, "\n"
	        "  loadaddr      load address of the ELF file in hex\n"
	        "  entry         optional program's entry point in hex, defaults to loadaddr\n"
	        "  out.elf       the ELF file to create\n"
	        "  --version     print version info\n"
	        "  --help        show usage\n"
	        );
}

int main(int argc, char *argv[])
{
	unsigned int loadaddr;
	unsigned int entry;
	unsigned int size;

	struct fake_header {
		struct elf32_header h;
		struct elf32_phdr p;
	} *e;
	char buf[0x1000];
	FILE *in, *out;
	size_t err;
	unsigned int i;
	int is_big_endian;
	int e_machine;

	if (argc >= 2) {
		if (!strcmp(argv[1], "--version")) {
			version();
			return 0;
		}
		if (!strcmp(argv[1], "--help")) {
			usage(stdout);
			return 0;
		}
	}

	if (argc != 5 && argc != 6) {
		usage(stderr);
		return 1;
	}

	for (i = 0; i < NUM_ARCH_TYPES; i++) {
		if (!strcmp(argv[2], arch_types[i].name)) {
			is_big_endian = arch_types[i].big_endian;
			e_machine = arch_types[i].elf_machine;
			break;
		}
	}
	if (i == NUM_ARCH_TYPES) {
		fprintf(stderr, "unknown architecture\n");
		return 2;
	}

	in = fopen(argv[1], "rb");
	if (!in) {
		fprintf(stderr, "could not open source file %s\n", argv[1]);
		return 3;
	}
	/* get file size */
	fseek(in, 0, SEEK_END);
	size = ftell(in);
	fseek(in, 0, SEEK_SET);

	loadaddr = strtoul(argv[3], NULL, 0);
	entry = loadaddr;
	if (argc == 6) {
		entry = strtoul(argv[5], NULL, 0);
	}

	out = fopen(argv[4], "w+b");
	if (!out) {
		fprintf(stderr, "could not open target file %s\n", argv[4]);
		return 3;
	}

	memset(buf, 0, sizeof(buf));
	e = (struct fake_header *)buf;
	e->h.e_ident[0] = 0x7f;
	e->h.e_ident[1] = 'E';
	e->h.e_ident[2] = 'L';
	e->h.e_ident[3] = 'F';
	e->h.e_ident[4] = 1;								/* 32-bit */
	e->h.e_ident[5] = is_big_endian ? 2 : 1;			/* endianess */
	e->h.e_ident[6] = 1;								/* embedded application */
	e->h.e_type = __htox16(2, is_big_endian);		/* executable file */
	e->h.e_machine = __htox16(e_machine, is_big_endian);
	e->h.e_version = __htox32(1, is_big_endian);	/* current version */
	e->h.e_entry = __htox32(entry, is_big_endian);
	e->h.e_phoff = __htox32(0x34, is_big_endian);

	e->h.e_shoff = __htox32(0x54, is_big_endian);
	e->h.e_flags = __htox32(0, is_big_endian);
	e->h.e_ehsize = __htox16(0x34, is_big_endian);
	e->h.e_phentsize = __htox16(0x20, is_big_endian);
	e->h.e_phnum = __htox16(1, is_big_endian);
	e->h.e_shentsize = __htox16(0x28, is_big_endian);
	e->h.e_shnum = __htox16(1, is_big_endian);
	e->h.e_shtrndx = __htox32(0, is_big_endian);

	e->p.p_type = __htox32(PT_LOAD, is_big_endian);
	e->p.p_offset = __htox32(0x1000, is_big_endian);
	e->p.p_vaddr = __htox32(loadaddr, is_big_endian);
	e->p.p_paddr = __htox32(loadaddr, is_big_endian);
	e->p.p_filesz = __htox32(size, is_big_endian);
	e->p.p_memsz = __htox32(size, is_big_endian);
	e->p.p_flags = __htox32(PF_R | PF_W | PF_X, is_big_endian);
	e->p.p_align = __htox32(0, is_big_endian);


	fwrite(buf, 1, sizeof(buf), out);

	do {
		err = fread(buf, 1, sizeof(buf), in);
		if (err == 0)
			break;
		fwrite(buf, 1, err, out);
	} while (1);

	fclose(in);
	fclose(out);

	return 0;
}
