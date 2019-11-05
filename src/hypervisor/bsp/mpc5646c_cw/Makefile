# Makefile
#

MODS = start board board_halt cache serial test_support string runtime
OBJS = $(addprefix src/,$(addsuffix .o,$(MODS))) $(TESTOBJS) $(SIZEOBJS)

INCLUDES := -Iinclude 

ifneq ($(OUTDIR),)
OUTFILE = $(OUTDIR)/$(notdir $(OUTDIR)).elf
RESULTSFILE = $(OUTDIR)/$(notdir $(OUTDIR))_results.txt
SIZEFILE = $(OUTDIR)/$(notdir $(OUTDIR))_sizes.txt
INCLUDES += -I$(OUTDIR) -I$(OUTDIR)/..
INCLUDES += -I../../include
else
SIZEOBJS = test.o
OUTFILE = test.elf
RESULTSFILE = test_results.txt
SIZEFILE = test_sizes.txt
endif

include bsp_defs.mk

.PHONY: all clean debug autorun getinfo

all: $(OUTFILE)

$(OUTFILE): $(OBJS)
	#$(LD) -nostdlib -vle_bl_opt -vle_enhance_merging -romaddr 0 -rambuffer 0 -srec -g -w on -o $@ -map -lcf mwlink.lcf $^ $(LIBS)
	$(LD)  -Tkernel.ld -o $@ $(OBJS) $(LIBS)
	#$(LD) -nostdlib -w on -o $@ -lcf mwlink.lcf $^ $(LIBS)
	$(OBJDUMP) -h $(SIZEOBJS) | ./printsize.pl > $(SIZEFILE)
	cat $(SIZEFILE)

# debugging
debug: $(OUTFILE)
	# Debug session using Trace32
	$(T32CMD) -s mpc5646c.cmm $<

autorun: $(OUTFILE)
	$(T32CMD) -s mpc5646c_autorun.cmm $<
	mv dump.out $(RESULTSFILE)
	cat $(RESULTSFILE)

# cleanup
clean:
	rm -f $(OBJS) $(OUTFILE)

getinfo:
	@echo $(notdir $(BSP)) $(CC) $(filter-out -I%,$(CFLAGS))
