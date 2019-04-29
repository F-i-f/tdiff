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

libtdiff_a_SOURCES		= ent_key.h		\
				  ent_pair_cache.c	\
				  ent_pair_cache.h	\
				  genhash.c		\
				  genhash.h		\
				  hard_links_cache.c    \
				  hard_links_cache.h	\
				  str_list.c		\
				  str_list.h		\
				  utils.c		\
				  utils.h

bin_PROGRAMS			= tdiff

tdiff_SOURCES			= tdiff.c tdiff.h
tdiff_LDADD			= libtdiff.a
noinst_HEADERS			= linux_getdents.c	\
				  linux_getdents64.c
dist_man1_MANS			= tdiff.1

noinst_PROGRAMS			= ent_pair_cache_test	\
				  genhash_test

genhash_test_SOURCES		= genhash_test.c
genhash_test_LDADD		= libtdiff.a

ent_pair_cache_test_SOURCES	= ent_pair_cache_test.c
ent_pair_cache_test_LDADD	= libtdiff.a

if HAVE_S_IFSOCK
noinst_PROGRAMS		       += make_sock
make_sock_SOURCES		= make_sock.c
make_sock_LDADD			= $(LIB_FOR_SOCKETS)
endif

if HAVE_S_IFDOOR
noinst_PROGRAMS		       += make_door
make_door_SOURCES		= make_door.c
endif

bash_FUNCTIONS			= tdiff.bash
zsh_FUNCTIONS			= _tdiff.zsh

AM_TESTS_ENVIRONMENT = export LC_ALL=C \
			      FAKEROOT="$(FAKEROOT)" \
			      HAVE_S_IFDOOR="$(HAVE_S_IFDOOR)" \
			      HAVE_S_IFSOCK="$(HAVE_S_IFSOCK)" \
			      HAVE_ST_FLAGS="$(HAVE_ST_FLAGS)" \
			      HAVE_ACLS="$(HAVE_ACLS)" \
			      HAVE_XATTRS="$(HAVE_XATTRS)" \
			      ;
TEST_EXTENSIONS	     = .test
TESTS = tests/ent_pair_cache.test	\
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
	tests/error.test		\
	tests/exclude.test		\
	tests/flag-archived.test	\
	tests/flag-hidden.test		\
	tests/flag-nodump.test		\
	tests/flag-offline.test		\
	tests/flag-opaque.test		\
	tests/flag-readonly.test	\
	tests/flag-reparse.test		\
	tests/flag-sappend.test		\
	tests/flag-schange.test		\
	tests/flag-sparse.test		\
	tests/flag-sunlink.test		\
	tests/flag-system.test		\
	tests/flag-uappend.test		\
	tests/flag-uarchive.test	\
	tests/flag-uimmutable.test	\
	tests/flag-uunlink.test		\
	tests/group.test		\
	tests/major.test		\
	tests/minor.test		\
	tests/mode.test			\
	tests/mode-and.test		\
	tests/mode-or.test		\
	tests/mtime.test		\
	tests/nlink.test		\
	tests/owner.test		\
	tests/size.test			\
	tests/type.test			\
	tests/type-door.test		\
	tests/type-sock.test		\
	tests/type-root.test		\
	tests/xattr.test

EXTRA_DIST = README.md					\
	     tdiff.spec					\
	     tdiff.1.html				\
	     tdiff.1.pdf				\
	     $(bash_FUNCTIONS)				\
	     $(zsh_FUNCTIONS)				\
	     tests/contents.lib.sh			\
	     tests/fakeroot.lib.sh			\
	     tests/mode.lib.sh				\
	     tests/pre-check-file.lib.sh		\
	     tests/test.lib.sh				\
	     tests/type.lib.sh				\
	     $(TESTS)					\
	     tests/acl.out.gold				\
	     tests/atime.out.gold			\
	     tests/blocks.out.gold			\
	     tests/contents.out.gold			\
	     tests/contents-always.out.gold		\
	     tests/contents-always-diff.out.gold	\
	     tests/contents-exec.out.gold		\
	     tests/ctime.out.gold			\
	     tests/error.out.gold			\
	     tests/exclude.out.gold			\
	     tests/directories.out.gold			\
	     tests/flag-archived.out.gold		\
	     tests/flag-hidden.out.gold			\
	     tests/flag-nodump.out.gold			\
	     tests/flag-offline.out.gold		\
	     tests/flag-opaque.out.gold			\
	     tests/flag-readonly.out.gold		\
	     tests/flag-reparse.out.gold		\
	     tests/flag-sappend.out.gold		\
	     tests/flag-schange.out.gold		\
	     tests/flag-sparse.out.gold			\
	     tests/flag-sunlink.out.gold		\
	     tests/flag-system.out.gold			\
	     tests/flag-uappend.out.gold		\
	     tests/flag-uarchive.out.gold		\
	     tests/flag-uimmutable.out.gold		\
	     tests/flag-uunlink.out.gold		\
	     tests/group.out.gold			\
	     tests/major.out.gold			\
	     tests/minor.out.gold			\
	     tests/mode.out.gold			\
	     tests/mode-and.out.gold			\
	     tests/mode-or.out.gold			\
	     tests/mtime.out.gold			\
	     tests/nlink.out.gold			\
	     tests/owner.out.gold			\
	     tests/size.out.gold			\
	     tests/type.out.gold			\
	     tests/type-door.out.gold			\
	     tests/type-root.out.gold			\
	     tests/type-sock.out.gold			\
	     tests/xattr.out.gold

@FI_AUTOMAKE@
