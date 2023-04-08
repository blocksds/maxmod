# SPDX-License-Identifier: Zlib
#
# Copyright (c) 2023 Antonio Niño Díaz

MAXMOD_MAJOR	:= 1
MAXMOD_MINOR	:= 0
MAXMOD_PATCH	:= 12

VERSION		:= $(MAXMOD_MAJOR).$(MAXMOD_MINOR).$(MAXMOD_PATCH)

# Tools
# -----

CP		:= cp
INSTALL		:= install
MAKE		:= make
RM		:= rm -rf

# Verbose flag
# ------------

ifeq ($(VERBOSE),1)
V		:=
else
V		:= @
endif

# Targets
# -------

.PHONY: all clean ds7 ds9 ds9e gba install

all: gba ds7 ds9 ds9e

clean:
	@echo "  CLEAN"
	@rm -rf lib build

ds: ds7 ds9

gba:
	@+$(MAKE) SYSTEM=GBA -f Makefile.plat --no-print-directory

ds7:
	@+$(MAKE) SYSTEM=DS7 -f Makefile.plat --no-print-directory

ds9:
	@+$(MAKE) SYSTEM=DS9 -f Makefile.plat --no-print-directory

ds9e:
	@+$(MAKE) SYSTEM=DS9E -f Makefile.plat --no-print-directory

INSTALLDIR	?= /opt/blocksds/core/libs/maxmod
INSTALLDIR_ABS	:= $(abspath $(INSTALLDIR))

install: all
	@echo "  INSTALL $(INSTALLDIR_ABS)"
	@test $(INSTALLDIR_ABS)
	$(V)$(RM) $(INSTALLDIR_ABS)
	$(V)$(INSTALL) -d $(INSTALLDIR_ABS)
	$(V)$(CP) -r include lib LICENSE.txt $(INSTALLDIR_ABS)
