#  tdiff - tree diffs
#  configure.ac
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

AC_INIT([tdiff],[0.2.99.WIP])
AC_CONFIG_AUX_DIR([config.aux])
AC_CANONICAL_TARGET()
FI_PROJECT()

# Get started
AC_CONFIG_SRCDIR([tdiff.c])
AC_LANG([C])

### 1) Checking for system
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

### 2) Checking for programs
AC_PROG_CC
AC_PROG_CPP
AM_PROG_AR
AC_PROG_RANLIB
AC_PROG_EGREP
AC_PROG_INSTALL

FI_GIT_REVISION

### 3) Checking for libraries

### 4) Checking for header files

# Getdents
AH_TEMPLATE(HAVE_SYS_DIRENT_H, [Define if your system has <sys/dirent.h>])
AH_TEMPLATE(GETDENTS_STRUCT, [Define to the name of your system struct returned by getdents()])
AH_TEMPLATE(GETDENTS_RETURN_Q, [Define to the name of the field inside struct GETDENTS_STRUCT that points to the next dentry.])
AH_TEMPLATE(GETDENTS_NEXTDENT, [Define to the name of the field in your system's dentry structure that has a jump offeset to get to the next entry.]) dnl '
AH_TEMPLATE(HAVE_GETDENTS_SYSCALL_H, [Define to 1 if you have SYS_getdents in <syscall.h>])

getdents="not-found"

FI_CHECK_EGREP([getdents],
	       [getdents],
	       [#include <sys/dirent.h>],
	       [in libc],
	       [getdents="sys_dirent"
		AC_DEFINE(HAVE_SYS_DIRENT_H, 1)
		AC_DEFINE(GETDENTS_STRUCT, dirent)
		AC_DEFINE(GETDENTS_RETURN_Q, f_dent)
	       ])

if test "x$getdents" = xnot-found
then
  case "$target_os" in
    linux*)
      FI_CHECK_EGREP([getdents syscall],
		     [fi_found_sys_getdents],
		     [#include <syscall.h>
		      #ifdef SYS_getdents
		      fi_found_sys_getdents
		      #endif
		     ],
		     [],
		     [getdents=syscall
		      AC_DEFINE(HAVE_GETDENTS_SYSCALL_H,1)
		      AC_DEFINE(GETDENTS_STRUCT, linux_dent)
		      AC_DEFINE(GETDENTS_NEXTDENT, d_reclen)
		      AC_DEFINE(GETDENTS_RETURN_Q, f_byte)
		      ])
      ;;
  esac
fi

case "$getdents" in
    sys_dirent)
	getdents_decl="#include <sys/dirent.h>"
	;;
    syscall)
	getdents_decl="#include \"linux_getdents.c\""
	;;
esac

AC_CACHE_CHECK([for random in stdlib.h], fi_cv_have_random, [
  AC_EGREP_CPP(random, [#include <stdlib.h>],
	       fi_cv_have_random=yes,
	       fi_cv_have_random=no)])
if test "x$fi_cv_have_random" = xyes
then
  AC_DEFINE(HAVE_RANDOM,1,[FIXME])
fi

# Major / minor
AH_TEMPLATE(HAVE_SYS_MKDEV_H, [Define to 1 if your system has major and minor definitions in mkdev.h])
AH_TEMPLATE(HAVE_SYS_SYSMACROS_H, [Define to 1 if your system has major and minor definitions in sys/sysmacros.h])
AH_TEMPLATE(HAVE_MAJOR_MINOR_FUNCTIONS, [Define to 1 if your system has major and minor functions])

major_keep_trying=yes
AC_CACHE_CHECK([for major and minor in mkdev.h], fi_cv_majmin_mkdev, [
  AC_EGREP_CPP(major,
	       [#include <sys/mkdev.h>],
	       fi_cv_majmin_mkdev=yes,
	       fi_cv_majmin_mkdev=no)])
if test "x$fi_cv_majmin_mkdev" = xyes
then
  AC_DEFINE(HAVE_SYS_MKDEV_H,1)
  unset major_keep_trying
fi

if test "$major_keep_trying"
then
  AC_CACHE_CHECK([for major and minor in sysmacros.h], fi_cv_majmin_sysmacros, [
    AC_EGREP_CPP(oh_yes,
		 [#include <sys/sysmacros.h>
		  #if defined(minor) && defined(major)
		  oh_yes
		  #endif
		 ],
		 fi_cv_majmin_sysmacros=yes,
		 fi_cv_majmin_sysmacros=no)])
  if test "x$fi_cv_majmin_sysmacros" = xyes
  then
    AC_DEFINE(HAVE_SYS_SYSMACROS_H, 1)
    unset major_keep_trying
  fi
fi

if test "$major_keep_trying"
then
  AC_CACHE_CHECK([for major() and minor() functions], [fi_cv_majmin_functions],
  [AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
  return major(0)+minor(0);
}])],
		     [fi_cv_majmin_functions=yes],
		     [fi_cv_majmin_functions=no])])
  if test "x$fi_cv_majmin_functions" = xyes
  then
    AC_DEFINE(HAVE_MAJOR_MINOR_FUNCTIONS, 1)
    unset major_keep_trying
  fi
fi

if test "$major_keep_trying"
then
    AC_MSG_ERROR([Cannot find a definition for major() and minor()])
fi

### 5) Checking for typedefs: NONE

### 6) Checking for macros

FI_CHECK_DEFINES([O_NOATIME O_DIRECTORY], [#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>])

FI_CHECK_DEFINES([S_IFLNK S_IFIFO S_IFSOCK S_IFDOOR S_IFWHT \
		  UF_NODUMP UF_IMMUTABLE UF_APPEND UF_OPAQUE UF_NOUNLINK \
		  UF_COMPRESSED UF_TRACKED UF_SYSTEM UF_SPARSE UF_OFFLINE \
		  UF_REPARSE UF_ARCHIVE UF_READONLY UF_HIDDEN \
		  SF_ARCHIVED SF_IMMUTABLE SF_APPEND SF_NOUNLINK SF_SNAPSHOT],
		 [#include <sys/stat.h>],
		 [in <sys/stat.h>])

### 6) Checking for structures:
AH_TEMPLATE([HAVE_ST_XTIMESPEC],[Define to 1 if your struct stat has the st_Xtimespec fields])
FI_CHECK_IDENTIFIERS([st_atimespec st_flags],
		     [#include <sys/stat.h>],
		     [in <sys/stat.h>])

AC_CACHE_CHECK([for nanosecond resolution in stat], [fi_cv_have_st_timensec],
[AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
  struct stat sbuf;
  return sbuf.st_atim.tv_nsec;
}])],
		   [fi_cv_have_st_timensec=yes],
		   [fi_cv_have_st_timensec=no])])
if test "x$fi_cv_have_st_timensec" = xyes
then
  AC_DEFINE([HAVE_ST_TIMENSEC], [1],
	    [Define to 1 if your system provides nanosecond timestamps (st_atimensec,, st_ctimensec, st_mtimensec)])
fi

FI_CHECK_IDENTIFIER([getxattr], [#include <attr/xattr.h>], [in <attr/xattr.h>])

### 7) Checking for compiler characteristics
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(ino_t)
AC_CHECK_SIZEOF(dev_t)

### 8) Checking for library functions
FI_LINK_IFELSE([for getopt_long()], [HAVE_GETOPT_LONG],
	       [AC_LANG_PROGRAM([[#include <unistd.h>
				  #include <getopt.h>
				 ]],
				[[char *a[1];
				  getopt_long(0, a, "foo", NULL, NULL);
				 ]])],
	       [Define to 1 if your system has the GNU getopt_long().])

FI_LINK_IFELSE([for fdopendir()], [HAVE_FDOPENDIR],
	       [AC_LANG_PROGRAM([[#include <sys/types.h>
				  #include <dirent.h>
				 ]],
				[[fdopendir(0);
				 ]])],
	       [Define to 1 if your system has fdopendir().])

FI_LINK_IFELSE([for mallinfo()], [HAVE_MALLINFO],
	       [AC_LANG_PROGRAM([[#include <malloc.h>]],
				[[mallinfo();]])],
	       [Define to 1 if your system has mallinfo().])

if test "x$getdents" != "xnot-found"
then
  AC_CACHE_CHECK([for working getdents],
		 [fi_cv_working_getdents],
		 [fi_saved_CPPFLAGS="$CPPFLAGS"
		 CPPFLAGS="$CPPFLAGS -I$srcdir"
		 AC_RUN_IFELSE([AC_LANG_SOURCE([[#include <sys/types.h>
						  #include <fcntl.h>
						  $getdents_decl
						  int main() {
						    char buf[10000];
						    int fd;
						    int nread;
						    fd = open(".", O_RDONLY);
						    nread = getdents(fd, buf, 10000);
						    return nread<=0;
						  }
						 ]])
				],
				[fi_cv_working_getdents=yes],
				[fi_cv_working_getdents=no],
				[fi_cv_working_getdents=cross])
		 CPPFLAGS="$fi_saved_CPPFLAGS"])

  if test "x$fi_cv_working_getdents" = xcross
  then
    AC_MSG_WARN([assuming that getdents() works on cross-compiling target])
    fi_cv_working_getdents=yes
  fi
  if test "x$fi_cv_working_getdents" = xyes
  then
    AC_DEFINE(HAVE_GETDENTS,1,[Define to 1 if your system had a working getdents().])
  fi
fi

FI_LINK_IFELSE([for lstat()], [HAVE_LSTAT],
	       [AC_LANG_PROGRAM([[#include <sys/stat.h>
				  #include <unistd.h>
				 ]],
				[[struct stat buf;
				  lstat("/", &buf);]
				])],
	       [Define to 1 if your system has lstat().])

AC_SEARCH_LIBS([acl_get_file],[acl],[
  AC_DEFINE([HAVE_ACL],1,[Define if your system has ACLs])
])

### 9) Check for "system services"

### END: output

AC_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

# Local variables:
# mode: sh
# end:
