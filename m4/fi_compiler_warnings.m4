#  fi-autoconf-macros - A collection of autoconf macros
#  fi_compiler_warnings
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
#   Should be called after:
#     AM_MAINTAINER_MODE
#     AC_PROG_CC
#   FI_COMPILER_WARNINGS
# Effect:
#   Turns on maximum warning level

AC_DEFUN([FI_COMPILER_WARNINGS],
	 [AC_ARG_ENABLE([compiler-warnings],
			AS_HELP_STRING([--enable-compiler-warnings],
				       [Enable extra compiler warnings if the GNU C Compiler is detected.  Defaults to enabled when maintainer-mode is enabled.])
AS_HELP_STRING([--disable-compiler-warnings],
	       [Disable extra compiler warnings unconditionally.]),
			[fi_enable_compiler_warnings="$enableval"],
			[fi_enable_compiler_warnings=default])
	 if test "x$ac_compiler_gnu" = xyes -a \
		 \( \( "x$fi_enable_compiler_warnings" = xdefault \
		    -a "x$USE_MAINTAINER_MODE" = xyes \) \
		 -o "x$fi_enable_compiler_warnings" = xyes \)
	 then
	   FI_AUTOMAKE_FRAGMENT([# Added by FI_COMPILER_WARNINGS
][AM_CFLAGS += -Wstrict-prototypes -Werror -Wall -Wextra
])
	 fi])
