# -*- sh -*-
# aclocal - Library of autoconf macros
# fi_use_xnet.m4 - Select whether or not to use the X/Open networking library.
# Copyright (C) 2000, 2002, 2004 Philippe Troin <phil@fifi.org>
#
# $Id: fi_use_xnet.m4 1830 2004-04-06 03:18:18Z phil $
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# FI_USE_XNET (yesorno)
#
# Select whether or not to use -lxnet.

AC_DEFUN([FI_USE_XNET], [
case "x$1" in dnl (((
  [x1|x[yY][eE][sS]|x[tT][rR][uU][eE]]) fi_use=yes;;
  [x0|x[nN][oO]|x[fF][aA][lL][sS][eE]]) fi_use=no;;
  *) AC_MSG_ERROR([[FI_USE_XNET]: unknown argument $1, must be true/yes or false/no]) ;;
esac
AC_CACHE_CHECK([if the package requested X/Open networking], fi_cv_use_xnet,
[fi_cv_use_xnet=$fi_use])
if test "x$fi_cv_use_xnet" != "x$fi_use"
then
  AC_MSG_ERROR([[FI_USE_XNET] is set to $fi_cv_use_xnet in the cache and to $fi_use
		  in the current run, potentially causing problems.
	   => Please remove the cache and restart configure.])
fi
  case $fi_use in dnl ((
    yes) fi_use=1;;
    no)  fi_use=0;;
    *)   AC_MSG_ERROR([[FI_USE_XNET]: unexpected fi_use value $fi_use]);;
  esac
  AC_DEFINE_UNQUOTED(USE_XNET, $fi_use,
[Do not change this setting, it is PACKAGE-dependent, not system dependent.])
])
