# Extra files Makefile
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

include ../Makefile.conf

# Sub-directories and sub-makefiles

SUBFILES=$(wildcard */Makefile)
SUBDIRS=$(foreach f,$(SUBFILES),$(dir $f))

########

all:
	for dir in $(SUBDIRS); do \
	   ( cd $$dir && $(MAKE) $@ ); \
	done

########

test:
	for dir in $(SUBDIRS); do \
	   ( cd $$dir && $(MAKE) $@ ); \
	done

########

install:
	for dir in $(SUBDIRS); do \
	   ( cd $$dir && $(MAKE) $@ ); \
	done

########

clean:
	for dir in $(SUBDIRS); do \
	   ( cd $$dir && $(MAKE) $@ ); \
	done

########

distclean:
	for dir in $(SUBDIRS); do \
	   ( cd $$dir && $(MAKE) $@ ); \
	done

########

.PHONY:: all test install clean distclean
