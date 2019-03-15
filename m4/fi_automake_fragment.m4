#  fi-autoconf-macros - A collection of autoconf macros
#  fi_automake_fragment
#  Copyright (C) 2019 Philippe Troin <phil+github-commits@fifi.org>
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
#   FI_AUTOMAKE_FRAGMENT(makefile-fragment)
# Effect:
#   Outputs makefile-fragment in the Makefile
#   Makefile.am must include @FI_AUTOMAKE@, see FI_AUTOMAKE_FRAGMENT macro.

AC_DEFUN([FI_AUTOMAKE_FRAGMENT],
	 [m4_ifdef([fi_automake_fragment_initialized],
		   [],
		   [m4_define([fi_automake_fragment_initialized],[yes])
		   FI_AUTOMAKE=config.fi.frag.make
		   : > "$FI_AUTOMAKE"
		   AC_SUBST_FILE([FI_AUTOMAKE])
		   FI_AUTOMAKE_FRAGMENT(
[# Begin FI_AUTOMAKE_FRAGMENT() Makefile fragment
distclean-am: distclean-fi-automate-fragment

distclean-fi-automate-fragment:
	rm -f config.fi.frag.make

.PHONY: distclean-fi-automate-fragment
# End FI_AUTOMAKE_FRAGMENT Makefile fragment
])])
	 cat >> "$FI_AUTOMAKE"  <<'FI_AUTOMAKE_FRAGMENT_EOD'
$1
FI_AUTOMAKE_FRAGMENT_EOD])
