#  fi-autoconf-macros - A collection of autoconf macros
#  fi_project
#  Copyright (C) 1999, 2006, 2008, 2014, 2019 Philippe Troin <phil+github-commits@fifi.org>
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

# Usage:
#   AC_INIT()
#   AC_AUX_DIR()
#   FI_PROJECT([automake-flags])
# Effect:
#   Initialize a project, call AM_INIT, and a few others.

AC_DEFUN([FI_PROJECT],
	 [AC_PREREQ([2.69])
	 AC_CONFIG_MACRO_DIR([m4])
	 AM_INIT_AUTOMAKE([-Wall gnu check-news $1])
	 AM_MAINTAINER_MODE()
	 FI_AUTOMAKE_FRAGMENT(
[# Begin FI_PROJECT() Makefile fragment
FI_CLEANFILES = *~ *.gcda *.gcno *.gcov

clean-am: clean-fi-project

clean-fi-project:
	@for i in '' m4 $(SUBDIRS) $(DIST_SUBDIRS); do       \
	  echo "cd \"$(srcdir)/$$i\" && rm -f $(FI_CLEANFILES)";    \
	  (cd "$(srcdir)/$$i" && rm -f $(FI_CLEANFILES)) || exit $$?; \
	done

.PHONY: clean-fi-project

distclean-am: distclean-fi-project

distclean-fi-project:
	rm -fr autom4te.cache .deps

.PHONY: distclean-fi-project

maintainer-clean-am: maintainer-clean-fi-project

maintainer-clean-fi-project:
	rm -f INSTALL Makefile.in aclocal.m4 configure config.h.in
	rm -fr config.aux

.PHONY: maintainer-clean-fi-project
#End FI_PROJECT() Makefile fragment
])
])
