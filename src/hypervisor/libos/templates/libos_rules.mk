#
# libos_rules.mk
#
# example rules to build configuration for the OSEK library
# include this file in your application makefile.
# rules expect that:
#  OSEKPARTITION is set to the name of the OSEK partition to generate code for
#  LIBOSDIR is set to the directory name of the OSEK library
#
# will output files into "osek" directory; link against osek/libos_cfg.o

ifeq ("$(OSEKPARTITION)","")
$(error set OSEKPARTITION to the name of the OSEK partition to generate code for)
endif
ifeq ("$(wildcard $(LIBOSDIR)/templates)","")
$(error set LIBOSDIR to the directory name of the OSEK library)
endif

osek:
	@echo "  MKDIR $@"
	$(Q)mkdir -p $@

osek/Os_Cfg.h: $(LIBOSDIR)/templates/Os_Cfg.h.tt gen_config.xml | osek
	@echo "  GEN   $@"
	$(Q)$(ECCG) -p OSEKPartition:$(OSEKPARTITION) -c gen_config.xml -t $< -o $(dir $@)

osek/libos_cfg.c: $(LIBOSDIR)/templates/libos_cfg.c.tt gen_config.xml | osek
	@echo "  GEN   $@"
	$(Q)$(ECCG) -p OSEKPartition:$(OSEKPARTITION) -c gen_config.xml -t $< -o $(dir $@)

osek/libos_cfg.o: osek/libos_cfg.c osek/Os_Cfg.h
	@echo "  CC    $<"
	$(Q)$(CC) $(CFLAGS) -o $@ $<

$(OBJS) $(DEPS): osek/Os_Cfg.h
