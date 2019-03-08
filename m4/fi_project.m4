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
	 AM_INIT_AUTOMAKE([-Wall gnu check-news $3])
	 AM_MAINTAINER_MODE()
])
