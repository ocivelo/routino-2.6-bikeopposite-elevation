# tagmodifier Makefile
#
# Part of the Routino routing software.
#
# This file Copyright 2013 Andrew M. Bishop
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# All configuration is in the top-level Makefile.conf

include ../../Makefile.conf

# Compilation targets

C=$(wildcard *.c)
D=$(wildcard .deps/*.d)

ROUTINO=../../src

EXE=tagmodifier

########

all: all-routino $(EXE)
	@true

all-routino:
	cd $(ROUTINO) && $(MAKE) all

########

TAGMODIFIER_OBJ=tagmodifier.o \
	        $(ROUTINO)/files.o $(ROUTINO)/logging.o $(ROUTINO)/logerror.o \
	        $(ROUTINO)/uncompress.o $(ROUTINO)/xmlparse.o $(ROUTINO)/tagging.o

tagmodifier : $(TAGMODIFIER_OBJ)
	$(LD) $(TAGMODIFIER_OBJ) -o $@ $(LDFLAGS)

########

$(ROUTINO)/%.o :
	cd $(ROUTINO) && $(MAKE) $(notdir $@)

%.o : %.c
	@[ -d .deps ] || mkdir .deps
	$(CC) -c $(CFLAGS) -DSLIM=0 -I$(ROUTINO) $< -o $@ -MMD -MP -MF $(addprefix .deps/,$(addsuffix .d,$(basename $@)))

########

test: all
	@true

########

install: all
	@true

########

clean:
	rm -f *~
	rm -f *.o
	rm -f core

########

distclean: clean
	-rm -f $(EXE)
	-rm -f $(D)
	-rm -fr .deps

########

include $(D)

########

.PHONY:: all all-routino test install clean distclean
