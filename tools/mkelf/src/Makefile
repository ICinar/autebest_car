# Makefile for mkelf
#
# azuepke, 2014-01-31: initial

HOSTCC      := gcc -m32
HOST_CFLAGS := -W -Wall -Wextra
STRIP       := strip --strip-all
TARGET      ?= windows
OUTPUT =
Q           ?= @

ifeq ($(TARGET),windows)
    OUTPUT = ../bin/mkelf.win32.exe
else ifeq ($(TARGET),linux)
    OUTPUT = ../bin/mkelf.linux
else
    $(error Compilation target '$(TARGET)' not supported)
endif

.PHONY: all clean distclean

all: $(OUTPUT)

$(OUTPUT): mkelf.c elf.h endian.h
	@echo Bulding $(OUTPUT)...
	$(Q)$(HOSTCC) $(HOST_CFLAGS) -o $@ $<
	@echo Stripping $(OUTPUT)...
	$(Q)$(STRIP) $(OUTPUT)

clean:
	rm -f $(OUTPUT)

distclean: clean

