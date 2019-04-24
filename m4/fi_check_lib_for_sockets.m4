# -*- sh -*-
# aclocal - Library of autoconf macros
# fi_check_lib_for_sockets.m4 - Finds required libraries for socket calls
# Copyright (C) 2002, 2004 Philippe Troin <phil@fifi.org>
#
# $Id: fi_check_lib_for_sockets.m4 1826 2004-04-06 00:26:44Z phil $
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

# FI_CHECK_LIB_FOR_SOCKETS (no arguments)
#
# AC_SUBST's LIB_FOR_SOCKETS to a library to be linked in by socket programs.
# FI_USE_XNET should have been called before calling FI_CHECK_LIB_FOR_SOCKETS.


AC_DEFUN([FI_CHECK_LIB_FOR_SOCKETS], [
case "x$fi_cv_use_xnet" in
  xyes|xno) :;;
  *)  AC_MSG_ERROR([call [FI_USE_XNET] before invoking [FI_CHECK_LIB_FOR_SOCKETS]]) ;;
esac
if test "x$fi_cv_use_xnet" = xyes
then
  AC_CHECK_LIB(xnet, connect, [:])
fi
AC_CHECK_LIB(c, connect, [:])
AC_CHECK_LIB(socket, connect, [:])
AC_MSG_CHECKING([which library to use for socket functions])
AC_CACHE_VAL(fi_cv_lib_for_sockets,[
if test "x$ac_cv_lib_xnet_connect" = xyes \
  && test "x$fi_cv_use_xnet" = xyes
then
    fi_cv_lib_for_sockets="-lxnet"
else
    if test "x$ac_cv_lib_c_connect" = xyes
    then
	fi_cv_lib_for_sockets=""
    else
	if test "x$ac_cv_lib_socket_connect" = xyes
	then
	    fi_cv_lib_for_sockets="-lsocket"
	else
	    AC_MSG_ERROR([cannot find a library with socket functions])
	fi
    fi
fi
])
AC_SUBST(LIB_FOR_SOCKETS, $fi_cv_lib_for_sockets)
results="$fi_cv_lib_for_sockets"
if test "x$results" = "x"
then
    results="none required"
fi
AC_MSG_RESULT($results)
])
