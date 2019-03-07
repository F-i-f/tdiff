#  tdiff - tree diffs
#  Makefile.am
#  Copyright (C) 2014, 2019 Philippe Troin <phil+github-commits@fifi.org>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

AM_CFLAGS   = -Wstrict-prototypes -Werror -Wall

noinst_LIBRARIES   = libtdiff.a

libtdiff_a_SOURCES = genhash.c genhash.h utils.c utils.h inocache.c inocache.h

bin_PROGRAMS    = tdiff

tdiff_SOURCES   = tdiff.c tdiff.h
tdiff_LDADD     = libtdiff.a

noinst_PROGRAMS = inocache_test genhash_test

inocache_test_SOURCES = inocache_test.c
inocache_test_LDADD   = libtdiff.a

genhash_test_SOURCES  = genhash_test.c
genhash_test_LDADD    = libtdiff.a

CLEANFILES = *~ */*~

check: $(noinst_PROGRAMS)
	./inocache_test
	./genhash_test

maintainer-clean-local:
	rm -f INSTALL Makefile.in aclocal.m4 configure config.h.in
	rm -fr config.aux
