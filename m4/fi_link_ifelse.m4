#  fi-autoconf-macros - A collection of autoconf macros
#  fi_link_ifelse
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
#   FI_LINK_IFELSE(check-message, macro, code
#                 [, descr [, action-if-true [, action-if-false]]])
# Effect:
#   If code compiles and links:
#      - if action-if-true is defined, evaluate it
#      - else define the preprocessor symbol macro macro with a description desc
#   If the code doesn't link, evailuate action-if-false

AC_DEFUN([FI_LINK_IFELSE],
	 [AS_VAR_PUSHDEF([fi_cachevar], [AS_TR_SH([fi_cv_$2])])
	 AC_CACHE_CHECK([$1],[fi_cachevar],
			[AC_LINK_IFELSE([$3],
			[AS_VAR_SET([fi_cachevar], [yes])],
			[AS_VAR_SET([fi_cachevar], [no])])])
	 AS_IF([test "x[]AS_VAR_GET([fi_cachevar])" = xyes],
	       [m4_if([$5], [],
		      [AS_VAR_PUSHDEF([fi_vardef], [$2])
		       AC_DEFINE([fi_vardef], [1], [$4])
		       AS_VAR_POPDEF([fi_vardef])
		      ], [$5])],
	       [m4_if([$6], [], [:], [$6])])
	 AS_VAR_POPDEF([fi_cachevar])])
