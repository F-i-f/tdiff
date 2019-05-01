#  fi-autoconf-macros - A collection of autoconf macros
#  fi_check_identifier
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
#   FI_CHECK_IDENTIFIER(identifier, includes [, header-description [, if-true [, if-false]]])
# Effect:
#   If identifer is found in the expansion of includes, then define a
#   preprocessor symbol HAVE_macro-name, and run if-true.  Otherwise
#   run if-false.  The symbol will have for documentation

AC_DEFUN([FI_CHECK_IDENTIFIER],
	 [FI_CHECK_EGREP([$1], [$1], [$2], [$3], [$4], [$5])
])
