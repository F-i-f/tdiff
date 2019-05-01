#  fi-autoconf-macros - A collection of autoconf macros
#  fi_check_egrep
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
#   FI_CHECK_EGREP(macro-name, text-to-grep, cpp-code , header-description [, if-true [, if-false]])
# Effect:
#   If text-to-grep is found in the C preprocessor expansion of
#   cpp-code, then run if-true.  Otherwise run if-false.  If if-true
#   is empty or not provided, define a preprocessor symbol
#   HAVE_macro-name, documentation "Define macro-name to 1 if
#   macron-name is defined header-description".

AC_DEFUN([FI_CHECK_EGREP],
	 [AS_VAR_PUSHDEF([fi_cachevar], [AS_TR_SH([fi_cv_$1])])
	 AC_CACHE_CHECK([for $1]m4_if([$4], [], [], [ $4]), [fi_cachevar],
			[AC_EGREP_CPP([$2], [$3],
				      [AS_VAR_SET([fi_cachevar], [yes])],
				      [AS_VAR_SET([fi_cachevar], [no])])])
	 AS_IF([test "x[]AS_VAR_GET([fi_cachevar])" = xyes],
	       [m4_if([$5], [],
		      [AS_VAR_PUSHDEF([fi_vardef], [AS_TR_CPP(HAVE_[]$1)])
		      AC_DEFINE([fi_vardef], [1], [Define ]fi_vardef[ to 1 if $1 is defined]m4_if([$4], [], [], [ $4]))
		      AS_VAR_POPDEF([fi_vardef])],
		      [$5])],
	       [m4_if([$6], [], [:], [$6])])
	 AS_VAR_POPDEF([fi_cachevar])])
