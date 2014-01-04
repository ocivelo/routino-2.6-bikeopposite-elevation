# find-fixme Makefile
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

# Web file paths

WEBBINDIR=web/bin
WEBDATADIR=web/data
WEBWWWDIR=web/www

# Compilation targets

C=$(wildcard *.c)
D=$(wildcard .deps/*.d)

ROUTINO=../../src

EXE=fixme-finder fixme-finder-slim fixme-dumper fixme-dumper-slim
DATA=fixme.xml
WWW_COPY=$(notdir $(wildcard ../../$(webwwwdir)/maplayout*.css)) $(notdir $(wildcard ../../$(webwwwdir)/page-elements.*)) mapprops.js
DOC_COPY=$(notdir $(wildcard ../../doc/html/*.css))

########

all: all-routino $(EXE)
	-@[ -d $(WEBBINDIR) ] && \
	  for file in $(EXE); do \
	     if [ ! -f $(WEBBINDIR)/$$file ] || [ $$file -nt $(WEBBINDIR)/$$file ]; then \
	        echo cp $$file $(WEBBINDIR) ;\
	        cp -f $$file $(WEBBINDIR) ;\
	     fi ;\
	  done
	-@[ -d $(WEBDATADIR) ] && \
	  for file in $(DATA); do \
	     if [ ! -f $(WEBDATADIR)/$$file ] || [ $$file -nt $(WEBDATADIR)/$$file ]; then \
	        echo cp $$file $(WEBDATADIR) ;\
	        cp -f $$file $(WEBDATADIR) ;\
	     fi ;\
	  done
	-@[ -d $(WEBWWWDIR) ] && \
	  for file in $(WWW_COPY); do \
	     if [ ! -f $(WEBWWWDIR)/$$file ] || [ ../../$(webwwwdir)/$$file -nt $(WEBWWWDIR)/$$file ]; then \
	        echo cp ../../$(webwwwdir)/$$file $(WEBWWWDIR) ;\
	        cp -f ../../$(webwwwdir)/$$file $(WEBWWWDIR) ;\
	     fi ;\
	  done
	-@[ -d $(WEBWWWDIR) ] && \
	  for file in $(DOC_COPY); do \
	     if [ ! -f $(WEBWWWDIR)/$$file ] || [ ../../doc/html/$$file -nt $(WEBWWWDIR)/$$file ]; then \
	        echo cp ../../doc/html/$$file $(WEBWWWDIR) ;\
	        cp -f ../../doc/html/$$file $(WEBWWWDIR) ;\
	     fi ;\
	  done

all-routino:
	cd $(ROUTINO) && $(MAKE) all

########

FIXME_FINDER_OBJ=fixme-finder.o osmparser.o \
	       	 $(ROUTINO)/nodesx.o $(ROUTINO)/segmentsx.o $(ROUTINO)/waysx.o $(ROUTINO)/relationsx.o \
	       	 $(ROUTINO)/ways.o $(ROUTINO)/types.o \
	       	 $(ROUTINO)/files.o $(ROUTINO)/logging.o $(ROUTINO)/logerror.o $(ROUTINO)/errorlogx.o \
	       	 $(ROUTINO)/sorting.o \
	       	 $(ROUTINO)/xmlparse.o $(ROUTINO)/tagging.o \
	       	 $(ROUTINO)/uncompress.o $(ROUTINO)/osmxmlparse.o $(ROUTINO)/osmpbfparse.o $(ROUTINO)/osmo5mparse.o

fixme-finder : $(FIXME_FINDER_OBJ)
	$(LD) $(FIXME_FINDER_OBJ) -o $@ $(LDFLAGS)

########

FIXME_FINDER_SLIM_OBJ=fixme-finder-slim.o osmparser.o \
	       	      $(ROUTINO)/nodesx-slim.o $(ROUTINO)/segmentsx-slim.o $(ROUTINO)/waysx-slim.o $(ROUTINO)/relationsx-slim.o \
	              $(ROUTINO)/ways.o $(ROUTINO)/types.o \
	       	      $(ROUTINO)/files.o $(ROUTINO)/logging.o $(ROUTINO)/logerror-slim.o $(ROUTINO)/errorlogx-slim.o \
	              $(ROUTINO)/sorting.o \
	       	      $(ROUTINO)/xmlparse.o $(ROUTINO)/tagging.o \
	       	      $(ROUTINO)/uncompress.o $(ROUTINO)/osmxmlparse.o $(ROUTINO)/osmpbfparse.o $(ROUTINO)/osmo5mparse.o

fixme-finder-slim : $(FIXME_FINDER_SLIM_OBJ)
	$(LD) $(FIXME_FINDER_SLIM_OBJ) -o $@ $(LDFLAGS)

########

FIXME_DUMPER_OBJ=fixme-dumper.o \
	         $(ROUTINO)/errorlog.o \
	         $(ROUTINO)/files.o $(ROUTINO)/logging.o $(ROUTINO)/xmlparse.o

fixme-dumper : $(FIXME_DUMPER_OBJ)
	$(LD) $(FIXME_DUMPER_OBJ) -o $@ $(LDFLAGS)

########

FIXME_DUMPER_SLIM_OBJ=fixme-dumper-slim.o \
	              $(ROUTINO)/errorlog-slim.o \
	              $(ROUTINO)/files.o $(ROUTINO)/logging.o $(ROUTINO)/xmlparse.o

fixme-dumper-slim : $(FIXME_DUMPER_SLIM_OBJ)
	$(LD) $(FIXME_DUMPER_SLIM_OBJ) -o $@ $(LDFLAGS)

########

$(ROUTINO)/%.o :
	cd $(ROUTINO) && $(MAKE) $(notdir $@)

$(ROUTINO)/%-slim.o :
	cd $(ROUTINO) && $(MAKE) $(notdir $@)

%.o : %.c
	@[ -d .deps ] || mkdir .deps
	$(CC) -c $(CFLAGS) -DSLIM=0 -I$(ROUTINO) $< -o $@ -MMD -MP -MF $(addprefix .deps/,$(addsuffix .d,$(basename $@)))

%-slim.o : %.c
	@[ -d .deps ] || mkdir .deps
	$(CC) -c $(CFLAGS) -DSLIM=1 -I$(ROUTINO) $< -o $@ -MMD -MP -MF $(addprefix .deps/,$(addsuffix .d,$(basename $@)))

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
	-cd $(WEBBINDIR) && rm -f $(EXE)
	-cd $(WEBDATADIR) && rm -f $(DATA)
	-cd $(WEBWWWDIR) && rm -f $(WWW_COPY)
	-cd $(WEBWWWDIR) && rm -f $(DOC_COPY)
	-rm -f $(EXE)
	-rm -f $(D)
	-rm -fr .deps

########

include $(D)

########

.PHONY:: all all-routino test install clean distclean
