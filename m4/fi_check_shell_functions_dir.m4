#  fi-autoconf-macros - A collection of autoconf macros
#  fi_check_shell_functions_dir
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
#   FI_CHECK_SHELL_FUNCTIONS_DIR( shell, default-function-path)
# Effect:
#   Add --with-<shell>-functions-dir / --without-<shell>-functions-dir
#   options.
#
#   Calls AC_CHECK_PROG on <shell>.
#
#   If no --with/out-<shell>-functions-dir option is passed, then if
#   <shell> is detected, then a Makefile fragment is added and the
#   function installation directory will be <default-function-path>.
#
#   If --with-<shell>-functions-dir is passed, then a Makefile
#   fragment is added and the functions will be installed in the path
#   provided.
#
#   If --without-<shell>-functions-dir is passed, no Makefile fragment
#   will be added and no shell functions will be installed.
#
#   The Makefile fragment, when added, will install & uninstall all
#   the functions listed in <shell>_FUNCTIONS in function path as
#   described above.
#   If the <shell>_FUNCTIONS files have a .<shell> extension, it is
#   removed when the file is installed.
#   The Makefile fragment also defines defines a
#   <SHELL>_FUNCTIONS_PATH Makefile variable.
#   The <shell>_FUNCTIONS files should also added to EXTRA_DIST, as
#   this canot be done automatically.
#
#   The Makefile.am must include the @FI_AUTOMAKE@ macro in a line by
#   itself, see FI_AUTOMAKE_FRAGMENT().
#
# Example:
#   configure.ac:
#     FI_CHECK_SHELL_FUNCTIONS_DIR([zsh], [$(datarootdir)/zsh/site-functions])
#   Makefile.am:
#     zsh_FUNCTIONS = func.zsh
#     EXTRA_DIST = $(zsh_FUNCTIONS)
#     @FI_AUTOMAKE@

AC_DEFUN([FI_CHECK_SHELL_FUNCTIONS_DIR],
	 [AS_VAR_PUSHDEF([with_shell_fpath],[AS_TR_SH([with_$1[]_fpath])])
	 AS_VAR_SET([with_shell_fpath], [default])
	 AS_VAR_PUSHDEF([have_shell],[])
	 AC_ARG_WITH([AS_TR_SH([$1])-functions-dir],
	   AC_HELP_STRING([--with-[]AS_TR_SH([$1])-functions-dir=PATH],
			  [Install $1 completions to PATH])
AC_HELP_STRING([--without-[]AS_TR_SH([$1])-functions-dir],
			  [Do not install $1 completions]),
	   [AS_VAR_SET([with_shell_fpath],[$withval])])
	 AS_VAR_PUSHDEF([have_shell],[AS_TR_SH([have_$1])])
	 AC_CHECK_PROG(AS_TR_SH([have_$1]),[$1],[yes],[no])
	 if test "x[]AS_VAR_GET([with_shell_fpath])" = xdefault
	 then
	   if test "x[]AS_VAR_GET([have_shell])" = xyes
	   then
	     AS_VAR_SET([with_shell_fpath],[yes])
	   else
	     AS_VAR_SET([with_shell_fpath],[no])
	   fi
	 fi
	 AS_VAR_PUSHDEF([shell_functions_dir],[AS_TR_CPP([$1[]_FUNCTIONS_DIR])])
	 case "AS_VAR_GET([with_shell_fpath])" in
	   no)
	     ;;
	   yes)
	     AS_VAR_SET([shell_functions_dir], [$2])
	     AC_SUBST(shell_functions_dir) dnl No [] quoting
	     ;;
	   *)
	     AS_VAR_SET([shell_functions_dir], [AS_VAR_GET([with_shell_fpath])])
	     AC_SUBST(shell_functions_dir) dnl No [] quoting
	     ;;
	 esac
	 if test "x[]AS_VAR_GET([shell_functions_dir])" != x
	 then
	   FI_AUTOMAKE_FRAGMENT(
[# Begin FI_CHECK_SHELL_FUNCTIONS_DIR($1) Makefile fragment
install-data-fi-check-shell-functions-dir-[]AS_TR_SH([$1]): $(AS_TR_SH([$1])_FUNCTIONS)
	$(MKDIR_P) $(DESTDIR)$(AS_TR_CPP([$1[]_FUNCTIONS_DIR]))
	@for i in $(AS_TR_SH([$1])_FUNCTIONS); do \
	  echo "$(INSTALL_DATA) \"$(srcdir)/$$i\"" \
	    "\"$(DESTDIR)$(AS_TR_CPP([$1[]_FUNCTIONS_DIR]))/$$(echo "$$i" | sed -e 's,^.*/,,' -e 's,\.$1$$,,')\""; \
	  $(INSTALL_DATA) "$(srcdir)/$$i" \
			  "$(DESTDIR)$(AS_TR_CPP([$1[]_FUNCTIONS_DIR]))/$$(echo "$$i" | sed -e 's,^.*/,,' -e 's,\.$1$$,,')"; \
	done

uninstall-data-fi-check-shell-functions-dir-[]AS_TR_SH([$1]):
	@for i in $(AS_TR_SH([$1])_FUNCTIONS); do \
	  echo "rm -f \"$(DESTDIR)$(AS_TR_CPP([$1[]_FUNCTIONS_DIR]))/$$(echo "$$i" | sed -e 's,^.*/,,' -e 's,\.$1$$,,')\""; \
	  rm -f "$(DESTDIR)$(AS_TR_CPP([$1[]_FUNCTIONS_DIR]))/$$(echo "$$i" | sed -e 's,^.*/,,' -e 's,\.$1$$,,')"; \
	done
	rmdir $(DESTDIR)$(AS_TR_CPP([$1[]_FUNCTIONS_DIR])) || :

.PHONY: install-data-fi-check-shell-functions-dir-[]AS_TR_SH([$1]) uninstall-data-fi-check-shell-functions-dir-[]AS_TR_SH([$1])

install-data-am: install-data-fi-check-shell-functions-dir-[]AS_TR_SH([$1])
uninstall-am: uninstall-data-fi-check-shell-functions-dir-[]AS_TR_SH([$1])
# End FI_CHECK_SHELL_FUNCTIONS_DIR($1) Makefile fragment
])
	 fi
	 AS_VAR_POPDEF([shell_functions_dir])
	 AS_VAR_POPDEF([have_shell])
	 AS_VAR_POPDEF([with_shell_fpath])
])
