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
#   Calls AC_CHECK_PROG on <shell>.
#   If no --with/out-<shell>-functions-dir option is passed, then if <shell>
#   is detected, then the functions will be installed in
#   <default-function-path>.
#   If --with-<shell>-functions-dir is passed, then the functions will be
#   installed in the path provided.
#   If --without-<shell>-functions-dir is passed, no functions will be
#   installed.
#   Also defines the INSTALL_<SHELL>_FUNCTIONS automake conditional, and
#   the <SHELL>_FUNCTIONS_PATH Makefile expansion
#
# Example:
#   configure.ac:
#     FI_CHECK_SHELL_FUNCTIONS_DIR([zsh],[$datarootdir/zsh/site-functions])
#   Makefile.am:
#     ZSH_FUNCTIONS_DIR = @ZSH_FUNCTIONS_DIR@
#     EXTRA_DIST = func.zsh
#     install-data-local:
#     if INSTALL_ZSH_FUNCTIONS
#	$(MKDIR_P) $(DESTDIR)$(ZSH_FUNCTIONS_DIR)
#	$(INSTALL_DATA) $(srcdir)/func.zsh $(DESTDIR)$(ZSH_FUNCTIONS_DIR)/func
#     endif
#     uninstall-local:
#     if INSTALL_ZSH_FUNCTIONS
#	rm -f $(DESTDIR)$(ZSH_FUNCTIONS_DIR)/func
#	rmdir $(DESTDIR)$(ZSH_FUNCTIONS_DIR) || : Ignoring rmdir failure
#     endif


AC_DEFUN([FI_CHECK_SHELL_FUNCTIONS_DIR],
	 [AS_VAR_PUSHDEF([with_shell_fpath],[with_$1[]_fpath])
	 AS_VAR_SET([with_shell_fpath], [default])
	 AS_VAR_PUSHDEF([have_shell],[])
	 AC_ARG_WITH([$1-functions-dir],
	   AC_HELP_STRING([--with-$1-functions-dir=PATH],
			  [Install $1 completions to PATH])
AC_HELP_STRING([--without-$1-functions-dir],
			  [Do not install $1 completions]),
	   [AS_VAR_SET([with_shell_fpath],[$withval])])
	 AS_VAR_PUSHDEF([have_shell],[have_$1])
	 AC_CHECK_PROG([have_shell],[$1],[yes],[no])
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
	     AC_SUBST(shell_functions_dir)
	     ;;
	   *)
	     AS_VAR_SET([shell_functions_dir], [$with_zsh_fpath])
	     AC_SUBST(shell_functions_dir) dnl No [] quoting
	     ;;
	 esac
	 AS_VAR_PUSHDEF([install_shell_functions],
			[AS_TR_CPP([INSTALL_$1[]_FUNCTIONS])])
	 AM_CONDITIONAL(install_shell_functions, dnl No [] quoting
			[test "x[]AS_VAR_GET([shell_functions_dir])" != x])
	 AS_VAR_POPDEF([install_shell_functions])
	 AS_VAR_POPDEF([have_shell])
	 AS_VAR_POPDEF([shell_functions_dir])
	 AS_VAR_POPDEF([with_shell_fpath])
])
