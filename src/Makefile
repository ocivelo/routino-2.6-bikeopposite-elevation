# Source code Makefile
#
# Part of the Routino routing software.
#
# This file Copyright 2008-2013 Andrew M. Bishop
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

include ../Makefile.conf

# Web file paths

WEBBINDIR=../$(webbindir)

# Debugging options

#CFLAGS+=-O0 -g
#CFLAGS+=-pg
#CFLAGS+=-fprofile-arcs -ftest-coverage

#LDFLAGS+=-pg -static
#LDFLAGS+=-fprofile-arcs -ftest-coverage

# Sub-directories and sub-makefiles

SUBFILES=$(wildcard */Makefile)
SUBDIRS=$(foreach f,$(SUBFILES),$(dir $f))

# Compilation targets

C=$(wildcard *.c)
D=$(wildcard .deps/*.d)

EXE=planetsplitter planetsplitter-slim router router-slim filedumperx filedumper filedumper-slim

########

all: all-local
	for dir in $(SUBDIRS); do \
	   ( cd $$dir && $(MAKE) $@ ); \
	done

all-local: $(EXE)
	-@[ -d $(WEBBINDIR) ] && \
	  for file in $(EXE); do \
	     if [ ! -f $(WEBBINDIR)/$$file ] || [ $$file -nt $(WEBBINDIR)/$$file ]; then \
	        echo cp $$file $(WEBBINDIR) ;\
	        cp -f $$file $(WEBBINDIR) ;\
	     fi ;\
	  done

########

PLANETSPLITTER_OBJ=planetsplitter.o \
	           nodesx.o segmentsx.o waysx.o relationsx.o superx.o prunex.o \
	           ways.o types.o \
	           files.o logging.o logerror.o errorlogx.o \
	           results.o queue.o sorting.o \
	           xmlparse.o tagging.o \
	           uncompress.o osmxmlparse.o osmpbfparse.o osmo5mparse.o osmparser.o

planetsplitter : $(PLANETSPLITTER_OBJ)
	$(LD) $(PLANETSPLITTER_OBJ) -o $@ $(LDFLAGS)

########

PLANETSPLITTER_SLIM_OBJ=planetsplitter-slim.o \
	                nodesx-slim.o segmentsx-slim.o waysx-slim.o relationsx-slim.o superx-slim.o prunex-slim.o \
	                ways.o types.o \
	                files.o logging.o logerror-slim.o errorlogx-slim.o \
	                results.o queue.o sorting.o \
	                xmlparse.o tagging.o \
	                uncompress.o osmxmlparse.o osmpbfparse.o osmo5mparse.o osmparser.o

planetsplitter-slim : $(PLANETSPLITTER_SLIM_OBJ)
	$(LD) $(PLANETSPLITTER_SLIM_OBJ) -o $@ $(LDFLAGS)

########

ROUTER_OBJ=router.o \
	   nodes.o segments.o ways.o relations.o types.o fakes.o \
	   optimiser.o output.o \
	   files.o logging.o profiles.o xmlparse.o \
	   results.o queue.o translations.o

router : $(ROUTER_OBJ)
	$(LD) $(ROUTER_OBJ) -o $@ $(LDFLAGS)

########

ROUTER_SLIM_OBJ=router-slim.o \
	        nodes-slim.o segments-slim.o ways-slim.o relations-slim.o types.o fakes-slim.o \
	        optimiser-slim.o output-slim.o \
	        files.o logging.o profiles.o xmlparse.o \
	        results.o queue.o translations.o

router-slim : $(ROUTER_SLIM_OBJ)
	$(LD) $(ROUTER_SLIM_OBJ) -o $@ $(LDFLAGS)

########

FILEDUMPERX_OBJ=filedumperx.o \
	        files.o logging.o

filedumperx : $(FILEDUMPERX_OBJ)
	$(LD) $(FILEDUMPERX_OBJ) -o $@ $(LDFLAGS)

########

FILEDUMPER_OBJ=filedumper.o \
	       nodes.o segments.o ways.o relations.o types.o fakes.o errorlog.o \
               visualiser.o \
	       files.o logging.o xmlparse.o

filedumper : $(FILEDUMPER_OBJ)
	$(LD) $(FILEDUMPER_OBJ) -o $@ $(LDFLAGS)

########

FILEDUMPER_SLIM_OBJ=filedumper-slim.o \
	       nodes-slim.o segments-slim.o ways-slim.o relations-slim.o types.o fakes-slim.o errorlog-slim.o \
               visualiser-slim.o \
	       files.o logging.o xmlparse.o

filedumper-slim : $(FILEDUMPER_SLIM_OBJ)
	$(LD) $(FILEDUMPER_SLIM_OBJ) -o $@ $(LDFLAGS)

########

%.o : %.c
	@[ -d .deps ] || mkdir .deps
	$(CC) -c $(CFLAGS) -DSLIM=0 -DDATADIR=\"$(datadir)\" $< -o $@ -MMD -MP -MF $(addprefix .deps/,$(addsuffix .d,$(basename $@)))

%-slim.o : %.c
	@[ -d .deps ] || mkdir .deps
	$(CC) -c $(CFLAGS) -DSLIM=1 -DDATADIR=\"$(datadir)\" $< -o $@ -MMD -MP -MF $(addprefix .deps/,$(addsuffix .d,$(basename $@)))

########

test: all-local
	for dir in $(SUBDIRS); do \
	   ( cd $$dir && $(MAKE) $@ ); \
	done

########

install: install-local
	for dir in $(SUBDIRS); do \
	   ( cd $$dir && $(MAKE) $@ ); \
	done

install-local: all-local
	-[ -d $(DESTDIR)$(bindir) ] || mkdir -p $(DESTDIR)$(bindir)
	@[ -d $(DESTDIR)$(bindir) ] && \
	  for file in $(EXE); do \
	     echo cp $$file $(DESTDIR)$(bindir) ;\
	     cp -f $$file $(DESTDIR)$(bindir) ;\
	  done

########

clean: clean-local
	for dir in $(SUBDIRS); do \
	   ( cd $$dir && $(MAKE) $@ ); \
	done

clean-local:
	rm -f *~
	rm -f *.o
	rm -f core
	rm -f *.gcda *.gcno *.gcov gmon.out

########

distclean: distclean-local clean-local
	for dir in $(SUBDIRS); do \
	   ( cd $$dir && $(MAKE) $@ ); \
	done

distclean-local:
	-cd $(WEBBINDIR) && rm -f $(EXE)
	-rm -f $(EXE)
	-rm -f $(D)
	-rm -fr .deps

########

include $(D)

########

.PHONY:: all test install clean distclean

.PHONY:: all-local install-local clean-local distclean-local
