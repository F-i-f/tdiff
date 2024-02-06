/*
  tdiff - tree diffs
  Main()
  Copyright (C) 1999, 2008, 2014, 2019, 2022, 2024 Philippe Troin <phil+github-commits@fifi.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#if HAVE_TERMIOS_H
# include <termios.h>
#endif

#if HAVE_SYS_TTY_H
# include <sys/tty.h>
#endif

#if HAVE_GETDENTS
#  if HAVE_SYS_DIRENT_H
#    include <sys/dirent.h>
#  elif HAVE_GETDENTS64_SYSCALL_H
#    include "linux_getdents64.c"
#  elif HAVE_GETDENTS_SYSCALL_H
#    include "linux_getdents.c"
#  else
#    error HAVE_GETDENTS is set, but I do not know how to get it !!!
#  endif
#endif

#if HAVE_GETOPT_LONG
#  include <getopt.h>
#endif

#if !HAVE_LSTAT
#  define lstat(f,b) stat(f,b)
#endif

#if HAVE_SYS_MKDEV_H
#  include <sys/mkdev.h>
#elif HAVE_SYS_SYSMACROS_H
#  include <sys/sysmacros.h>
#elif ! HAVE_MAJOR_MINOR_FUNCTIONS
#  error Cannot find major() and minor()
#endif

#if HAVE_LGETXATTR
#include <sys/xattr.h>
#endif

#if HAVE_ACL
#include <sys/acl.h>
#  if HAVE_ACL_CMP
#    include <acl/libacl.h>
#  endif
#endif

#include "tdiff.h"
#include "str_list.h"
#include "utils.h"
#include "ent_pair_cache.h"
#include "hard_links_cache.h"
#include "st_xtime_ns.h"

#define UNUSED(x) ((void)(x))

#define XATTR_BUF_SIZE		16384
#define GETDIRLIST_DENTBUF_SIZE 8192
#define XREADLINK_BUF_SIZE	1024
#define CMPFILE_BUF_SIZE	16384

#define VERB_STATS	1
#define VERB_SKIPS	2
#define VERB_HASH_STATS 3
#define VERB_MEM_STATS  4

typedef long long counter_t;

typedef struct dexe_s
{
  char **	argv;
  char **	arg1;
  char **	arg2;
} dexe_t;

#define OPT_MODE_OR_DEFAULT  ((unsigned)0)
#define OPT_MODE_AND_DEFAULT ((unsigned)~0)
typedef struct option_s
{
  unsigned int		 verbosityLevel:8;
  unsigned int		 dirs:1;
  unsigned int		 type:1;
  unsigned int		 mode:1;
  unsigned int		 uid:1;
  unsigned int		 gid:1;
  unsigned int		 nlink:1;
  unsigned int		 hardlinks:1;
  unsigned int		 mtime:1;
  unsigned int		 atime:1;
  unsigned int		 ctime:1;
  unsigned int		 size:1;
  unsigned int		 blocks:1;
  unsigned int		 contents:1;
  unsigned int		 major:1;
  unsigned int		 minor:1;
  unsigned int		 xattr:1;
  unsigned int		 flags:1;
  unsigned int		 acl:1;
  unsigned int		 mode_and;
  unsigned int		 mode_or;
  unsigned int		 exec:1;
  unsigned int		 exec_always:1;
  unsigned int           follow_symlinks:1;
  genhash_t*		 exclusions;
  dexe_t		 exec_args;
  dexe_t		 exec_always_args;
  size_t		 root1_length;
  size_t		 root2_length;
  ent_pair_cache_t	*inocache;
  hard_links_cache_t    *hardlinks1;
  hard_links_cache_t    *hardlinks2;
  str_list_t            *empty_str_list;
  struct
  {
    counter_t	singles;
    counter_t	compared;
    counter_t	contents_compared;
    counter_t	excluded;
    counter_t	skipped_same;
    counter_t	skipped_cache;
  } stats;

} options_t;

typedef struct str_list_client_data_s
{
  options_t *opts;
} str_list_client_data_t;


str_list_t *getDirList(const char* path, const struct stat*, int*);
int dodiff(options_t* opts, const char* p1, const char* p2);
char* pconcat(const char* p1, const char* p2);
int execprocess(const dexe_t *dex, const char* p1, const char* p2);
int dropAclXattrs(const char*);

#if DEBUG
void printopts(const options_t*);
#else
#  define printopts(a)
#endif

void
xperror(const char* msg, const char *filename)
{
  fprintf(stderr, "%s: %s: %s: %s (errno=%d)\n",
	  progname, filename, msg, strerror(errno), errno);
}

/*
 * Common printing
 */
static void
reportMissing(int which, const char* f, const char* what, str_list_client_data_t* clientData)
{
  const char*	subp;
  int		rootlen;
  /**/

  switch(which)
    {
    case 1:
      subp = f+(rootlen = clientData->opts->root1_length);
      break;
    case 2:
      subp = f+(rootlen = clientData->opts->root2_length);
      break;
    default:
      fprintf(stderr, "%s: reportMissing(): unexpected which=%d, aborting...\n",
	      progname, which);
      exit(XIT_INTERNALERROR);
    }
  if (*subp)
    {
      if (*subp != '/')
	{
	  fprintf(stderr, "%s: reportMissing(): unexpected subp=\"%c\" (code=%d), aborting...\n",
		  progname, isprint(*subp) ? *subp : '?', *subp);
	  exit(XIT_INTERNALERROR);
	}
      ++subp;
    }
  else
    {
      subp = "(top-level)";
    }
  printf("%s: %s: %s: only present in %.*s\n",
	 progname, subp, what, rootlen, f);
}

/*
 * Xattrs comparisons
 */
#if HAVE_LGETXATTR
str_list_t*
getXattrList(const char* path)
{
  str_list_t *rv;
  char *buf;
  size_t bufSize = XATTR_BUF_SIZE;
  ssize_t rSize;
  const char *p;
  const char *p_e;
  /**/

  buf = xmalloc(bufSize);

 again:
  rSize = llistxattr(path, buf, bufSize);
  if (rSize == -1)
    switch(errno)
      {
      case ENOTSUP:
	str_list_new(&rv);
	return rv;
      case ERANGE:
	free(buf);
	buf = xmalloc(bufSize *= 2);
	goto again;
      default:
	xperror("cannot get extended attribute list, llistxattr()", path);
	free(buf);
	return NULL;
      }

  str_list_new(&rv);
  for (p = buf, p_e = buf+rSize;
       p < p_e && *p;
       p += strlen(p)+1)
    str_list_push(rv, p);

  free(buf);
  return rv;
}

static int
reportMissingXattr(int which, const char* f, const char* xn, str_list_client_data_t* clientData)
{
  static const char prefix[] = "xattr ";
  size_t	 buf_len;
  char		*buf;

#if HAVE_ACL
  if (dropAclXattrs(xn))
    return XIT_OK;
#endif

  buf_len = sizeof(prefix)-1+strlen(xn)+1;
  buf = xmalloc(buf_len);
  snprintf(buf, buf_len, "%s%s", prefix, xn);
  reportMissing(which, f, buf, clientData);
  free(buf);

  return XIT_DIFF;
}

void*
getXattr(const char* p, const char* name, size_t *retSize)
{
  size_t	 bufSize = XATTR_BUF_SIZE;
  void		*buf	 = xmalloc(bufSize);
  ssize_t	 sz;
  /**/

  *retSize = 0;

 again:
  sz	 = lgetxattr(p, name, buf, bufSize);

  if (sz == -1)
    switch(errno)
      {
      case ERANGE:
	free(buf);
	buf = xmalloc(bufSize *= 2);
	goto again;
      default:
	free(buf);
	return 0;
      }

  *retSize = sz;
  return buf;
}

int
compareXattrs(const char* p1, const char* p2,
	      const char* e1, const char* e2,
	      str_list_client_data_t* clientData)
{
  int rv = XIT_OK;
  size_t sz1 = 0;
  size_t sz2 = 0;
  void* buf1;
  void* buf2;

#if HAVE_ACL
  if (dropAclXattrs(e1))
    return rv;
#endif

  buf1 = getXattr(p1, e1, &sz1);
  buf2 = getXattr(p2, e2, &sz2);

  if (!buf1)
    {
      BUMP_EXIT_CODE(rv, XIT_SYS);
      xperror("cannot get extended attribute list, lgetxattr()", p1);
      goto ret;
    }

  if (!buf2)
    {
      BUMP_EXIT_CODE(rv, XIT_SYS);
      xperror("cannot get extended attribute list, lgetxattr()", p2);
      goto ret;
    }

  if (sz1 != sz2 || memcmp(buf1, buf2, sz1))
    {
      printf("%s: %s: xattr %s: contents differ\n",
	     progname, p1+clientData->opts->root1_length+1, e1);
      BUMP_EXIT_CODE(rv, XIT_DIFF);
    }

 ret:
  if (buf1)
    free(buf1);
  if (buf2)
    free(buf2);
  return rv;
}

int
dropAclXattrs(const char *xn)
{
  return (!strcmp(xn, "system.posix_acl_access")
	  || !strcmp(xn, "system.posix_acl_default")
	  || !strcmp(xn, "trusted.SGI_ACL_FILE")
	  || !strcmp(xn, "trusted.SGI_ACL_DEFAULT"));
}
#endif

/*
 * ACL comparisons
 */
#if HAVE_ACL
static int
getAcl(const char* path, acl_type_t acltype, acl_t *pacl)
{
  acl_t acl = acl_get_file(path, acltype);
  if (acl == NULL)
    switch(errno)
      {
      case ENOTSUP:
      case EINVAL:
	*pacl = NULL;
	return 0;
      default:
	xperror("cannot get ACL, acl_get_file()", path);
	return XIT_SYS;
      }
  *pacl = acl;
  return 0;
}

str_list_t *
getAclList(const char* path, acl_t acl)
{
  ssize_t acllen;
  char *acls;
  char *p;
  str_list_t *rv;
  /**/

  str_list_new(&rv);
  if (acl == NULL) return rv;

  acls = acl_to_text(acl, &acllen);
  if (!acls)
    {
      xperror("cannot get ACL, acl_to_text()", path);
      return NULL;
    }

  const char* beg_user = acls;
  enum {
    STATE_FIRST_WS,
    STATE_USER,
    STATE_USER_FIRST_COLON,
    STATE_PERM1,
    STATE_PERM2,
    STATE_PERM3,
    STATE_LAST_WS,
    STATE_COMMENT
  } state = STATE_FIRST_WS;

  for (p = acls; p < acls+acllen; ++p)
    switch(state)
      {
      case STATE_FIRST_WS:
	if (!isspace(*p))
	  state = STATE_USER;
	beg_user = p;
	break;
      case STATE_USER:
      case STATE_USER_FIRST_COLON:
	if (*p == ':')
	  ++state;
	else if (isspace(*p))
	  {
	    fprintf(stderr, "%s: getAclList(): space-like character \"%c\" (code=%d) while parsing acl \"%s\", aborting...\n",
		    progname, isprint(*p) ? *p : '?', *p, p);
	    exit(XIT_INTERNALERROR);
	  }
	break;
      case STATE_PERM1:
      case STATE_PERM2:
      case STATE_PERM3:
	switch(*p)
	  {
	  case 'r':
	  case 'w':
	  case 'x':
	  case '-':
	    break;
	  default:
	    fprintf(stderr, "%s: getAclList(): unexpected character in state %d: \"%c\" (code=%d)\n",
		    progname, state, isprint(*p) ? *p : '?', *p);
	    exit(XIT_INTERNALERROR);
	  }
	if (state == STATE_PERM3)
	  {
	    char buf[p-beg_user+2];
	    memcpy(buf, beg_user, p-beg_user+1);
	    buf[p-beg_user+1] = 0;
	    buf[p-beg_user-3] = 0;
	    if (strcmp(buf, "user:")
		&& strcmp(buf, "group:")
		&& strcmp(buf, "other:"))
	      str_list_push_length(rv, buf, p-beg_user+2);
	  }
	state += 1;
	break;
      case STATE_LAST_WS:
	switch (*p)
	  {
	  case '#':
	    state = STATE_COMMENT;
	    break;
	  case '\n':
	    state = STATE_FIRST_WS;
	    break;
	  default:
	    if (!isspace(*p))
	      {
		fprintf(stderr, "%s: getAclList(): unexpected non-whitespace character in state %d: \"%c\" (code=%d), aborting...\n",
			progname, state, isprint(*p) ? *p : '?', *p);
		exit(XIT_INTERNALERROR);
	      }
	  }
	break;
      case STATE_COMMENT:
	if (*p == '\n')
	  state = STATE_FIRST_WS;
	break;
      default:
	fprintf(stderr, "%s: getAclList(): unexpected state %d, aborting...\n",
		progname, state);
	exit(XIT_INTERNALERROR);
      }

  if (state != STATE_FIRST_WS)
    {
      fprintf(stderr, "%s: getAclList(): unexpected state %d at end, aborting...\n",
	      progname, state);
      exit(XIT_INTERNALERROR);
    }

  acl_free(acls);

  return rv;
}

typedef struct aclCompareClientData_s
{
  str_list_client_data_t	cmn;
  const char*			acldescr;
} aclCompareClientData_t;

int
reportMissingAcl(int which, const char* f, const char* xn, str_list_client_data_t* commonClientData)
{
  aclCompareClientData_t*	 clientData = (aclCompareClientData_t*)commonClientData;
  static const char infix[]		    = " acl ";
  size_t			 buf_len;
  char				*buf;
  /**/

  buf_len = strlen(clientData->acldescr)+sizeof(infix)-1+strlen(xn)+1;
  buf = xmalloc(buf_len);
  snprintf(buf, buf_len, "%s%s%s", clientData->acldescr, infix, xn);
  reportMissing(which, f, buf, &clientData->cmn);
  free(buf);

  return XIT_DIFF;
}

int
compareAcls(const char* p1, const char* p2,
	    const char* e1, const char* e2,
	    str_list_client_data_t* commonClientData)
{
  aclCompareClientData_t* clientData = (aclCompareClientData_t*)commonClientData;
  int rv = XIT_OK;
  const char *v1;
  const char *v2;
  UNUSED(p2);

  v1 = e1 + strlen(e1)+1;
  v2 = e2 + strlen(e1)+1;

  if (strcmp(v1, v2))
    {
      printf("%s: %s: %s acl %s: %s %s\n",
	     progname, p1+clientData->cmn.opts->root1_length+1, clientData->acldescr, e1, v1, v2);
      BUMP_EXIT_CODE(rv, XIT_DIFF);
    }

  return rv;
}

int
diffacl(options_t* opts, const char* p1, const char* p2,
	acl_type_t acltype, const char* acldescr)
{
  acl_t				 acl1  = NULL;
  acl_t				 acl2  = NULL;
  str_list_t			*acls1 = NULL;
  str_list_t			*acls2 = NULL;
  int				 rv, rv2;
  aclCompareClientData_t	 clientData;
  /**/

  rv = getAcl(p1, acltype, &acl1);
  rv2 = getAcl(p2, acltype, &acl2);
  if (rv || rv2) {
    rv = rv || rv2;
    goto clean_acls;
  }

  if (acl1 == NULL && acl2 == NULL) return 0;

#if HAVE_ACL_CMP
  if (acl1 != NULL && acl2 != NULL)
    {
      int ret = acl_cmp(acl1, acl2);
      switch(ret)
	{
	case -1:
	  xperror("cannot compare ACLs, acl_cmp()", p1);
	  rv = XIT_SYS;
	  goto clean_acls;
	case 0:
	  rv = 0;
	  goto clean_acls;
	case 1:
	  break;
	default:
	  fprintf(stderr, "%s: %s: acl_cmp(): unexpected result %d\n",
		  progname, p1, ret);
	  return XIT_SYS;
	}
    }
#endif /* HAVE_ACL_CMP */

  acls1 = getAclList(p1, acl1);
  acls2 = getAclList(p2, acl2);

  clientData.cmn.opts = opts;
  clientData.acldescr = acldescr;

  rv = str_list_compare(p1, p2,
			acls1, acls2,
			reportMissingAcl,
			compareAcls,
			(str_list_client_data_t*)&clientData);

  str_list_destroy(acls1);
  str_list_destroy(acls2);

 clean_acls:
  if (acl1 != NULL) acl_free(acl1);
  if (acl2 != NULL) acl_free(acl2);

  return rv;
}

#endif /* HAVE_ACL */

/*
 * Utilities
 */
static int
openFile(const char* path, const struct stat *sbuf)
{
  int fd;
  int flags;
  /**/

  flags = O_RDONLY;
  switch((sbuf->st_mode) & S_IFMT)
    {
    case S_IFDIR:
      flags = O_RDONLY;
#if HAVE_O_DIRECTORY
      flags |= O_DIRECTORY;
#endif /* HAVE_O_DIRECTORY */
      break;
    case S_IFREG:
      flags = O_RDONLY;
      break;
#if HAVE_S_IFLNK && HAVE_O_PATH && HAVE_O_NOFOLLOW
    case S_IFLNK:
      flags = O_PATH|O_NOFOLLOW;
      break;
#endif /* HAVE_S_IFLNK && HAVE_O_PATH && HAVE_O_NOFOLLOW */
    default:
#if HAVE_O_PATH
      flags = O_PATH;
#else /* ! HAVE_O_PATH */
      flags = O_RDONLY;
#endif /* ! HAVE_O_PATH */
      break;
    }
#if HAVE_O_NOATIME
  flags |= O_NOATIME;
#endif /* HAVE_O_NOATIME */

  fd = open(path, flags);

#if HAVE_O_NOATIME
  if (fd < 0 && errno == EPERM)
    fd = open(path, flags & ~O_NOATIME);
#endif

  return fd;
}

/*
 * Directory comparisons
 */
str_list_t *
getDirList(const char* path, const struct stat *sbuf, int *fd)
#if HAVE_GETDENTS
{
  char		dentbuf[GETDIRLIST_DENTBUF_SIZE];
  str_list_t *	rv = NULL;
  int		nread;
  /**/

  if (*fd < 0)
    *fd = openFile(path, sbuf);
  if (*fd < 0)
    goto err_open;

  str_list_new(&rv);

  while((nread = getdents(*fd, (void*)dentbuf,
			  GETDIRLIST_DENTBUF_SIZE))>0)
    {
      struct STRUCT_DIRENT *dent;
      /**/
      for (dent = (struct STRUCT_DIRENT*) dentbuf;
	   (char*)dent - dentbuf < nread;
	   dent = (struct STRUCT_DIRENT*) ( (char*)dent + dent->d_reclen ))
	{
	  if ( (dent->d_name[0]=='.'
		&& (dent->d_name[1] == 0
		    || (dent->d_name[1]=='.'
			&& dent->d_name[2]== 0))))
	    continue;

	  str_list_push(rv, dent->d_name);
	}
    }

  if (nread<0)
    goto err_dents;

  return rv;

 err_dents:
  str_list_destroy(rv);
  rv = NULL;

 err_open:
  xperror("cannot get directory listing, getdents()", path);
  return rv;
}
#else /* ! HAVE_GETDENTS */
{
#if HAVE_FDOPENDIR
  int           dupfd;
#endif /* HAVE_FDOPENDIR */
  DIR		*dir;
  struct dirent *dent;
  str_list_t	*rv = NULL;
  /**/

#if HAVE_FDOPENDIR
  if (*fd < 0)
    *fd = openFile(path, sbuf);

  if ( *fd < 0 )
    goto err;

  dupfd = dup(*fd);
  if (dupfd < 0)
    goto err;

  dir = fdopendir(dupfd);
  if (!dir)
    {
      close(dupfd);
      goto err;
    }
#else /* ! HAVE_FDOPENDIR */
  UNUSED(sbuf);
  UNUSED(fd);
  dir = opendir(path);
  if (!dir)
    goto err;
#endif /* ! HAVE_FDOPENDIR */

  str_list_new(&rv);

  while ((dent = readdir(dir)))
    {
      if (dent->d_name[0]=='.'
	  && (dent->d_name[1] == 0
	      || (dent->d_name[1]=='.'
		  && dent->d_name[2]== 0)))
	continue;

      str_list_push(rv, dent->d_name);
    }

  if (closedir(dir))
    {
      str_list_destroy(rv);
      rv = NULL;
      goto err;
    }

  return rv;

 err:
  xperror("cannot get directory listing, readdir()", path);
  return rv;
}
#endif  /* ! HAVE_GETDENTS */

static int
reportMissingFile(int which, const char* d, const char *f, str_list_client_data_t* clientData)
{
  if ( ! clientData->opts->dirs )
    return XIT_OK;

  if ( gh_find(clientData->opts->exclusions, f, NULL) )
    ++ clientData->opts->stats.excluded;
  else
    {
      const char*	subp;
      int		rootlen;
      size_t		fp_len = strlen(f)+strlen(d)+1;
      char		fp[fp_len];

      ++ clientData->opts->stats.singles;

      switch(which)
	{
	case 1:
	  subp = d+(rootlen = clientData->opts->root1_length);
	  break;
	case 2:
	  subp = d+(rootlen = clientData->opts->root2_length);
	  break;
	default:
	  fprintf(stderr, "%s: reportMissingFile(): unexpected which=%d, aborting...\n",
		  progname, which);
	  exit(XIT_INTERNALERROR);
	}
      if (*subp)
	{
	  if (*subp != '/')
	    {
	      fprintf(stderr, "%s: reportMissingFile(): unexpected subp=\"%c\" (code=%d), aborting...\n",
		      progname, isprint(*subp) ? *subp : '?', *subp);
	      exit(XIT_INTERNALERROR);
	    }
	  ++subp;
	}
      else
	{
	  subp = "";
	}
      snprintf(fp, fp_len, "%s%s%s", subp, *subp ? "/" : "", f);
      printf("%s: %s: only present in %.*s\n",
	     progname, fp, rootlen, d);
    }
  return XIT_DIFF;
}

static int
compareFileEntries(const char* p1, const char* p2,
		   const char* e1, const char* e2,
		   str_list_client_data_t* clientData)
{
  int rv = XIT_OK;
  UNUSED(e2);
  /**/

  if (gh_find(clientData->opts->exclusions, e1, NULL))
    ++ clientData->opts->stats.excluded;
  else
    {
      char *np1 = pconcat(p1, e1);
      char *np2 = pconcat(p2, e1);
      rv = dodiff(clientData->opts, np1, np2);
      free(np1);
      free(np2);
    }
  return rv;
}

/*
 * Hard links comparisons
 */
static int
reportMissingHardLink(int which, const char* d, const char *f, str_list_client_data_t* clientData)
{
  static const char prefix[] = "hard link to ";
  size_t	 buf_len;
  char		*buf;

  buf_len = sizeof(prefix)-1+strlen(f)+1;
  buf = xmalloc(buf_len);
  snprintf(buf, buf_len, "%s%s", prefix, f);
  reportMissing(which, d, buf, clientData);
  free(buf);

  return XIT_DIFF;
}

/*
 * Reporting utilities
 */
const char*
getFileType(mode_t m)
{
  switch(m & S_IFMT)
    {
    case S_IFDIR:  return "directory";
    case S_IFREG:  return "regular-file";
    case S_IFCHR:  return "character-device";
    case S_IFBLK:  return "block-device";
#if HAVE_S_IFIFO
    case S_IFIFO:  return "fifo";
#endif
#if HAVE_S_IFLNK
    case S_IFLNK:  return "symbolic-link";
#endif
#if HAVE_S_IFSOCK
    case S_IFSOCK: return "socket";
#endif
#if HAVE_S_IFDOOR
    case S_IFDOOR: return "door";
#endif
#if HAVE_S_IFWHT
    case S_IFWHT:  return "whiteout";
#endif
    default:       return "unknown";
    }
}

static void
formatTime(char* obuf, size_t obufsize, time_t fsecs, int fnsecs)
{
  size_t written;
  struct tm tms;
  /**/
  localtime_r(&fsecs, &tms);
  written = strftime(obuf, obufsize, "%Y-%m-%d %H:%M:%S", &tms);
  if (fnsecs >= 0 && written < obufsize)
    snprintf(obuf+written, obufsize-written, ".%09d", fnsecs);
}


static void
reportTimeDiscrepancy(const char* f, const char* whattime,
		      time_t f1secs, time_t f2secs,
		      int f1nsecs, int f2nsecs)
{
  char t1[256];
  char t2[256];
  /**/

  formatTime(t1, sizeof(t1), f1secs, f1nsecs);
  formatTime(t2, sizeof(t2), f2secs, f2nsecs);

  printf("%s: %s: %s: [%s] [%s]\n",
	 progname, f, whattime, t1, t2);
}

static int
cmpFiles(const char* f1, const struct stat *sbuf1, int *fd1,
	 const char* f2, const struct stat *sbuf2, int *fd2)
{
  char		buf1[CMPFILE_BUF_SIZE];
  char		buf2[CMPFILE_BUF_SIZE];
  ssize_t	toread;
  int		rv = XIT_OK;
  /**/

  if (sbuf1->st_size != sbuf2->st_size)
    {
      fprintf(stderr, "%s: cmpFiles called on files of different sizes\n",
	      progname);
      BUMP_EXIT_CODE(rv, XIT_INTERNALERROR);
      return rv;
    }

  if (sbuf1->st_size == 0)
    {
      return rv;
    }

  if (*fd1 < 0)
    *fd1 = openFile(f1, sbuf1);

  if (*fd1 < 0)
    {
      xperror("cannot open file", f1);
      BUMP_EXIT_CODE(rv, XIT_SYS);
      return rv;
    }

  if (*fd2 < 0)
    *fd2 = openFile(f2, sbuf2);

  if (*fd2 < 0)
    {
      xperror("cannot open file", f2);
      BUMP_EXIT_CODE(rv, XIT_SYS);
      return rv;
    }

  for (toread = sbuf1->st_size; toread>0; )
    {
      int nread1, nread2;
      /**/
      nread1 = read(*fd1, buf1, CMPFILE_BUF_SIZE);
      if (nread1 < 0)
	{
	  xperror("read()", f1);
	  BUMP_EXIT_CODE(rv, XIT_SYS);
	  return rv;
	}
      if (nread1 < CMPFILE_BUF_SIZE && nread1 != toread)
	{
	  fprintf(stderr, "%s: %s: short read\n", progname, f1);
	  BUMP_EXIT_CODE(rv, XIT_SYS);
	  return rv;
	}
      nread2 = read(*fd2, buf2, CMPFILE_BUF_SIZE);
      if (nread2 < 0)
	{
	  xperror("read()", f2);
	  BUMP_EXIT_CODE(rv, XIT_SYS);
	  return rv;
	}
      if (nread2 < CMPFILE_BUF_SIZE && nread2 != toread)
	{
	  fprintf(stderr, "%s: %s: short read\n", progname, f2);
	  BUMP_EXIT_CODE(rv, XIT_SYS);
	  return rv;
	}
      if (nread1 != nread2)
	{
	  fprintf(stderr, "%s: %s, %s: read different number of bytes (%d vs. %d)\n",
		  progname, f1, f2, (int)nread1, (int)nread2);
	  BUMP_EXIT_CODE(rv, XIT_SYS);
	  return rv;
	}
      if (memcmp(buf1, buf2, nread1)!=0)
	{
	  BUMP_EXIT_CODE(rv, XIT_DIFF);
	  break;
	}
      toread -= nread1;
    }

  return rv;
}

/*
 * Options handling
 */

unsigned
get_terminal_width(FILE *fd)
{
  static const int	 default_terminal_width = 80;
  int			 fno;
  char			*width_string;
  struct winsize	 ws;

  fno = fileno(fd);

  if (! isatty(fno))
    return default_terminal_width;

  width_string = getenv("COLUMNS");
  if (width_string != NULL)
    {
      unsigned long width;
      char *ptr;

      errno = 0;
      width = strtoul(width_string, &ptr, 0);
      if (*ptr == '\0' && width > 0 && errno == 0 && width < UINT_MAX )
	return (unsigned)width;
    }

  if (ioctl(fileno(fd), TIOCGWINSZ, &ws) != -1 && ws.ws_col > 0)
    return ws.ws_col;

  return default_terminal_width;
}

void
print_list(FILE* fd, unsigned width, const char *header, ...)
{
  size_t	 header_length;
  size_t	 printed_so_far;
  va_list	 va;
  const char	*item;
  size_t	 i;

  header_length = strlen(header);

  va_start(va, header);
  printed_so_far = 0;

  while ((item = va_arg(va, const char*)))
    {
      size_t item_length = strlen(item);

      if (printed_so_far == 0)
	{
	  fprintf(fd, "%s%s", header, item);
	  printed_so_far = header_length + item_length;
	}
      else if (printed_so_far + item_length + 3 > width)
	// 2 for ", " and 1 more for the trailing "," or "." that will
	// be added later.
	{
	  fprintf(fd, ",\n");
	  for (i=0; i<header_length; ++i)
	    putc(' ', fd);
	  fprintf(fd, "%s", item);
	  printed_so_far = header_length + item_length;
	}
      else
	{
	  fprintf(fd, ", %s", item);
	  printed_so_far += item_length + 2;
	}
    }

  va_end(va);

  if (printed_so_far != 0)
    fprintf(fd, ".\n");
}

void
show_version(void)
{
  unsigned terminal_width = get_terminal_width(stdout);

  printf("tdiff version " VERSION
#ifdef GIT_REVISION
	 " (git: " GIT_REVISION ")"
#endif
	 "\n");


  print_list(stdout, terminal_width,
	     "Features: ",
#if HAVE_ACL
	     "acl=yes",
#  if HAVE_ACL_CMP
	     "acl_cmp=yes",
#  else /* ! HAVE_ACL_CMP */
	     "acl_cmp=no",
#  endif /* ! HAVE_ACL_CMP */
#else /* ! HAVE_ACL */
	     "acl=no",
	     "acl_cmp=no",
#endif /* ! HAVE_ACL */
#if HAVE_ST_FLAGS
	     "flags=BSD",
#else /* ! HAVE_ST_FLAGS */
	     "flags=no",
#endif /* ! HAVE_ST_FLAGS */
#if HAVE_O_NOATIME
	     "O_NOATIME=yes",
#else /* ! HAVE_O_NOATIME */
	     "O_NOATIME=no",
#endif /* ! HAVE_O_NOATIME */
#if HAVE_GETDENTS
#  if HAVE_SYS_DIRENT_H
	     "readdir=getdents(libc)",
#  elif HAVE_GETDENTS64_SYSCALL_H
	     "readdir=getdents64(syscall)",
#  elif HAVE_GETDENTS_SYSCALL_H
	     "readdir=getdents(syscall)",
#  else
#    error HAVE_GETDENTS is set, but I do not know how to get it !!!
#  endif
#else /* ! HAVE_GETDENTS */
	     "readdir=readdir",
#endif /* ! HAVE_GETDENTS */
#if HAVE_READLINKAT && HAVE_O_PATH && HAVE_O_NOFOLLOW && HAVE_O_NOATIME
	     "readlink=open(O_PATH|O_NOFOLLOW)+readlinkat",
#else /* ! (HAVE_READLINKAT && HAVE_O_PATH && HAVE_O_NOFOLLOW && HAVE_O_NOATIME) */
	     "readlink=readlink",
#endif /* ! (HAVE_READLINKAT && HAVE_O_PATH && HAVE_O_NOFOLLOW && HAVE_O_NOATIME) */
#ifdef ST_ATIMENSEC
	     "time_granularity=ns",
#else /* ! def ST_ATIMENSEC */
	     "time_granularity=s",
#endif /* ! def ST_ATIMENSEC */
#if HAVE_LGETXATTR
	     "xattr=yes",
#else /* ! HAVE_LGETXATTR */
	     "xattr=no",
#endif /* ! HAVE_LGETXATTR */
	     NULL);

#if HAVE_ST_FLAGS
  print_list(stdout, terminal_width,
	     "Flags: ",
# if HAVE_UF_NODUMP
	     "UF_NODUMP (nodump)",
# endif
# if HAVE_UF_IMMUTABLE
	     "UF_IMMUTABLE (uimmutable)",
# endif
# if HAVE_UF_APPEND
	     "UF_APPEND (uappend)",
# endif
# if HAVE_UF_OPAQUE
	     "UF_OPAQUE (opaque)",
# endif
# if HAVE_UF_NOUNLINK
	     "UF_NOUNLINK (uunlink)",
# endif
# if HAVE_UF_COMPRESSED
	     "UF_COMPRESSED (compressed)",
# endif
# if HAVE_UF_TRACKED
	     "UF_TRACKED (tracked)",
# endif
# if HAVE_UF_SYSTEM
	     "UF_SYSTEM (system)",
# endif
# if HAVE_UF_SPARSE
	     "UF_SPARSE (sparse)",
# endif
# if HAVE_UF_OFFLINE
	     "UF_OFFLINE (offline)",
# endif
# if HAVE_UF_REPARSE
	     "UF_REPARSE (reparse)",
# endif
# if HAVE_UF_ARCHIVE
	     "UF_ARCHIVE (uarchive)",
# endif
# if HAVE_UF_READONLY
	     "UF_READONLY (readonly)",
# endif
# if HAVE_UF_HIDDEN
	     "UF_HIDDEN (hidden)",
# endif
# if HAVE_SF_ARCHIVED
	     "SF_ARCHIVED (archived)",
# endif
# if HAVE_SF_IMMUTABLE
	     "SF_IMMUTABLE (simmutable)",
# endif
# if HAVE_SF_APPEND
	     "SF_APPEND (sappend)",
# endif
# if HAVE_SF_NOUNLINK
	     "SF_NOUNLINK (sunlink)",
# endif
# if HAVE_SF_SNAPSHOT
	     "SF_SNAPSHOT (snapshot)",
# endif
	     NULL);
#endif /* HAVE_ST_FLAGS */

  printf("\n"
	 "Copyright (C) 1999-2024 Philippe Troin <phil+github-commits@fifi.org>.\n"
	 "Tdiff comes with ABSOLUTELY NO WARRANTY.\n"
	 "This is free software, and you are welcome to redistribute it\n"
	 "under certain conditions.\n"
	 "You should have received a copy of the GNU General Public License\n"
	 "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n");
}

void
show_help(void)
{
  printf("usage: %s [options]... <dir1> <dir2>\n"
	 " Standard options:\n"
	 "   -v --verbose  increase verbosity level, can be used more than once:\n"
	 "        -v     report overall statistics\n"
	 "        -vv    report on skipped files\n"
	 "        -vvv   internal data structures statistics\n"
	 "        -vvvv  memory statistics\n"
	 "   -V --version  show %s version\n"
	 "   -h --help     show brief help message\n"
	 " Toggle options:\n"
	 "   -d --dirs      diff directories (reports missing files)\n"
	 "   -t --type      diffs for file type differences\n"
	 "                  (if unset, then size, blocks, major, minor and contents\n"
	 "                   are not checked either)\n"
	 "   -m --mode      diffs file modes (permissions)\n"
	 "   -u --uid       diffs file user id\n"
	 "   -g --gid       diffs file group id\n"
	 "   -n --nlink     diffs the (hard) link count\n"
	 "   -e --hardlinks diffs the hard link targets\n"
	 "   -i --mtime     diffs mtime (contents modification time)\n"
	 "   -y --atime     diffs atime (access time)\n"
	 "   -z --ctime     diffs ctime (inode modification time)\n"
	 "   -s --size      diffs file size (for regular files, symlinks)\n"
	 "   -b --blocks    diffs file blocks (for regular files, symlinks & directories)\n"
	 "   -c --contents  diffs file contents (for regular files and symlinks)\n"
	 "   -j --major     diffs major device numbers (for device files)\n"
	 "   -k --minor     diffs minor device numbers (for device files)\n"
#if HAVE_ST_FLAGS
	 "   -f --flags     diffs flags (4.4BSD)\n"
#endif
#if HAVE_ACL
	 "   -l --acl       diffs file ACLs\n"
#endif
#if HAVE_LGETXATTR
	 "   -q --xattr     diffs file extended attributes\n"
#endif
	 "  Each of these options can be negated with an uppercase (short option)\n"
	 "  or with --no-option (eg -M --no-mode for not diffing modes)\n"
	 " Presets (cumulative, -2 implies -1, etc.):\n"
	 "   -0 -p|--preset 0|none       No checks are made.         (-DTMUGNEIYZSBCJK"
#if HAVE_ST_FLAGS
	 "F"
#endif
#if HAVE_ACL
	 "L"
#endif
#if HAVE_LGETXATTR
	 "Q"
#endif
	 ")\n"
	 "   -1 -p|--preset 1|missing    Missing files are reported. (-dtMUGNEIYZSBCJK"
#if HAVE_ST_FLAGS
	 "F"
#endif
#if HAVE_ACL
	 "L"
#endif
#if HAVE_LGETXATTR
	 "Q"
#endif
	 ")\n"
	 "   -2 -p|--preset 2|mode       Add modes.                  (-dtmUGNEIYZSBCJK"
#if HAVE_ST_FLAGS
	 "F"
#endif
#if HAVE_ACL
	 "L"
#endif
#if HAVE_LGETXATTR
	 "Q"
#endif
	 ")\n"
	 "   -3 -p|--preset 3|owner      Add uid/gid ownership"
#if HAVE_ACL
	 ", ACLs."
#else
	 ".      "
#endif
	 "(-dtmugNEIYZSBCJK"
#if HAVE_ST_FLAGS
	 "F"
#endif
#if HAVE_ACL
	 "l"
#endif
#if HAVE_LGETXATTR
	 "Q"
#endif
	 ")\n"
	 "   -4 -p|--preset 4|hardlinks  Add hardlinks.              (-dtmugneIYZSBCJK"
#if HAVE_ST_FLAGS
	 "F"
#endif
#if HAVE_ACL
	 "l"
#endif
#if HAVE_LGETXATTR
	 "Q"
#endif
	 ")\n"
	 "   -5 -p|--preset 5|contents   Add size, contents, blocks,\n"
	 "                               major and minor.            (-dtmugneIYZsbcjk"
#if HAVE_ST_FLAGS
	 "F"
#endif
#if HAVE_ACL
	 "l"
#endif
#if HAVE_LGETXATTR
	 "Q"
#endif
	 ")\n"
	 "   -6 -p|--preset 6|notimes    "
#if HAVE_ST_FLAGS || HAVE_LGETXATTR
	 "Add %-13s (default) (-dtmugneIYZsbcjk"
# if HAVE_ST_FLAGS
	 "f"
# endif
# if HAVE_ACL
	 "l"
# endif
# if HAVE_LGETXATTR
	 "q"
# endif
	 ")"
#else /* ! HAVE_ST_FLAGS && ! HAVE_LGETXATTR */
	 "Same as preset 5 or contents."
#endif
	 "\n"
	 "   -7 -p|--preset 7|mtime      Add mtime.                  (-dtmugneiYZsbcjk"
#if HAVE_ST_FLAGS
	 "f"
#endif
#if HAVE_ACL
	 "l"
#endif
#if HAVE_LGETXATTR
	 "q"
#endif
	 ")\n"
	 "   -8 -p|--preset 8|amtimes    Add atime.                  (-dtmugneiyZsbcjk"
#if HAVE_ST_FLAGS
	 "f"
#endif
#if HAVE_ACL
	 "l"
#endif
#if HAVE_LGETXATTR
	 "q"
#endif
	 ")\n"
	 "   -9 -p|--preset 9|alltimes   Add ctime.                  (-dtmugneiyzsbcjk"
#if HAVE_ST_FLAGS
	 "f"
#endif
#if HAVE_ACL
	 "l"
#endif
#if HAVE_LGETXATTR
	 "q"
#endif
	 ")\n"
	 " Miscellania:\n"
	 "   -a --mode-and <bits>       applies <bits> AND mode before comparison\n"
	 "   -o --mode-or <bits>        applies <bits> OR mode before comparison\n"
	 "   -w --exec-always <cmd> \\;  always executes <cmd> for every regular file pair\n"
	 "                              <cmd> uses %%1 and %%2 as placeholders for files\n"
	 "                              from <dir1> and <dir2>.\n"
	 "   -W --exec-always-diff      always executes \"diff -u\" for every reg. file pair\n"
	 "                              equivalent to: -w diff -u %%1 %%2 \\;\n"
	 "   -x --exec <cmd> \\;         executes <cmd> for every reg. file pair having the\n"
	 "                              same size to determine if their contents are equal\n"
	 "   -X --exclude <file>        omits <file> from report\n"
	 "   -O --follow-symlinks       always dereference symbolic links\n"
#if ! HAVE_GETOPT_LONG
	 "WARNING: your system does not have getopt_long (use a GNU system !)\n"
#endif
	 ,progname, progname
#if HAVE_ST_FLAGS || HAVE_LGETXATTR
	 ,
	 ""
# if HAVE_ST_FLAGS
	 "flags"
# endif
# if HAVE_LGETXATTR
#   if HAVE_ST_FLAGS
	 ", "
#   endif
	 "xattrs"
# endif
	 "."
#endif
	 );
}

static void
get_exec_args(char **argv, int *optind, dexe_t *dex)
{
  char **cargv;
  int found_semi = 0;
  char ** found_f1 = NULL;
  char ** found_f2 = NULL;
  int numargs;
  int i;
  /**/

  argv += *optind;
  cargv = argv;
  while(1)
    {
      if (!*cargv)
	break;
      if (strcmp("%1", *cargv)==0)
	found_f1 = cargv;
      else if (strcmp("%2", *cargv)==0)
	found_f2 = cargv;
      else if (strcmp(";", *cargv)==0)
	{
	  found_semi=1;
	  break;
	}
      ++cargv;
      (*optind)++;
    }
  (*optind)++;
  if (!found_semi)
    {
      fprintf(stderr,
	      "%s: missing final semi-colon while scanning command line\n",
	      progname);
      exit(XIT_INVOC);
    }
  numargs = cargv-argv;
  if (dex->argv != NULL)
    {
      free(dex->argv);
    }
  dex->argv = xmalloc(sizeof(char*)*(numargs+1));
  dex->arg1 = NULL;
  dex->arg2 = NULL;
  for (i=0; i<numargs; i++)
    if (&argv[i] == found_f1)
      {
	dex->argv[i] = NULL;
	dex->arg1 = &dex->argv[i];
      }
    else if(&argv[i] == found_f2)
      {
	dex->argv[i] = NULL;
	dex->arg2 = &dex->argv[i];
      }
    else
      {
	dex->argv[i] = argv[i];
      }
  dex->argv[numargs] = NULL;
}

int
get_octal_arg(const char* string, unsigned int* val)
{
  size_t	 len;
  char		*ptr;
  unsigned long	 m;
  const char	*num_descr;

  len = strlen(string);
  if (len > 2 && string[0]=='0' && string[1]=='x')
    {
      m = strtoul(&string[2], &ptr, 16);
      num_descr = "hexadecimal";
    }
  else
    {
      m = strtoul(string, &ptr, 8);
      num_descr = "octal";
    }

  if (*ptr)
    {
      fprintf(stderr, "%s: invalid %s number \"%s\"\n", progname, num_descr, string);
      return 0;
    }
  else
    {
      if (m > 07777)
	fprintf(stderr,
		"%s: file mode \"%s\" too big, should be in the 0-7777 range (octal)\n",
		progname, string);

      *val = m;
      return 1;
    }
}

#if DEBUG
void
printopts(const options_t* o)
{
#define POPT(x) printf(#x " = %s\n", o->x ? "yes" : "no")
  printf("verbosityLevel = %d\n", o->verbosityLevel);
  POPT(follow_symlinks);
  POPT(dirs);
  POPT(type);
  POPT(mode);
  POPT(uid);
  POPT(gid);
  POPT(nlink);
  POPT(hardlinks);
  POPT(mtime);
  POPT(atime);
  POPT(ctime);
  POPT(size);
  POPT(blocks);
  POPT(contents);
  POPT(major);
  POPT(minor);
  POPT(flags);
  POPT(acl);
  POPT(xattr);
  printf("mode AND = %04o\n", o->mode_and);
  printf("mode OR = %04o\n", o->mode_or);
  POPT(exec_always);
  POPT(exec);
#undef POPT
  if (o->exec_always)
    {
      char **c = o->exec_always_args.argv;
      /**/

      if (o->exec_always_args.arg1) *(o->exec_always_args.arg1) = "<arg1>";
      if (o->exec_always_args.arg2) *(o->exec_always_args.arg2) = "<arg2>";
      printf("Exec always cmd line:");
      while(*c)
	printf(" <%s>", *c++);
      printf("\n");
    }
  if (o->exec)
    {
      char **c = o->exec_args.argv;
      /**/

      if (o->exec_args.arg1) *(o->exec_args.arg1) = "<arg1>";
      if (o->exec_args.arg2) *(o->exec_args.arg2) = "<arg2>";
      printf("Exec cmd line:");
      while(*c)
	printf(" <%s>", *c++);
      printf("\n");
    }
}
#endif /* DEBUG */

char*
pconcat(const char* p1, const char* p2)
{
  char *rv;
  int p1_length;
  /**/

  rv = xmalloc((p1_length=strlen(p1))+1+strlen(p2)+1);
  strcpy(rv, p1);
  rv[p1_length] = '/';
  strcpy(rv+p1_length+1, p2);
  return rv;
}

int
execprocess(const dexe_t *dex, const char* p1, const char* p2)
{
  int status;
  /**/
  if (dex->arg1 != NULL)
    *(dex->arg1) = (char*)p1;
  if (dex->arg2 != NULL)
    *(dex->arg2) = (char*)p2;
  fflush(stdin);
  fflush(stdout);
  fflush(stderr);
  switch(fork())
    {
    case -1:
      /* Failure */
      fprintf(stderr, "%s: fork(): %s\n", progname, strerror(errno));
      return 0;
    case 0:
      /* Child */
      if (execvp(dex->argv[0], dex->argv)<0)
	{
	  fprintf(stderr, "%s: exec(%s): %s\n", progname, dex->argv[0],
		  strerror(errno));
	  exit(1);
	}
      fprintf(stderr, "%s: exec returned positive value\n", progname);
      exit(XIT_INTERNALERROR);
    default:
      /* Parent */
      if (wait(&status)<0)
	{
	  fprintf(stderr, "%s: wait(): %s\n", progname, strerror(errno));
	  return 0;
	}
      return (WIFEXITED(status) && WEXITSTATUS(status)==0) ? 1 : 0;
    }
}

#if HAVE_S_IFLNK
char*
xreadlink(const char* path, const struct stat *sbuf, int *fd)
{
  char *buf;
  int bufsize = XREADLINK_BUF_SIZE;
  int nstored;
#if HAVE_READLINKAT && HAVE_O_PATH && HAVE_O_NOFOLLOW && HAVE_O_NOATIME
  /**/
  if (*fd < 0)
    *fd = openFile(path, sbuf);
#else /* ! (HAVE_READLINKAT && HAVE_O_PATH && HAVE_O_NOFOLLOW && HAVE_O_NOATIME) */
  UNUSED(sbuf);
  UNUSED(fd);
#endif /* ! (HAVE_READLINKAT && HAVE_O_PATH && HAVE_O_NOFOLLOW && HAVE_O_NOATIME) */
  buf = xmalloc(bufsize);

 again:
#if HAVE_READLINKAT && HAVE_O_PATH && HAVE_O_NOFOLLOW && HAVE_O_NOATIME
  if (*fd >= 0)
    nstored = readlinkat(*fd, "", buf, bufsize);
  if (*fd < 0 || nstored < 0)
#endif /* HAVE_READLINKAT && HAVE_O_PATH && HAVE_O_NOFOLLOW && HAVE_O_NOATIME */
    nstored = readlink(path, buf, bufsize);
  if (nstored < 0)
    {
      xperror("cannot read link target, readlink()", path);
      free(buf);
      return NULL;
    }
  if (nstored >= bufsize)
    {
      buf = xrealloc(buf, bufsize*=2);
      goto again;
    }
  buf[nstored] = 0;
  return buf;
}
#endif /* HAVE_S_IFLNK */

int
dodiff(options_t* opts, const char* p1, const char* p2)
{
  int                         (*stat_func)(const char*, struct stat*);
  const char*			stat_func_name;
  struct stat			sbuf1;
  struct stat			sbuf2;
  int                           fd1 = -1;
  int                           fd2 = -1;
  ent_pair_cache_key_t *	ice;
  char *			icepath;
  int				rv	     = XIT_OK;
  const char *			subpath;
  int				content_diff = 1;
  /**/

  ++ opts->stats.compared;

  /* Stat the paths */
  if (opts->follow_symlinks)
    {
      stat_func	     = stat;
      stat_func_name = "stat";
    }
  else
    {
      stat_func	     = lstat;
      stat_func_name = "lstat";
    }

  if (stat_func(p1, &sbuf1)<0)
    {
      fprintf(stderr, "%s: %s: cannot get inode status, %s(): %s (errno=%d)\n",
	      progname, p1, stat_func_name, strerror(errno), errno);
      BUMP_EXIT_CODE(rv, XIT_SYS);
    }
  if (stat_func(p2, &sbuf2)<0)
    {
      fprintf(stderr, "%s: %s: cannot get inode status, %s(): %s (errno=%d)\n",
	      progname, p2, stat_func_name, strerror(errno), errno);
      BUMP_EXIT_CODE(rv, XIT_SYS);
    }
  if (rv != XIT_OK)
    return rv;

  subpath = p1+opts->root1_length;
  switch(*subpath)
    {
    case '\0': subpath = "(top-level)"; break;
    case '/':  subpath++;               break;
    default:
      fprintf(stderr, "%s: dodiff(): unexpected *subpath=\"%c\" (code=%d), aborting...\n",
	      progname, isprint(*subpath) ? *subpath : '?', *subpath);
      exit(XIT_INTERNALERROR);
    }

  /* Check if we're comparing the same dev/inode pair */
  if (sbuf1.st_ino == sbuf2.st_ino && sbuf1.st_dev == sbuf2.st_dev)
    {
      ++ opts->stats.skipped_same;
      if (opts->verbosityLevel >= VERB_SKIPS)
	fprintf(stderr, "%s: %s: same dev/ino pair, skipping\n",
	       progname, subpath);
      return rv;
    }

  /* Check if we have compared these two guys before */
  ice = xmalloc(sizeof(ent_pair_cache_key_t));
  memset(ice, 0, sizeof(*ice));
  ice->ent1.ino = sbuf1.st_ino;
  ice->ent1.dev = sbuf1.st_dev;
  ice->ent2.ino = sbuf2.st_ino;
  ice->ent2.dev = sbuf2.st_dev;
  icepath = xstrdup(subpath);
  if (! epc_put(opts->inocache, ice, icepath))
    {
      ++ opts->stats.skipped_cache;
      if (opts->verbosityLevel >= VERB_SKIPS)
	{
	  const char* ptr;
	  ptr = epc_get(opts->inocache, ice);
	  fprintf(stderr, "%s: %s: already compared %slinked files at %s\n",
		  progname, icepath, opts->follow_symlinks ? "" : "hard-", ptr);
	}
      free(ice);
      free(icepath);
      return rv;
    }

  /* Hard links handling */
  if (opts->hardlinks)
    {
      int			 store1, store2;
      hard_links_cache_key_t	 k1, k2;
      hard_links_cache_val_t	*v1 = NULL, *v2 = NULL;

      k1.dev = sbuf1.st_dev;
      k1.ino = sbuf1.st_ino;
      if (sbuf1.st_nlink > 1 && ((sbuf1.st_mode)&S_IFMT) != S_IFDIR)
	{
	  v1 = hc_get(opts->hardlinks1, &k1);
	  store1 = 1;
	}
      else
	store1 = 0;

      k2.dev = sbuf2.st_dev;
      k2.ino = sbuf2.st_ino;
      if (sbuf2.st_nlink > 1 && ((sbuf2.st_mode)&S_IFMT) != S_IFDIR)
	{
	  v2 = hc_get(opts->hardlinks2, &k2);
	  store2 = 1;
	}
      else
	store2 = 0;

      if (v1 != NULL || v2 != NULL)
	{
	  str_list_client_data_t	clientData;
	  int				nrv;

	  if ((v1 == NULL || v2 == NULL) && opts->empty_str_list == NULL)
	    str_list_new_size(&opts->empty_str_list, 0);

	  clientData.opts = opts;
	  nrv = str_list_compare(p1, p2,
				 v1 != NULL ? v1 : opts->empty_str_list,
				 v2 != NULL ? v2 : opts->empty_str_list,
				 &reportMissingHardLink,
				 NULL,
				 &clientData);
	  BUMP_EXIT_CODE(rv, nrv);
	}

      if (store1)
	hc_add_hard_link(opts->hardlinks1, &k1, v1, subpath);

      if (store2)
	hc_add_hard_link(opts->hardlinks2, &k2, v2, subpath);
    }

  /* Generic perms, uid, etc... */
  if (opts->mode)
    {
      int rm1, rm2;
      int mm1, mm2;
      /**/

      /* Note that we mask the symlink modes... because of 4.4BSD */
      rm1 = ((~S_IFMT)&(sbuf1.st_mode))
#if HAVE_S_IFLNK
	|(S_ISLNK(sbuf1.st_mode) ? 0777 : 0)
#endif
	;
      rm2 = ((~S_IFMT)&(sbuf2.st_mode))
#if HAVE_S_IFLNK
	|(S_ISLNK(sbuf1.st_mode) ? 0777 : 0)
#endif
	;

      mm1 = (rm1&(opts->mode_and))|(opts->mode_or);
      mm2 = (rm2&(opts->mode_and))|(opts->mode_or);

      if (mm1 != mm2)
	{
	  char buf[32];
	  /**/

	  if (opts->mode_or != OPT_MODE_OR_DEFAULT
	      || opts->mode_and != OPT_MODE_AND_DEFAULT) {
	    snprintf(buf, sizeof(buf), " (masked %04o %04o)",
		     mm1, mm2);
	  } else {
	    buf[0] = '\0';
	  }

	  printf("%s: %s: mode: %04o %04o%s\n",
		 progname, subpath, rm1, rm2, buf);
	  BUMP_EXIT_CODE(rv, XIT_DIFF);
	}
    }
#if HAVE_ST_FLAGS
  if (opts->flags && sbuf1.st_flags != sbuf2.st_flags)
    {
      int flags1 = sbuf1.st_flags;
      int flags2 = sbuf2.st_flags;

#define CHECK_FLAG(flag, desc) do {			  \
	if ((flags1 & (flag)) != ( flags2 & (flag))) {	  \
	  printf("%s: %s: flag %s only set in %.*s\n",	  \
		 progname, subpath, desc,		  \
		 (int)( (flags1 & (flag))		  \
		     ? opts->root1_length		  \
		     : opts->root2_length ),		  \
		 ( (flags1 & (flag)) ? p1 : p2 ));	  \
	  BUMP_EXIT_CODE(rv, XIT_DIFF);			  \
	}						  \
	flags1 &= ~flag;				  \
	flags2 &= ~flag;				  \
      } while(0)

#if HAVE_UF_NODUMP
      CHECK_FLAG(UF_NODUMP, "nodump");
#endif
#if HAVE_UF_IMMUTABLE
      CHECK_FLAG(UF_IMMUTABLE, "uimmutable");
#endif
#if HAVE_UF_APPEND
      CHECK_FLAG(UF_APPEND, "uappend");
#endif
#if HAVE_UF_OPAQUE
      CHECK_FLAG(UF_OPAQUE, "opaque");
#endif
#if HAVE_UF_NOUNLINK
      CHECK_FLAG(UF_NOUNLINK, "uunlink");
#endif
#if HAVE_UF_COMPRESSED
      CHECK_FLAG(UF_COMPRESSED, "compressed");
#endif
#if HAVE_UF_TRACKED
      CHECK_FLAG(UF_TRACKED, "tracked");
#endif
#if HAVE_UF_SYSTEM
      CHECK_FLAG(UF_SYSTEM, "system");
#endif
#if HAVE_UF_SPARSE
      CHECK_FLAG(UF_SPARSE, "sparse");
#endif
#if HAVE_UF_OFFLINE
      CHECK_FLAG(UF_OFFLINE, "offline");
#endif
#if HAVE_UF_REPARSE
      CHECK_FLAG(UF_REPARSE, "reparse");
#endif
#if HAVE_UF_ARCHIVE
      CHECK_FLAG(UF_ARCHIVE, "uarchive");
#endif
#if HAVE_UF_READONLY
      CHECK_FLAG(UF_READONLY, "readonly");
#endif
#if HAVE_UF_HIDDEN
      CHECK_FLAG(UF_HIDDEN, "hidden");
#endif
#if HAVE_SF_ARCHIVED
      CHECK_FLAG(SF_ARCHIVED, "archived");
#endif
#if HAVE_SF_IMMUTABLE
      CHECK_FLAG(SF_IMMUTABLE, "simmutable");
#endif
#if HAVE_SF_APPEND
      CHECK_FLAG(SF_APPEND, "sappend");
#endif
#if HAVE_SF_NOUNLINK
      CHECK_FLAG(SF_NOUNLINK, "sunlink");
#endif
#if HAVE_SF_SNAPSHOT
      CHECK_FLAG(SF_SNAPSHOT, "snapshot");
#endif

#undef CHECK_FLAG

      if (flags1 != flags2)
	{
	  printf("%s: %s: unknown flags: %08X %08X\n",
		 progname, subpath, flags1, flags2);
	  BUMP_EXIT_CODE(rv, XIT_DIFF);
	}
    }
#endif /* HAVE_ST_FLAGS */
  if (opts->uid && sbuf1.st_uid != sbuf2.st_uid)
    {
      const struct passwd* pw;
      char *pn1=NULL, *pn2=NULL;
      /**/
      if ((pw = getpwuid(sbuf1.st_uid))) pn1 = xstrdup(pw->pw_name);
      if ((pw = getpwuid(sbuf2.st_uid))) pn2 = xstrdup(pw->pw_name);

      printf("%s: %s: uid: %s(%ld) %s(%ld)\n",
	     progname,
	     subpath,
	     pn1 ? pn1 : "", (long)sbuf1.st_uid,
	     pn2 ? pn2 : "", (long)sbuf2.st_uid);

      if (pn1) free(pn1);
      if (pn2) free(pn2);

      BUMP_EXIT_CODE(rv, XIT_DIFF);
    }
  if (opts->gid && sbuf1.st_gid != sbuf2.st_gid)
    {
      const struct group* gr;
      char *gn1=NULL, *gn2=NULL;
      /**/
      if ((gr = getgrgid(sbuf1.st_gid))) gn1 = xstrdup(gr->gr_name);
      if ((gr = getgrgid(sbuf2.st_gid))) gn2 = xstrdup(gr->gr_name);

      printf("%s: %s: gid: %s(%ld) %s(%ld)\n",
	     progname,
	     subpath,
	     gn1 ? gn1 : "", (long)sbuf1.st_gid,
	     gn2 ? gn2 : "", (long)sbuf2.st_gid);

      if (gn1) free(gn1);
      if (gn2) free(gn2);

      BUMP_EXIT_CODE(rv, XIT_DIFF);
    }
  if (opts->mtime
      && (sbuf1.st_mtime != sbuf2.st_mtime
#ifdef ST_MTIMENSEC
	  || sbuf1.ST_MTIMENSEC.tv_nsec != sbuf2.ST_MTIMENSEC.tv_nsec
#endif
	  )
      )
    {
      reportTimeDiscrepancy(subpath, "mtime",
			    sbuf1.st_mtime, sbuf2.st_mtime,
#ifdef ST_CTIMENSEC
			    sbuf1.ST_MTIMENSEC.tv_nsec, sbuf2.ST_MTIMENSEC.tv_nsec
#else
			    -1, -1
#endif
			    );
      BUMP_EXIT_CODE(rv, XIT_DIFF);
    }
  if (opts->atime
      && ( sbuf1.st_atime != sbuf2.st_atime
#ifdef ST_ATIMENSEC
	   || sbuf1.ST_ATIMENSEC.tv_nsec != sbuf2.ST_ATIMENSEC.tv_nsec
#endif
	   )
      )
    {
      reportTimeDiscrepancy(subpath, "atime",
			    sbuf1.st_atime, sbuf2.st_atime,
#ifdef ST_CTIMENSEC
			    sbuf1.ST_ATIMENSEC.tv_nsec, sbuf2.ST_ATIMENSEC.tv_nsec
#else
			    -1, -1
#endif
			    );
      BUMP_EXIT_CODE(rv, XIT_DIFF);
    }
  if (opts->ctime
      && ( sbuf1.st_ctime != sbuf2.st_ctime
#ifdef ST_CTIMENSEC
	   || sbuf1.ST_CTIMENSEC.tv_nsec != sbuf2.ST_CTIMENSEC.tv_nsec
#endif
	   )
      )
    {
      reportTimeDiscrepancy(subpath, "ctime",
			    sbuf1.st_ctime, sbuf2.st_ctime,
#ifdef ST_CTIMENSEC
			    sbuf1.ST_CTIMENSEC.tv_nsec, sbuf2.ST_CTIMENSEC.tv_nsec
#else
			    -1, -1
#endif
			    );
      BUMP_EXIT_CODE(rv, XIT_DIFF);
    }

  // Don't report on sizes and blocks for directories
  if (((sbuf1.st_mode)&S_IFMT) != S_IFDIR)
    {
      if (opts->blocks && sbuf1.st_blocks != sbuf2.st_blocks)
	{
	  printf("%s: %s: blocks: %ld %ld\n",
		 progname,
		 subpath,
		 (long)sbuf1.st_blocks, (long)sbuf2.st_blocks);
	  BUMP_EXIT_CODE(rv, XIT_DIFF);
	}

      if (sbuf1.st_size != sbuf2.st_size)
	{
	  if (opts->size)
	    {
	      printf("%s: %s: size: %lld %lld\n",
		     progname,
		     subpath,
		     (long long)sbuf1.st_size, (long long)sbuf2.st_size);
	      BUMP_EXIT_CODE(rv, XIT_DIFF);
	    }
	  content_diff = 0;
	}
    }

  if (opts->nlink && sbuf1.st_nlink != sbuf2.st_nlink)
    {
      printf("%s: %s: nlink: %ld %ld\n",
	     progname,
	     subpath,
	     (long)sbuf1.st_nlink, (long)sbuf2.st_nlink);
      BUMP_EXIT_CODE(rv, XIT_DIFF);
    }

#if HAVE_LGETXATTR
  if (opts->xattr)
    {
      str_list_t		*xl1 = getXattrList(p1);
      str_list_t		*xl2 = getXattrList(p2);
      int			 nrv;
      /**/

      if (xl1 && xl2)
	{
	  str_list_client_data_t clientData;
	  clientData.opts = opts;
	  nrv = str_list_compare(p1, p2,
				 xl1, xl2,
				 reportMissingXattr,
				 compareXattrs,
				 &clientData);
	}
      else
	nrv = XIT_SYS;

      BUMP_EXIT_CODE(rv, nrv);

      if (xl1) str_list_destroy(xl1);
      if (xl2) str_list_destroy(xl2);
    }
#endif

#if HAVE_ACL
  if (opts->acl
#if HAVE_S_IFLNK
      && ((sbuf1.st_mode)&S_IFMT) != S_IFLNK
      && ((sbuf2.st_mode)&S_IFMT) != S_IFLNK
#endif
      )
    {
      int nrv;
      /**/
      nrv = diffacl(opts, p1, p2, ACL_TYPE_ACCESS, "access");
      BUMP_EXIT_CODE(rv, nrv);
    }

  if (opts->acl
      && ((sbuf1.st_mode)&S_IFMT) == S_IFDIR
      && ((sbuf2.st_mode)&S_IFMT) == S_IFDIR)
    {
      int nrv = diffacl(opts, p1, p2, ACL_TYPE_DEFAULT, "default");
      BUMP_EXIT_CODE(rv, nrv);
    }
#endif

  /* Type tests */
  if (((sbuf1.st_mode)&S_IFMT) != ((sbuf2.st_mode)&S_IFMT))
    {
      if (opts->type)
	{
	  printf("%s: %s: type: %s %s\n",
		 progname,
		 subpath,
		 getFileType(sbuf1.st_mode), getFileType(sbuf2.st_mode));
	  BUMP_EXIT_CODE(rv, XIT_DIFF);
	}
    }
  else
    /* Type specific checks */
    switch ((sbuf1.st_mode)&S_IFMT)
      {
      case S_IFDIR:
	{
	  str_list_t			*ct1, *ct2;
	  int				 nrv;
	  /**/

	  ct1 = getDirList(p1, &sbuf1, &fd1);
	  ct2 = getDirList(p2, &sbuf2, &fd2);

	  if (ct1 && ct2)
	    {
	      str_list_client_data_t	 clientData;

	      clientData.opts = opts;
	      nrv = str_list_compare(p1, p2,
				     ct1, ct2,
				     reportMissingFile,
				     compareFileEntries,
				     &clientData);
	    }
	  else
	    {
	      printf("%s: %s: pruning tree as %s unreadable\n",
		     progname, subpath,
		     (!ct1 && !ct2) ? "both subdirectories are" : "one directory is");
	      nrv = XIT_SYS;
	    }

	  BUMP_EXIT_CODE(rv, nrv);

	  if (ct1) str_list_destroy(ct1);
	  if (ct2) str_list_destroy(ct2);
	}
	break;
      case S_IFREG:
	if (opts->contents)
	  {
	    if (content_diff)
	      {
		++ opts->stats.contents_compared;
		if (opts->exec)
		  {
		    if (!execprocess(&opts->exec_args, p1, p2))
		      BUMP_EXIT_CODE(rv, XIT_DIFF);
		  }
		else
		  {
		    int nrv = cmpFiles(p1, &sbuf1, &fd1, p2, &sbuf2, &fd2);
		    BUMP_EXIT_CODE(rv, nrv);
		    switch(nrv)
		      {
		      case XIT_OK:
			break;
		      case XIT_DIFF:
			printf("%s: %s: contents differ\n",
			       progname, subpath);
			break;
		      default:
			printf("%s: %s: contents comparison skipped\n",
			       progname, subpath);
			break;
		      }
		  }
	      }
	    else
	      {
		printf("%s: %s: contents differ\n",
		       progname, subpath);
		BUMP_EXIT_CODE(rv, XIT_DIFF);
	      }
	    if (opts->exec_always
		&& !execprocess(&opts->exec_always_args, p1, p2))
	      BUMP_EXIT_CODE(rv, XIT_DIFF);
	  }
	break;
#if HAVE_S_IFLNK
      case S_IFLNK:
	if (opts->contents)
	  {
	    char *lnk1 = xreadlink(p1, &sbuf1, &fd1);
	    char *lnk2 = xreadlink(p2, &sbuf2, &fd2);
	    if (lnk1 && lnk2 && strcmp(lnk1, lnk2) != 0)
	      {
		printf("%s: %s: symbolic links differ\n",
		       progname, subpath);
		BUMP_EXIT_CODE(rv, XIT_DIFF);
	      }
	    if (lnk1) free(lnk1);
	    if (lnk2) free(lnk2);
	  }
	break;
#endif /* HAVE_S_IFLNK */
      case S_IFBLK:
      case S_IFCHR:
	if (opts->major && major(sbuf1.st_rdev) != major(sbuf2.st_rdev))
	  {
	    printf("%s: %s: major: %ld %ld\n",
		   progname,
		   subpath,
		   (long)major(sbuf1.st_rdev),
		   (long)major(sbuf2.st_rdev));
	    BUMP_EXIT_CODE(rv, XIT_DIFF);
	  }
	if (opts->minor && minor(sbuf1.st_rdev) != minor(sbuf2.st_rdev))
	  {
	    printf("%s: %s: minor: %ld %ld\n",
		   progname,
		   subpath,
		   (long)minor(sbuf1.st_rdev),
		   (long)minor(sbuf2.st_rdev));
	    BUMP_EXIT_CODE(rv, XIT_DIFF);
	  }
	break;
      default:
	break;
      }

  if (fd1 >= 0 && close(fd1))
    {
      xperror("cannot close", p1);
      BUMP_EXIT_CODE(rv, XIT_SYS);
    }

  if (fd2 >= 0 && close(fd2))
    {
      xperror("cannot close", p2);
      BUMP_EXIT_CODE(rv, XIT_SYS);
    }

  return rv;
}

static void
applyPresets(options_t* opts, int presetLevel)
{
  opts->dirs	   = opts->type                     = presetLevel >= 1;
  opts->mode					    = presetLevel >= 2;
  opts->uid	   = opts->gid       = opts->acl    = presetLevel >= 3;
  opts->nlink	   = opts->hardlinks                = presetLevel >= 4;
  opts->size	   = opts->blocks    =
  opts->contents   = opts->major     = opts->minor  = presetLevel >= 5;
  opts->flags      = opts->xattr                    = presetLevel >= 6;
  opts->mtime                                       = presetLevel >= 7;
  opts->atime                                       = presetLevel >= 8;
  opts->ctime                                       = presetLevel >= 9;
}

int
main(int argc, char*argv[])
{
  options_t options;
  enum { EAO_no, EAO_ok, EAO_error } end_after_options = EAO_no;
  int rv, optcode;
  /**/

  /* Set stdout and stderr to line buffered to avoid intermingling
     their outputs. */
  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IOLBF, 0);

  setprogname(argv[0]);

  #if DEBUG
  fprintf(stderr, "%s: memory stats at start:\n",
	  progname);
  pmem();
  fprintf(stderr, "%s: end\n", progname);
  #endif

  memset(&options, 0, sizeof(options));

  applyPresets(&options, 6);

  options.mode_or           = OPT_MODE_OR_DEFAULT;
  options.mode_and	    = OPT_MODE_AND_DEFAULT;

  options.exclusions = gh_new(&gh_string_hash, &gh_string_equal, &free, NULL);

  /* For getopt */
  argv[0] = (char*)progname;

  while(1)
    {
#if HAVE_GETOPT_LONG
      struct option long_options[] = {
	{ "verbose",           0, 0, 'v' },
	{ "version",           0, 0, 'V' },
	{ "help",              0, 0, 'h' },
	{ "dirs",              0, 0, 'd' },
	{ "no-dirs",           0, 0, 'D' },
	{ "type",              0, 0, 't' },
	{ "no-type",           0, 0, 'T' },
	{ "mode",              0, 0, 'm' },
	{ "no-mode",           0, 0, 'M' },
	{ "uid",               0, 0, 'u' },
	{ "no-uid",            0, 0, 'U' },
	{ "gid",               0, 0, 'g' },
	{ "no-gid",            0, 0, 'G' },
	{ "nlink",             0, 0, 'n' },
	{ "no-nlink",          0, 0, 'N' },
	{ "hardlinks",         0, 0, 'e' },
	{ "no-hardlinks",      0, 0, 'E' },
	{ "mtime",             0, 0, 'i' },
	{ "no-mtime",          0, 0, 'I' },
	{ "atime",             0, 0, 'y' },
	{ "no-atime",          0, 0, 'Y' },
	{ "ctime",             0, 0, 'z' },
	{ "no-ctime",          0, 0, 'Z' },
	{ "size",              0, 0, 's' },
	{ "no-size",           0, 0, 'S' },
	{ "blocks",            0, 0, 'b' },
	{ "no-blocks",         0, 0, 'B' },
	{ "contents",          0, 0, 'c' },
	{ "no-contents",       0, 0, 'C' },
	{ "major",             0, 0, 'j' },
	{ "no-major",          0, 0, 'J' },
	{ "minor",             0, 0, 'k' },
	{ "no-minor",          0, 0, 'K' },
#if HAVE_ST_FLAGS
	{ "flags",             0, 0, 'f' },
	{ "no-flags",          0, 0, 'F' },
#endif
#if HAVE_ACL
	{ "acl",               0, 0, 'l' },
	{ "no-acl",            0, 0, 'L' },
#endif
#if HAVE_LGETXATTR
	{ "xattr",             0, 0, 'q' },
	{ "no-xattr",          0, 0, 'Q' },
#endif
	{ "preset",            1, 0, 'p' },
	{ "mode-and",          1, 0, 'a' },
	{ "mode-or",           1, 0, 'o' },
	{ "exec",              0, 0, 'x' },
	{ "exclude",           1, 0, 'X' },
	{ "exec-always",       0, 0, 'w' },
	{ "exec-always-diff",  0, 0, 'W' },
	{ "follow-symlinks",   0, 0, 'O' },
	{ 0,                   0, 0, 0}
      };
#endif /* HAVE_GETOPT_LONG */
      switch( ( optcode =
#if HAVE_GETOPT_LONG
	     getopt_long
#else
	     getopt
#endif
	     (argc, argv, "vVhdDtTmM"
	      "uUgGnNeEiIyYzZsSbBcCjJkK"
#if HAVE_ST_FLAGS
	      "fF"
#endif
#if HAVE_ACL
	      "lL"
#endif
#if HAVE_LGETXATTR
	      "qQ"
#endif
	      "0123456789p:a:o:wWxX:O"
#if HAVE_GETOPT_LONG
	      , long_options, NULL
#endif
	      )))
	{
	case -1:
	  /* end of options */
	  goto end_of_options;
	case '?':
	  /* Unknown option */
	  end_after_options = 1;
	  break;
	case 'V':
	  show_version();
	  end_after_options = 1;
	  break;
	case 'h':
	  show_help();
	  end_after_options = 1;
	  break;
	case 'v': options.verbosityLevel += 1; break;
	case 'd': options.dirs            = 1; break;
	case 'D': options.dirs            = 0; break;
	case 't': options.type		  = 1; break;
	case 'T': options.type		  = 0; break;
	case 'm': options.mode		  = 1; break;
	case 'M': options.mode		  = 0; break;
	case 'u': options.uid		  = 1; break;
	case 'U': options.uid		  = 0; break;
	case 'g': options.gid		  = 1; break;
	case 'G': options.gid		  = 0; break;
	case 'n': options.nlink		  = 1; break;
	case 'N': options.nlink		  = 0; break;
	case 'e': options.hardlinks	  = 1; break;
	case 'E': options.hardlinks	  = 0; break;
	case 'i': options.mtime		  = 1; break;
	case 'I': options.mtime		  = 0; break;
	case 'y': options.atime		  = 1; break;
	case 'Y': options.atime		  = 0; break;
	case 'z': options.ctime		  = 1; break;
	case 'Z': options.ctime		  = 0; break;
	case 's': options.size		  = 1; break;
	case 'S': options.size		  = 0; break;
	case 'b': options.blocks	  = 1; break;
	case 'B': options.blocks	  = 0; break;
	case 'c': options.contents	  = 1; break;
	case 'C': options.contents	  = 0; break;
	case 'j': options.major		  = 1; break;
	case 'J': options.major		  = 0; break;
	case 'k': options.minor		  = 1; break;
	case 'K': options.minor		  = 0; break;
#if HAVE_ST_FLAGS
	case 'f': options.flags           = 1; break;
	case 'F': options.flags           = 0; break;
#endif
#if HAVE_ACL
	case 'l': options.acl		  = 1; break;
	case 'L': options.acl		  = 0; break;
#endif
#if HAVE_LGETXATTR
	case 'q': options.xattr		  = 1; break;
	case 'Q': options.xattr		  = 0; break;
#endif
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  applyPresets(&options, optcode - '0');
	  break;
	case 'p':
	  if (optarg[0] >= '0' && optarg[0] <= '9' && optarg[1] == '\0')
	    applyPresets(&options, optarg[0] - '0');
	  else
	    {
	      static const struct {
		const char * const name;
		unsigned char level;
		unsigned char minreq;
	      } presetNames[] = {
				 { "alltimes",  9, 2},
				 { "amtimes",   8, 2},
				 { "contents",  5, 1},
				 { "default",   6, 1},
				 { "hardlinks", 4, 1},
				 { "mode",      2, 2},
				 { "missing",   1, 2},
				 { "mtime",     7, 2},
				 { "none",      0, 3},
				 { "notimes",   6, 3},
				 { "owner",     3, 1},
				 { "type",      1, 1}
	      };
	      size_t i;
	      size_t optlen = strlen(optarg);
	      int found = 0;

	      for (i=0; i < sizeof(presetNames)/sizeof(presetNames[0]); ++i)
		{
		  if (optlen <= strlen(presetNames[i].name)
		      && optlen >= presetNames[i].minreq
		      && memcmp(optarg, presetNames[i].name, optlen) == 0)
		    {
		      applyPresets(&options, presetNames[i].level);
		      found = 1;
		      break;
		    }
		}
	      if (!found)
		{
		  fprintf(stderr, "%s: unknown preset \"%s\"\n", progname, optarg);
		  end_after_options = EAO_error;
		}
	    }
	  break;
	case 'a':
	  if (!get_octal_arg(optarg, &options.mode_and))
	    end_after_options = EAO_error;
	  break;
	case 'o':
	  if (!get_octal_arg(optarg, &options.mode_or))
	    end_after_options = EAO_error;
	  break;
	case 'w':
	case 'W':
	  if (options.exec_always)
	    {
	      fprintf(stderr, "%s: -w/-W specified twice, only the last command line will run\n", progname);
	    }
	  if (optcode == 'w')
	    {
	      get_exec_args(argv, &optind, &options.exec_always_args);
	      options.contents = options.exec_always = 1;
	    }
	  else /* -W */
	    {
	      if (options.exec_always_args.argv != NULL)
		{
		  free(options.exec_always_args.argv);
		}
	      options.exec_always_args.argv = xmalloc(sizeof(char*)*5);
	      options.exec_always_args.argv[0] = "diff";
	      options.exec_always_args.argv[1] = "-u";
	      options.exec_always_args.argv[2] = NULL;
	      options.exec_always_args.argv[3] = NULL;
	      options.exec_always_args.argv[4] = NULL;
	      options.exec_always_args.arg1 = &options.exec_always_args.argv[2];
	      options.exec_always_args.arg2 = &options.exec_always_args.argv[3];
	      options.contents = options.exec_always = 1;
	    }
	  break;
	case 'x':
	  if (options.exec)
	    {
	      fprintf(stderr, "%s: -x specified twice, only the last command line will run\n", progname);
	    }
	  get_exec_args(argv, &optind, &options.exec_args);
	  options.contents = options.exec = 1;
	  break;
	case 'X':
	  gh_insert(options.exclusions, xstrdup(optarg), NULL);
	  break;
	case 'O':
	  options.follow_symlinks = 1;
	  break;
	default:
	  fprintf(stderr, "%s: unexpected return value from getopt(): \"%c\" (code=%d), aborting...\n",
		  progname, isprint(optcode) ? optcode : '?', optcode);
	  exit(XIT_INTERNALERROR);
	}
    }
 end_of_options:
  switch (end_after_options)
    {
    case EAO_no:
      break;
    case EAO_ok:
      exit(XIT_OK);
    case EAO_error:
      exit(XIT_INVOC);
    default:
      fprintf(stderr, "%s: unknown EAO enum value\n", progname);
      exit(XIT_INTERNALERROR);
    }

  argc -= optind;
  argv += optind;

  printopts(&options);

  if (argc!=2)
    {
      fprintf(stderr, "%s: needs two arguments\n", progname);
      exit(XIT_INVOC);
    }

  if ( options.verbosityLevel >= VERB_MEM_STATS)
    {
      fprintf(stderr, "%s: memory stats before starting traversal:\n",
	      progname);
      pmem();
      fprintf(stderr, "%s: end\n", progname);
    }

  options.root1_length = strlen(argv[0]);
  while (argv[0][options.root1_length-1] == '/' && options.root1_length>1)
    argv[0][--options.root1_length] = 0;

  options.root2_length = strlen(argv[1]);
  while (argv[1][options.root2_length-1] == '/' && options.root2_length>1)
    argv[1][--options.root2_length] = 0;

  options.inocache = epc_new();

  if (options.hardlinks)
    {
      options.hardlinks1 = hc_new();
      options.hardlinks2 = hc_new();
    }

  rv = dodiff(&options, argv[0], argv[1]);

  if (options.exec)
    free(options.exec_args.argv);
  if (options.exec_always)
    free(options.exec_always_args.argv);
  if (options.empty_str_list != NULL)
    str_list_destroy(options.empty_str_list);
  gh_delete(options.exclusions);

  if (options.verbosityLevel >= VERB_STATS)
    {
      fprintf(stderr, "%s: %12lld          inode pairs compared\n",
	      progname, options.stats.compared);
      if (options.contents)
	{
	  fprintf(stderr, "%s: %12lld (%5.02f%%) file pair contents compared\n",
		  progname,
		  options.stats.contents_compared,
		  ( 100.0 * options.stats.contents_compared
		    / options.stats.compared));
	}
      fprintf(stderr, "%s: %12lld (%5.02f%%) inode pairs skipped as identical\n",
	      progname,
	      options.stats.skipped_same,
	      ( 100.0 * options.stats.skipped_same
		/ options.stats.compared));
      fprintf(stderr, "%s: %12lld (%5.02f%%) inode pairs skipped as already seen\n",
	      progname,
	      options.stats.skipped_cache,
	      ( 100.0 * options.stats.skipped_cache
		/ options.stats.compared));

      if ( options.dirs )
	fprintf(stderr, "%s: %12lld          unmatched paths\n",
		progname, options.stats.singles);

      fprintf(stderr, "%s: %12lld          excluded paths\n",
	      progname, options.stats.excluded);

    }

  if ( options.verbosityLevel >= VERB_HASH_STATS )
    {
      fprintf(stderr, "%s: inode cache statistics:\n", progname);
      gh_stats(options.inocache, "inode cache");
      fprintf(stderr, "%s: end\n", progname);

      if (options.hardlinks)
	{
	  fprintf(stderr, "%s: hard links cache 1 statistics:\n", progname);
	  gh_stats(options.hardlinks1, "hard links cache 1");
	  fprintf(stderr, "%s: end\n", progname);

	  fprintf(stderr, "%s: hard links cache 2 statistics:\n", progname);
	  gh_stats(options.hardlinks2, "hard links cache 2");
	  fprintf(stderr, "%s: end\n", progname);
	}
    }

  if ( options.verbosityLevel >= VERB_MEM_STATS)
    {
      fprintf(stderr, "%s: memory stats before releasing inode cache:\n",
	      progname);
      pmem();
      fprintf(stderr, "%s: end\n", progname);
    }

  epc_destroy(options.inocache);

  if (options.hardlinks)
    {
      hc_destroy(options.hardlinks1);
      hc_destroy(options.hardlinks2);
    }
  if ( options.verbosityLevel >= VERB_MEM_STATS)
    {
      fprintf(stderr, "%s: memory stats before exit:\n", progname);
      pmem();
      fprintf(stderr, "%s: end\n", progname);
    }

  exit(rv);
}
