# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

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

.PHONY: all clean docs ds7 ds9 gba install

all: gba ds7 ds9 ds

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

ds: ds7 ds9

INSTALLDIR	?= /opt/blocksds/core/libs/maxmod
INSTALLDIR_ABS	:= $(abspath $(INSTALLDIR))

install: ds
	@echo "  INSTALL $(INSTALLDIR_ABS)"
	@test $(INSTALLDIR_ABS)
	$(V)$(RM) $(INSTALLDIR_ABS)
	$(V)$(INSTALL) -d $(INSTALLDIR_ABS)
	$(V)$(CP) -r include lib COPYING $(INSTALLDIR_ABS)

docs:
	@echo "  DOXYGEN"
	doxygen Doxyfile
