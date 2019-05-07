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

dist_doc_DATA			= README \
				  README.md \
				  NEWS

AM_TESTS_ENVIRONMENT = export LC_ALL=C \
			      FAKEROOT="$(FAKEROOT)" \
			      TARGET_OS="$(TARGET_OS)" \
			      HAVE_S_IFDOOR="$(HAVE_S_IFDOOR)" \
			      HAVE_S_IFSOCK="$(HAVE_S_IFSOCK)" \
			      HAVE_ST_FLAGS="$(HAVE_ST_FLAGS)" \
			      HAVE_GETOPT_LONG="$(HAVE_GETOPT_LONG)" \
			      HAVE_ACLS="$(HAVE_ACLS)" \
			      HAVE_XATTRS="$(HAVE_XATTRS)" \
			      HAVE_NANOSECOND_TIME_RESOLUTION="$(HAVE_NANOSECOND_TIME_RESOLUTION)" \
			      ;
TEST_EXTENSIONS	     = .test
TESTS = tests/ent_pair_cache.test			\
	tests/error-unreadable-directory.test		\
	tests/error-unreadable-file.test		\
	tests/error-unwritable-directory.test		\
	tests/error-unwritable-file.test		\
	tests/genhash.test				\
	tests/parsing-duplicate-exec.test		\
	tests/parsing-missing-placeholders.test		\
	tests/parsing-missing-semicolon.test		\
	tests/parsing-wrong-preset.test			\
	tests/preset-alltimes.test			\
	tests/preset-amtimes.test			\
	tests/preset-contents.test			\
	tests/preset-hardlinks.test			\
	tests/preset-missing.test			\
	tests/preset-mode.test				\
	tests/preset-mtime.test				\
	tests/preset-none.test				\
	tests/preset-notimes.test			\
	tests/preset-owner.test				\
	tests/preset-acl-alltimes.test			\
	tests/preset-acl-amtimes.test			\
	tests/preset-acl-contents.test			\
	tests/preset-acl-hardlinks.test			\
	tests/preset-acl-missing.test			\
	tests/preset-acl-mode.test			\
	tests/preset-acl-mtime.test			\
	tests/preset-acl-none.test			\
	tests/preset-acl-notimes.test			\
	tests/preset-acl-owner.test			\
	tests/preset-flags-alltimes.test		\
	tests/preset-flags-amtimes.test			\
	tests/preset-flags-contents.test		\
	tests/preset-flags-hardlinks.test		\
	tests/preset-flags-missing.test			\
	tests/preset-flags-mode.test			\
	tests/preset-flags-mtime.test			\
	tests/preset-flags-none.test			\
	tests/preset-flags-notimes.test			\
	tests/preset-flags-owner.test			\
	tests/preset-root-alltimes.test			\
	tests/preset-root-amtimes.test			\
	tests/preset-root-contents.test			\
	tests/preset-root-hardlinks.test		\
	tests/preset-root-missing.test			\
	tests/preset-root-mode.test			\
	tests/preset-root-mtime.test			\
	tests/preset-root-none.test			\
	tests/preset-root-notimes.test			\
	tests/preset-root-owner.test			\
	tests/preset-xattr-alltimes.test		\
	tests/preset-xattr-amtimes.test			\
	tests/preset-xattr-contents.test		\
	tests/preset-xattr-hardlinks.test		\
	tests/preset-xattr-missing.test			\
	tests/preset-xattr-mode.test			\
	tests/preset-xattr-mtime.test			\
	tests/preset-xattr-none.test			\
	tests/preset-xattr-notimes.test			\
	tests/preset-xattr-owner.test			\
	tests/toggle-acl.test				\
	tests/toggle-atime.test				\
	tests/toggle-blocks.test			\
	tests/toggle-contents.test			\
	tests/toggle-contents-always.test		\
	tests/toggle-contents-always-diff.test		\
	tests/toggle-contents-exec.test			\
	tests/toggle-ctime.test				\
	tests/toggle-directories.test			\
	tests/toggle-exclude.test			\
	tests/toggle-flag-archived.test			\
	tests/toggle-flag-hidden.test			\
	tests/toggle-flag-nodump.test			\
	tests/toggle-flag-offline.test			\
	tests/toggle-flag-opaque.test			\
	tests/toggle-flag-readonly.test			\
	tests/toggle-flag-reparse.test			\
	tests/toggle-flag-sappend.test			\
	tests/toggle-flag-schange.test			\
	tests/toggle-flag-sparse.test			\
	tests/toggle-flag-sunlink.test			\
	tests/toggle-flag-system.test			\
	tests/toggle-flag-uappend.test			\
	tests/toggle-flag-uarchive.test			\
	tests/toggle-flag-uimmutable.test		\
	tests/toggle-flag-uunlink.test			\
	tests/toggle-gid.test				\
	tests/toggle-hardlinks.test			\
	tests/toggle-major.test				\
	tests/toggle-minor.test				\
	tests/toggle-mode.test				\
	tests/toggle-mode-and.test			\
	tests/toggle-mode-or.test			\
	tests/toggle-mtime.test				\
	tests/toggle-nlink.test				\
	tests/toggle-size.test				\
	tests/toggle-type.test				\
	tests/toggle-type-door.test			\
	tests/toggle-type-sock.test			\
	tests/toggle-type-root.test			\
	tests/toggle-uid.test				\
	tests/toggle-xattr.test

EXTRA_DIST = tdiff.spec						\
	     tdiff.1.html					\
	     tdiff.1.pdf					\
	     $(bash_FUNCTIONS)					\
	     $(zsh_FUNCTIONS)					\
	     tests/contents.lib.sh				\
	     tests/fakeroot.lib.sh				\
	     tests/mode.lib.sh					\
	     tests/pre-check-file.lib.sh			\
	     tests/preset.lib.sh				\
	     tests/require-acl.lib.sh				\
	     tests/require-xattr.lib.sh				\
	     tests/test.lib.sh					\
	     tests/type.lib.sh					\
	     $(TESTS)						\
	     tests/error-unreadable-directory.out.gold		\
	     tests/error-unreadable-file.out.gold		\
	     tests/error-unwritable-directory.out.gold		\
	     tests/error-unwritable-file.out.gold		\
	     tests/parsing-duplicate-exec.out.gold		\
	     tests/parsing-missing-placeholders.out.gold	\
	     tests/parsing-missing-semicolon.out.gold		\
	     tests/parsing-wrong-preset.out.gold		\
	     tests/preset-alltimes.out.gold			\
	     tests/preset-amtimes.out.gold			\
	     tests/preset-contents.out.gold			\
	     tests/preset-hardlinks.out.gold			\
	     tests/preset-missing.out.gold			\
	     tests/preset-mode.out.gold				\
	     tests/preset-mtime.out.gold			\
	     tests/preset-none.out.gold				\
	     tests/preset-notimes.out.gold			\
	     tests/preset-owner.out.gold			\
	     tests/preset-acl-alltimes.out.gold			\
	     tests/preset-acl-amtimes.out.gold			\
	     tests/preset-acl-contents.out.gold			\
	     tests/preset-acl-hardlinks.out.gold		\
	     tests/preset-acl-missing.out.gold			\
	     tests/preset-acl-mode.out.gold			\
	     tests/preset-acl-mtime.out.gold			\
	     tests/preset-acl-none.out.gold			\
	     tests/preset-acl-notimes.out.gold			\
	     tests/preset-acl-owner.out.gold			\
	     tests/preset-flags-alltimes.out.gold		\
	     tests/preset-flags-amtimes.out.gold		\
	     tests/preset-flags-contents.out.gold		\
	     tests/preset-flags-hardlinks.out.gold		\
	     tests/preset-flags-missing.out.gold		\
	     tests/preset-flags-mode.out.gold			\
	     tests/preset-flags-mtime.out.gold			\
	     tests/preset-flags-none.out.gold			\
	     tests/preset-flags-notimes.out.gold		\
	     tests/preset-flags-owner.out.gold			\
	     tests/preset-root-alltimes.out.gold		\
	     tests/preset-root-amtimes.out.gold			\
	     tests/preset-root-contents.out.gold		\
	     tests/preset-root-hardlinks.out.gold		\
	     tests/preset-root-missing.out.gold			\
	     tests/preset-root-mode.out.gold			\
	     tests/preset-root-mtime.out.gold			\
	     tests/preset-root-none.out.gold			\
	     tests/preset-root-notimes.out.gold			\
	     tests/preset-root-owner.out.gold			\
	     tests/preset-xattr-alltimes.out.gold		\
	     tests/preset-xattr-amtimes.out.gold		\
	     tests/preset-xattr-contents.out.gold		\
	     tests/preset-xattr-hardlinks.out.gold		\
	     tests/preset-xattr-missing.out.gold		\
	     tests/preset-xattr-mode.out.gold			\
	     tests/preset-xattr-mtime.out.gold			\
	     tests/preset-xattr-none.out.gold			\
	     tests/preset-xattr-notimes.out.gold		\
	     tests/preset-xattr-owner.out.gold			\
	     tests/toggle-acl.out.gold				\
	     tests/toggle-atime.out.gold			\
	     tests/toggle-blocks.out.gold			\
	     tests/toggle-contents.out.gold			\
	     tests/toggle-contents-always.out.gold		\
	     tests/toggle-contents-always-diff.out.gold		\
	     tests/toggle-contents-exec.out.gold		\
	     tests/toggle-ctime.out.gold			\
	     tests/toggle-exclude.out.gold			\
	     tests/toggle-directories.out.gold			\
	     tests/toggle-flag-archived.out.gold		\
	     tests/toggle-flag-hidden.out.gold			\
	     tests/toggle-flag-nodump.out.gold			\
	     tests/toggle-flag-offline.out.gold			\
	     tests/toggle-flag-opaque.out.gold			\
	     tests/toggle-flag-readonly.out.gold		\
	     tests/toggle-flag-reparse.out.gold			\
	     tests/toggle-flag-sappend.out.gold			\
	     tests/toggle-flag-schange.out.gold			\
	     tests/toggle-flag-sparse.out.gold			\
	     tests/toggle-flag-sunlink.out.gold			\
	     tests/toggle-flag-system.out.gold			\
	     tests/toggle-flag-uappend.out.gold			\
	     tests/toggle-flag-uarchive.out.gold		\
	     tests/toggle-flag-uimmutable.out.gold		\
	     tests/toggle-flag-uunlink.out.gold			\
	     tests/toggle-gid.out.gold				\
	     tests/toggle-hardlinks.out.gold			\
	     tests/toggle-major.out.gold			\
	     tests/toggle-minor.out.gold			\
	     tests/toggle-mode.out.gold				\
	     tests/toggle-mode-and.out.gold			\
	     tests/toggle-mode-or.out.gold			\
	     tests/toggle-mtime.out.gold			\
	     tests/toggle-nlink.out.gold			\
	     tests/toggle-size.out.gold				\
	     tests/toggle-type.out.gold				\
	     tests/toggle-type-door.out.gold			\
	     tests/toggle-type-root.out.gold			\
	     tests/toggle-type-sock.out.gold			\
	     tests/toggle-uid.out.gold				\
	     tests/toggle-xattr.out.gold

CLEANFILES = tests/*.tmp.* tests/*.out tests/*~

clean-local:
	-chflags -R nosimmutable tests/*.dir?
	-chflags -R nouimmutable tests/*.dir?
	-chflags -R nosappend    tests/*.dir?
	-chflags -R nouappend    tests/*.dir?
	-chmod -R u+rwx tests/*.dir?
	rm -fr tests/*.dir?

@FI_AUTOMAKE@
