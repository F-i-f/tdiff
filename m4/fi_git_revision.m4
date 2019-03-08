#  fi-autoconf-macros - A collection of autoconf macros
#  fi_git_revision
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
#   FI_GIT_REVISION([git-command])
# Effect:
#   If git is found on the PATH, defines the preprocessor symbol
#   GIT_REVISION with the current git revision as returned by `git
#   describe --tags --long --always' of by `git git-command' if
#   `git-command' has been passed

AC_DEFUN([FI_GIT_REVISION],
	 [AH_TEMPLATE([GIT_REVISION],
		      [Define to the git revision corresponding to the code checked out to build this release.])
	 AC_CHECK_PROG([have_git], [git], [yes], [no])
	 if test "x$have_git" = xyes
	 then
	   AC_MSG_CHECKING(for git revision)
	   if git_revision="`git m4_if([$1], [], [describe --tags --long --always], [$1])`"
	   then
	     AC_DEFINE_UNQUOTED([GIT_REVISION], ["$git_revision"])
	     AC_MSG_RESULT([$git_revision])
	   else
	     AC_MSG_RESULT([unknown or git failed])
	  fi
	fi
])
