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
AC_PREREQ([2.69])
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([gnu])

# Get started
AC_CONFIG_SRCDIR([tdiff.c])
AC_LANG([C])

### 1) Checking for system
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

### 2) Checking for programs
AC_PROG_CC
AC_PROG_CPP
AC_PROG_RANLIB
AC_PROG_EGREP
AC_PROG_INSTALL

# Embed git revision
AH_TEMPLATE([GIT_REVISION], [Define to the git revision corresponding to the code checked out to build this release.])
AC_CHECK_PROG([have_git], [git], [yes], [no])
if test "x$have_git" = xyes
then
  AC_MSG_CHECKING(for git revision)
  if git_head_ref="`git rev-parse HEAD`"
  then
    AC_DEFINE_UNQUOTED([GIT_REVISION], ["$git_head_ref"])
    AC_MSG_RESULT([$git_head_ref])
  else
    AC_MSG_RESULT([unknown or failed])
  fi
fi

### 3) Checking for libraries

### 4) Checking for header files
AC_MSG_CHECKING(for getdents() in libc)
AC_EGREP_CPP(getdents, [#include <sys/dirent.h>],
	     getdents=libc)
if test "$getdents"
then
    AC_MSG_RESULT(yes)
    AC_MSG_CHECKING(for getdents() in sys/dirent.h)
    AC_EGREP_CPP(getdents, [#include <sys/dirent.h>], getdents_sysdirent=1)
    if test "$getdents_sysdirent"
    then
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_SYS_DIRENT_H, 1,
	  [Define if your system has <sys/dirent.h>])
	AC_DEFINE(GETDENTS_STRUCT, dirent,
	  [Define to the name of your system struct returned by getdents()])
	AC_DEFINE(GETDENTS_RETURN_Q, f_dent,
	  [Define to the name of the filed inside struct GETDENTS_STRUCT that points to the next dentry.])
	getdents="sys_dirent"
    else
	AC_MSG_RESULT(no)
	unset getdents
    fi
else
    AC_MSG_RESULT(no)
    case "$target_os" in
      linux*)
	AC_MSG_CHECKING(for getdents syscall)
	AC_EGREP_CPP(found_sys_getdents,
	[#include <syscall.h>
	#ifdef SYS_getdents
	found_sys_getdents
	#endif
	], getdents=syscall)
	if test "$getdents"
	then
	    AC_MSG_RESULT(yes)
	    AC_DEFINE(HAVE_GETDENTS_SYSCALL_H,1,
	      [Define to 1 if you have SYS_getdents in <syscall.h>])
	    AC_DEFINE(GETDENTS_STRUCT, dent)
	    AC_DEFINE(GETDENTS_NEXTDENT, d_reclen,
	      [Define to the name of the field in your system's dentry structure that has a jump offeset to get to the next entry.]) dnl '
	    AC_DEFINE(GETDENTS_RETURN_Q, f_byte)
	else
	    AC_MSG_RESULT(no)
	fi
	;;
    esac
fi

case "$getdents" in
    sys_dirent)
	getdents_decl="#include <sys/dirent.h>"
	;;
    syscall)
	getdents_decl="#include <syscall.h>
	 #include <errno.h>
	 int getdents(int fd, char* buf, unsigned int count)
	 {
	    return syscall(SYS_getdents, fd, buf, count);
	 }
	 struct dent {
	   long           d_ino;
	   off_t          d_off;
	   unsigned short d_reclen;
	   char           *d_name;
	 };"
	;;
esac

AC_MSG_CHECKING(for random in stdlib.h)
AC_EGREP_CPP(random, [#include <stdlib.h>],
	     AC_MSG_RESULT(yes)
	     AC_DEFINE(HAVE_RANDOM,1,[FIXME]),
	     AC_MSG_RESULT(no))

major_keep_trying=yes
AC_MSG_CHECKING(for major and minor in mkdev.h)
AC_EGREP_CPP(major,
	     [#include <sys/mkdev.h>],
	     AC_MSG_RESULT(yes)
	     AC_DEFINE(HAVE_SYS_MKDEV_H,1,[FIXME])
	     unset major_keep_trying,
	     AC_MSG_RESULT(no))

if test "$major_keep_trying"
then
AC_MSG_CHECKING(for major and minor in sysmacros.h)
AC_EGREP_CPP(oh_yes,
	     [#include <sys/sysmacros.h>
	      #if defined(minor) && defined(major)
	      oh_yes
	      #endif
	     ],
	     AC_MSG_RESULT(yes)
	     AC_DEFINE(HAVE_SYS_SYSMACROS_H,1,[FIXME])
	     unset major_keep_trying,
	     AC_MSG_RESULT(no))
fi

if test "$major_keep_trying"
then
    AC_MSG_ERROR(Cannot find a definition for major() and minor())
fi

### 5) Checking for typedefs: NONE
AC_MSG_CHECKING(for O_NOATIME)
AC_EGREP_CPP(oh_yes_oh_yes, [#include <sys/types.h>
	     #include <sys/stat.h>
	     #include <fcntl.h>
	     #ifdef O_NOATIME
	     oh_yes_oh_yes
	     #endif],
	     AC_MSG_RESULT(yes)
	     AC_DEFINE(HAVE_O_NOATIME,1,[Define if your system has the O_NOATIME open() flag.]),
	     AC_MSG_RESULT(no))

AC_MSG_CHECKING(for S_IFLNK in stat)
AC_EGREP_CPP(oh_yes_oh_yes, [#include <sys/stat.h>
	     #ifdef S_IFLNK
	     oh_yes_oh_yes
	     #endif],
	     AC_MSG_RESULT(yes)
	     AC_DEFINE(HAVE_S_IFLNK,1,[FIXME]),
	     AC_MSG_RESULT(no))

AC_MSG_CHECKING(for S_IFIFO in stat)
AC_EGREP_CPP(oh_yes_oh_yes, [#include <sys/stat.h>
	     #ifdef S_IFIFO
	     oh_yes_oh_yes
	     #endif],
	     AC_MSG_RESULT(yes)
	     AC_DEFINE(HAVE_S_IFIFO,1,[FIXME]),
	     AC_MSG_RESULT(no))

AC_MSG_CHECKING(for S_IFSOCK in stat)
AC_EGREP_CPP(oh_yes_oh_yes, [#include <sys/stat.h>
	     #ifdef S_IFSOCK
	     oh_yes_oh_yes
	     #endif],
	     AC_MSG_RESULT(yes)
	     AC_DEFINE(HAVE_S_IFSOCK,1,[FIXME]),
	     AC_MSG_RESULT(no))

AC_MSG_CHECKING(for S_IFDOOR in stat)
AC_EGREP_CPP(oh_yes_oh_yes, [#include <sys/stat.h>
	     #ifdef S_IFDOOR
	     oh_yes_oh_yes
	     #endif],
	     AC_MSG_RESULT(yes)
	     AC_DEFINE(HAVE_S_IFDOOR,1,[FIXME]),
	     AC_MSG_RESULT(no))

AC_MSG_CHECKING(for S_IFWHT in stat)
AC_EGREP_CPP(oh_yes_oh_yes, [#include <sys/stat.h>
	     #ifdef S_IFWHT
	     oh_yes_oh_yes
	     #endif],
	     AC_MSG_RESULT(yes)
	     AC_DEFINE(HAVE_S_IFWHT,1,[FIXME]),
	     AC_MSG_RESULT(no))

AC_MSG_CHECKING(for BSD 4.4 flags in stat)
AC_EGREP_CPP(oh_yes_oh_yes, [#include <sys/stat.h>
	     #if defined(UF_NODUMP) && defined(UF_IMMUTABLE) && defined(UF_APPEND) && defined(UF_OPAQUE) && defined(UF_NOUNLINK) && defined(SF_ARCHIVED) && defined(SF_IMMUTABLE) && defined(SF_APPEND) && defined(SF_NOUNLINK)
	     oh_yes_oh_yes
	     #endif],
	     AC_MSG_RESULT(yes)
	     AC_DEFINE(HAVE_BSD4_4FLAGS, 1,
	       [Define to 1 if your system has the BSD4.4 UFS flags (UF_NODUMP, etc)]),
	     AC_MSG_RESULT(no))

### 6) Checking for structures:
AC_MSG_CHECKING(for BSD 4.4 extended times in stat)
AC_EGREP_CPP(st_atimespec, [#include <sys/stat.h>],
	     AC_MSG_RESULT(yes)
	     AC_DEFINE(HAVE_ST_XTIMESPEC,1,[FIXME]),
	     AC_MSG_RESULT(no))


AC_MSG_CHECKING(for st_flags in struct stat)
AC_EGREP_CPP(st_flags,[#include <sys/stat.h>],
	     AC_DEFINE(HAVE_ST_FLAGS,1,[FIXME])
	     AC_MSG_RESULT(yes),
	     AC_MSG_RESULT(no))

AC_MSG_CHECKING(for nanosecond resolution in stat)
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
  struct stat sbuf;
  return sbuf.st_atim.tv_nsec;
}])],
	     AC_MSG_RESULT(yes)
	     AC_DEFINE(HAVE_ST_TIMENSEC,1,[Define to 1 if your system provides nanosecond timestamps (st_atimensec,, st_ctimensec, st_mtimensec]),
	     AC_MSG_RESULT(no))

AC_MSG_CHECKING(for getxattr in attr/xattr.h)
AC_EGREP_CPP(getxattr,[#include <attr/xattr.h>],
	     AC_DEFINE(HAVE_GETXATTR,1,[Define to 1 if your system has xattrs])
	     AC_MSG_RESULT(yes),
	     AC_MSG_RESULT(no))


### 7) Checking for compiler characteristics
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(ino_t)
AC_CHECK_SIZEOF(dev_t)

### 8) Checking for library functions
AC_MSG_CHECKING(for getopt_long())
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>
	     #include <getopt.h>
	    ]], [[char *a[1];
	     getopt_long(0, a, "foo", NULL, NULL);
	    ]])],[AC_MSG_RESULT(yes)
	    AC_DEFINE(HAVE_GETOPT_LONG,1,[Define to 1 if your system has the GNU getopt_long().])],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for mallinfo())
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <malloc.h>]], [[mallinfo();]])],[AC_DEFINE(HAVE_MALLINFO,1,[FIXME])
	    AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for mmap())
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
	     #include <sys/mman.h>]], [[mmap((void*)0, 0, 0, 0, 0, 0);]])],[AC_MSG_RESULT(yes)
	    AC_DEFINE(HAVE_MMAP,1,[FIXME])],[AC_MSG_RESULT(no)])

AC_MSG_CHECKING(for madvise())
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
	     #include <sys/mman.h>]], [[madvise((void*)0, 0, 0);]])],[AC_MSG_RESULT(yes)
	    madvise=1],[AC_MSG_RESULT(no)])

if test "$madvise"
then
    AC_MSG_CHECKING(for working madvise())
    AC_RUN_IFELSE([AC_LANG_SOURCE([[
      #include <stdlib.h>
      #include <sys/types.h>
      #include <sys/mman.h>
      #include <fcntl.h>

      int main() {
	int fd = open("config.log", O_RDONLY);
	caddr_t addr = mmap(NULL, 10, PROT_READ, MAP_SHARED, fd, 0);
	return madvise(addr, 10, MADV_SEQUENTIAL)<0;
      }
    ]])],[AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_MADVISE,1,[FIXME])],[AC_MSG_RESULT(no)],[AC_MSG_ERROR(cannot test for working madvise() when cross-compiling)])
fi

if test "$getdents"
then
    AC_MSG_CHECKING(for working getdents)
    AC_RUN_IFELSE([AC_LANG_SOURCE([[
      #include <sys/types.h>
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
    ]])],[getdents_works=1],[],[AC_MSG_ERROR(cannot test for working getdents() when cross-compiling)])
    if test "$getdents_works"
    then
	AC_MSG_RESULT(yes)
	case "$getdents" in
	    libc)
		AC_DEFINE(HAVE_GETDENTS,1,[Define to 1 if your system had a working getdents() system call.])
		;;
	    syscall)
		AC_DEFINE(HAVE_GETDENTS,1)
		;;
	esac
    else
	AC_DEFINE(HAVE_GETDENTS,0)
	AC_MSG_RESULT(no)
    fi
fi

AC_MSG_CHECKING(for lstat())
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <sys/stat.h>
	    #include <unistd.h>]], [[struct stat buf;
	    lstat("/", &buf);]])],[AC_MSG_RESULT(yes)
	    AC_DEFINE(HAVE_LSTAT,1,[FIXME])],[AC_MSG_RESULT(no)])

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
