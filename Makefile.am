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
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

ACLOCAL_AMFLAGS = -I m4

noinst_LIBRARIES   = libtdiff.a

libtdiff_a_SOURCES = genhash.c genhash.h utils.c utils.h inocache.c inocache.h

bin_PROGRAMS	= tdiff

tdiff_SOURCES	= tdiff.c tdiff.h
tdiff_LDADD	= libtdiff.a

noinst_PROGRAMS = inocache_test \
		  genhash_test

genhash_test_SOURCES  = genhash_test.c
genhash_test_LDADD    = libtdiff.a

inocache_test_SOURCES = inocache_test.c
inocache_test_LDADD   = libtdiff.a

if HAVE_S_IFSOCK
noinst_PROGRAMS	     += make_sock
make_sock_SOURCES     = make_sock.c
endif

bash_FUNCTIONS	      = tdiff.bash
zsh_FUNCTIONS	      = _tdiff.zsh

AM_TESTS_ENVIRONMENT = export LC_ALL=C \
			      FAKEROOT="$(FAKEROOT)" \
			      HAVE_ACLS="$(HAVE_ACLS)" \
			      HAVE_S_IFSOCK="$(HAVE_S_IFSOCK)" \
			      HAVE_XATTRS="$(HAVE_XATTRS)" \
			      ;
TEST_EXTENSIONS	     = .test
TESTS = tests/inocache.test		\
	tests/genhash.test		\
	tests/acl.test			\
	tests/atime.test		\
	tests/blocks.test		\
	tests/contents.test		\
	tests/contents-always.test	\
	tests/contents-always-diff.test	\
	tests/contents-exec.test	\
	tests/ctime.test		\
	tests/directories.test		\
	tests/exclude.test		\
	tests/group.test		\
	tests/major.test		\
	tests/minor.test		\
	tests/mode.test			\
	tests/mode-and.test		\
	tests/mode-or.test		\
	tests/mtime.test		\
	tests/nlinks.test		\
	tests/owner.test		\
	tests/size.test			\
	tests/type.test			\
	tests/type-sock.test		\
	tests/type-root.test		\
	tests/xattr.test

EXTRA_DIST = $(bash_FUNCTIONS) $(zsh_FUNCTIONS)		\
	     $(TESTS)					\
	     tests/acl.out.gold				\
	     tests/atime.out.gold			\
	     tests/blocks.out.gold			\
	     tests/contents.lib				\
	     tests/contents.out.gold			\
	     tests/contents-always.out.gold		\
	     tests/contents-always-diff.out.gold	\
	     tests/contents-exec.out.gold		\
	     tests/ctime.out.gold			\
	     tests/exclude.out.gold			\
	     tests/fakeroot.lib				\
	     tests/directories.out.gold			\
	     tests/group.out.gold			\
	     tests/major.out.gold			\
	     tests/minor.out.gold			\
	     tests/mode.lib				\
	     tests/mode.out.gold			\
	     tests/mode-and.out.gold			\
	     tests/mode-or.out.gold			\
	     tests/mtime.out.gold			\
	     tests/nlinks.out.gold			\
	     tests/owner.out.gold			\
	     tests/size.out.gold			\
	     tests/test.lib				\
	     tests/type.lib				\
	     tests/type.out.gold			\
	     tests/type-root.out.gold			\
	     tests/type-sock.out.gold			\
	     tests/xattr.out.gold

@FI_AUTOMAKE@
