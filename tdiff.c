/*
  tdiff - tree diffs
  Main()
  Copyright (C) 1999, 2008, 2014, 2019 Philippe Troin <phil+github-commits@fifi.org>

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

#include "tdiff.h"
#include "utils.h"
#include "genhash.h"
#include "inocache.h"

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

#if HAVE_GETXATTR
#include <attr/xattr.h>
#endif

#if HAVE_ACL
#include <sys/acl.h>
#endif

#if HAVE_ST_XTIMESPEC
# define ST_ATIMENSEC st_atimespec
# define ST_CTIMENSEC st_ctimespec
# define ST_MTIMENSEC st_mtimespec
#elif HAVE_ST_TIMENSEC
# define ST_ATIMENSEC st_atim
# define ST_CTIMENSEC st_ctim
# define ST_MTIMENSEC st_mtim
#endif

#if __GNUC__
# define ATTRIBUTE_UNUSED __attribute__((unused))
#else /* ! __GNUC__ */
# define ATTRIBUTE_UNUSED /**/
#endif /* ! __GNUC__ */

#define GETSTRLIST_INITIAL_SIZE 8
#define XATTR_BUF_SIZE		16384
#define GETDIRLIST_DENTBUF_SIZE 8192
#define XREADLINK_BUF_SIZE	1024
#define CMPFILE_BUF_SIZE	16384

#define VERB_STATS	1
#define VERB_SKIPS	2
#define VERB_HASH_STATS 3
#define VERB_MEM_STATS  4

typedef long long counter_t;

typedef struct strl_s
{
  size_t size;
  size_t avail;
  const char **strings;
} strl_t;

typedef struct dexe_s
{
  char ** argv;
  char ** arg1;
  char ** arg2;
} dexe_t;

#define OPT_MODE_OR_DEFAULT  ((unsigned)0)
#define OPT_MODE_AND_DEFAULT ((unsigned)~0)
typedef struct option_s
{
  unsigned int verbosityLevel:8;
  unsigned int dirs:1;
  unsigned int type:1;
  unsigned int mode:1;
  unsigned int flags:1;
  unsigned int owner:1;
  unsigned int group:1;
  unsigned int ctime:1;
  unsigned int mtime:1;
  unsigned int atime:1;
  unsigned int size:1;
  unsigned int blocks:1;
  unsigned int contents:1;
  unsigned int nlinks:1;
  unsigned int major:1;
  unsigned int minor:1;
  unsigned int xattr:1;
  unsigned int acl:1;
  unsigned int exec:1;
  unsigned int exec_always:1;
  unsigned int mode_or;
  unsigned int mode_and;
  genhash_t*   exclusions;
  dexe_t exec_args;
  dexe_t exec_always_args;
  size_t root1_length;
  size_t root2_length;
  inocache_t *inocache;
  struct
  {
    counter_t singles;
    counter_t compared;
    counter_t contents_compared;
    counter_t excluded;
    counter_t skipped_same;
    counter_t skipped_cache;
  } stats;

} options_t;

strl_t *getDirList(const char* path);
void freeStrlist(strl_t *d);
int get_exec_args(char**, int*, dexe_t*);
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
 * String list handling
 */
typedef struct commonClientData_s
{
  options_t *opts;
} commonClientData_t;

void
newStrList(strl_t **l)
{
  strl_t* rv;
  /**/
  *l = NULL;
  rv = xmalloc(sizeof(strl_t));
  rv->size = 0;
  rv->avail = GETSTRLIST_INITIAL_SIZE;
  rv->strings = xmalloc(sizeof(const char*)*rv->avail);

  *l =rv;
}

void
pushStrList(strl_t *l, const char* s)
{
  if (l->size == l->avail)
    l->strings = xrealloc(l->strings, (l->avail*=2)*sizeof(const char*));
  l->strings[l->size++] = xstrdup(s);
}

void
pushStrListSized(strl_t *l, const char* s, size_t sz)
{
  char *buf;
  /**/

  if (l->size == l->avail)
    l->strings = xrealloc(l->strings, (l->avail*=2)*sizeof(const char*));
  buf = xmalloc(sz);
  memcpy(buf, s, sz);
  l->strings[l->size++] = buf;
}

int strpcmp(const char** s1, const char** s2)
{
  return (strcmp(*s1, *s2));
}

void
sortStrList(const strl_t *l)
{
  qsort(l->strings, l->size, sizeof(char*),
	(int(*)(const void*, const void*))strpcmp);
}

void
freeStrList(strl_t *d)
{
  size_t i;
  /**/
  for (i=0; i<d->size; ++i)
    free((char*)d->strings[i]);
  free(d->strings);
  free(d);
}

int
compareStrList(const char *p1, const char* p2,
	       const strl_t *ct1, const strl_t *ct2,
	       void (*reportMissing)(int, const char*, const char*, commonClientData_t*),
	       int (*compareEntries)(const char* p1, const char* p2,
				     const char* e1, const char* e2,
				     commonClientData_t*),
	       commonClientData_t *clientData)
{
  size_t	i1, i2;
  int		cmpres;
  int		rv	     = XIT_OK;
  int		localerr     = 0;
  /**/
  sortStrList(ct1);
  sortStrList(ct2);

  for (i1 = i2 = 0; i1 < ct1->size || i2 < ct2->size; )
    {
      if (i1 == ct1->size)
	{
	  reportMissing(2, p2, ct2->strings[i2], clientData);
	  i2++;
	}
      else if (i2 == ct2->size)
	{
	  reportMissing(1, p1, ct1->strings[i1], clientData);
	  i1++;
	}
      else if (!(cmpres = strcmp(ct1->strings[i1], ct2->strings[i2])))
	{
	  int nrv;
	  /**/
	  nrv = compareEntries(p1, p2, ct1->strings[i1], ct2->strings[i2], clientData);
	  if (nrv>rv) rv = nrv;
	  i1++;
	  i2++;
	}
      else if (cmpres<0)
	{
	  reportMissing(1, p1, ct1->strings[i1], clientData);
	  i1++;
	}
      else /* cmpres>0 */
	{
	  reportMissing(2, p2, ct2->strings[i2], clientData);
	  i2++;
	}

    }

  if (localerr && XIT_DIFF>rv) rv = XIT_DIFF;

  return rv;
}

/*
 * Xattrs comparisons
 */
#if HAVE_GETXATTR
strl_t*
getXattrList(const char* path)
{
  strl_t *rv;
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
	newStrList(&rv);
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

  newStrList(&rv);
  for (p = buf, p_e = buf+rSize;
       p < p_e && *p;
       p += strlen(p)+1)
    pushStrList(rv, p);

  free(buf);
  return rv;
}

static void
reportMissingXattr(int which, const char* f, const char* xn, commonClientData_t* clientData)
{
  const char*	subp;
  int		rootlen;
  /**/

#if HAVE_ACL
  if (dropAclXattrs(xn))
    return;
#endif

  switch(which)
    {
    case 1:
      subp = f+(rootlen = clientData->opts->root1_length);
      break;
    case 2:
      subp = f+(rootlen = clientData->opts->root2_length);
      break;
    default:
      abort();
    }
  if (*subp)
    {
      if (*subp != '/')
	abort();
      ++subp;
    }
  else
    {
      subp = "(top-level)";
    }
  printf("%s: %s: xattr %s: only present in %.*s\n",
	 progname, subp, xn, rootlen, f);
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
	      commonClientData_t* clientData)
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
      rv = XIT_SYS;
      xperror("cannot get extended attribute list, lgetxattr()", p1);
      goto ret;
    }

  if (!buf2)
    {
      rv = XIT_SYS;
      xperror("cannot get extended attribute list, lgetxattr()", p2);
      goto ret;
    }

  if (sz1 != sz2 || memcmp(buf1, buf2, sz1))
    {
      printf("%s: %s: xattr %s: contents differ\n",
	     progname, p1+clientData->opts->root1_length+1, e1);
      if (XIT_DIFF > rv)
	rv = XIT_DIFF;
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
	  || !strcmp(xn, "trusted.SGI_ACL_FILE"));
}
#endif

/*
 * ACL comparisons
 */
#if HAVE_ACL
strl_t *
getAclList(const char* path, acl_type_t acltype)
{
  acl_t acl = acl_get_file(path , acltype);
  ssize_t acllen;
  char *acls;
  char *p;
  strl_t *rv;
  /**/

  if (acl == NULL)
    switch(errno)
      {
      case ENOTSUP:
      case EINVAL:
	newStrList(&rv);
	return rv;
      default:
	xperror("cannot get ACL, acl_get_file()", path);
	return NULL;
      }

  acls = acl_to_text(acl, &acllen);
  if (!acls)
    {
      xperror("cannot get ACL, acl_to_text()", path);
      acl_free(acl);
      return NULL;
    }

  newStrList(&rv);

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
	  abort();
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
	    fprintf(stderr, "%s: unexpected character in state %d: %c\n",
		    progname, state, *p);
	    abort();
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
	      pushStrListSized(rv, buf, p-beg_user+2);
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
		fprintf(stderr, "%s: unexpected non-whitespace character in state %d: %c (code=%d)\n",
			progname, state, *p, *p);
		abort();
	      }
	  }
	break;
      case STATE_COMMENT:
	if (*p == '\n')
	  state = STATE_FIRST_WS;
	break;
      default:
	abort();
      }

  if (state != STATE_FIRST_WS)
    abort();

  acl_free(acls);
  acl_free(acl);

  return rv;
}

typedef struct aclCompareClientData_s
{
  commonClientData_t	cmn;
  const char*		acldescr;
} aclCompareClientData_t;

void
reportMissingAcl(int which, const char* f, const char* xn, commonClientData_t* commonClientData)
{
  aclCompareClientData_t*	clientData = (aclCompareClientData_t*)commonClientData;
  const char*			subp;
  int				rootlen;
  /**/
  switch(which)
    {
    case 1:
      subp = f+(rootlen = clientData->cmn.opts->root1_length);
      break;
    case 2:
      subp = f+(rootlen = clientData->cmn.opts->root2_length);
      break;
    default:
      abort();
    }
  if (*subp)
    {
      if (*subp != '/')
	abort();
      ++subp;
    }
  else
    {
      subp = "(top-level)";
    }
  printf("%s: %s: %s acl %s: only present in %.*s\n",
	 progname, subp, clientData->acldescr, xn, rootlen, f);
}

int
compareAcls(const char* p1, ATTRIBUTE_UNUSED const char* p2,
	    const char* e1, const char* e2,
	    commonClientData_t* commonClientData)
{
  aclCompareClientData_t* clientData = (aclCompareClientData_t*)commonClientData;
  int rv = XIT_OK;
  const char *v1;
  const char *v2;

  v1 = e1 + strlen(e1)+1;
  v2 = e2 + strlen(e1)+1;

  if (strcmp(v1, v2))
    {
      printf("%s: %s: %s acl %s: %s %s\n",
	     progname, p1+clientData->cmn.opts->root1_length+1, clientData->acldescr, e1, v1, v2);
      if (XIT_DIFF > rv)
	rv = XIT_DIFF;
    }

  return rv;
}

int
diffacl(options_t* opts, const char* p1, const char* p2,
	acl_type_t acltype, const char* acldescr)
{
  strl_t *acl1 = getAclList(p1, acltype);
  strl_t *acl2 = getAclList(p2, acltype);
  aclCompareClientData_t clientData;
  int rv;
  /**/

  clientData.cmn.opts = opts;
  clientData.acldescr = acldescr;

  if (acl1 && acl2)
    rv = compareStrList(p1, p2,
			acl1, acl2,
			reportMissingAcl,
			compareAcls,
			(commonClientData_t*)&clientData);
  else
    rv = XIT_SYS;

  if (acl1) freeStrList(acl1);
  if (acl2) freeStrList(acl2);

  return rv;
}

#endif /* HAVE_ACL */

/*
 * Directory comparisons
 */
strl_t *
getDirList(const char* path)
#if HAVE_GETDENTS
{
  char dentbuf[GETDIRLIST_DENTBUF_SIZE];
  int fd;
  strl_t *rv = NULL;
  int nread;
  /**/
  fd = open(path, O_RDONLY
#if HAVE_O_DIRECTORY
		 |O_DIRECTORY
#endif /* HAVE_O_DIRECTORY */
#if HAVE_O_NOATIME
		 |O_NOATIME
#endif /* HAVE_O_NOATIME */
	    );
  if (fd<0)
    goto err;

  newStrList(&rv);

  while((nread = getdents(fd, (void*)dentbuf,
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

	  pushStrList(rv, dent->d_name);
	}
    }

  if (nread<0 || close(fd)<0)
    goto err;

  return rv;

 err:
  xperror("cannot get directory listing, getdents()", path);
  return rv;
}
#else
{
#if HAVE_FDOPENDIR
  int		 dirfd;
#endif /* HAVE_FDOPENDIR */
  DIR		*dir;
  struct dirent *dent;
  strl_t	*rv = NULL;
  /**/

#if HAVE_FDOPENDIR
  dirfd = open(path, O_RDONLY
#if HAVE_O_DIRECTORY
	       |O_DIRECTORY
#endif /* HAVE_O_DIRECTORY */
#if HAVE_O_NOATIME
	       |O_NOATIME
#endif /* HAVE_O_NOATIME */
	       );
  if ( dirfd < 0 )
    goto err;

  dir = fdopendir(dirfd);
  if (!dir)
    {
      close(dirfd);
      goto err;
    }
#else /* ! HAVE_FDOPENDIR */
  dir = opendir(path);
  if (!dir)
    goto err;
#endif /* ! HAVE_FDOPENDIR */

  newStrList(&rv);

  while ((dent = readdir(dir)))
    {
      if (dent->d_name[0]=='.'
	  && (dent->d_name[1] == 0
	      || (dent->d_name[1]=='.'
		  && dent->d_name[2]== 0)))
	continue;

      pushStrList(rv, dent->d_name);
    }

  if (closedir(dir))
    goto err;

  return rv;

 err:
  xperror("cannot get directory listing, readdir()", path);
  return rv;
}
#endif

static void
reportMissingFile(int which, const char* d, const char *f, commonClientData_t* clientData)
{
  if ( ! clientData->opts->dirs )
    return;

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
	  abort();
	}
      if (*subp)
	{
	  if (*subp != '/')
	    abort();
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
}

static int
compareFileEntries(const char* p1, const char* p2,
		   const char* e1, ATTRIBUTE_UNUSED const char* e2,
		   commonClientData_t* clientData)
{
  int rv = XIT_OK;
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


int
cmpFiles(const char* f1, const struct stat sbuf1,
	 const char* f2, const struct stat sbuf2)
{
  int fd1, fd2;
  int result = 1;
  char buf1[CMPFILE_BUF_SIZE];
  char buf2[CMPFILE_BUF_SIZE];
  ssize_t toread;
  /**/

  if (sbuf1.st_size != sbuf2.st_size)
    {
      fprintf(stderr, "%s: cmpFiles called on files of different sizes\n",
	      progname);
      exit(XIT_INTERNALERROR);
    }

  if (sbuf1.st_size == 0)
    {
      return 1;
    }

  if ((fd1 = open(f1, O_RDONLY
#if HAVE_O_NOATIME
		     |O_NOATIME
#endif /* HAVE_O_NOATIME */
		  ))<0)
    {
      xperror("cannot open file, open()", f1);
      return 0;
    }
  if ((fd2 = open(f2, O_RDONLY
#if HAVE_O_NOATIME
		     |O_NOATIME
#endif /* HAVE_O_NOATIME */
		  ))<0)
    {
      xperror("cannot open file, open()", f2);
      goto fail1;
    }

	/**/

  for (toread = sbuf1.st_size; toread>0; )
    {
      int nread1, nread2;
      /**/
      nread1 = read(fd1, buf1, CMPFILE_BUF_SIZE);
      if (nread1 < 0)
	{
	  xperror("read()", f1);
	  goto fail;
	}
      if (nread1 < CMPFILE_BUF_SIZE && nread1 != toread)
	{
	  fprintf(stderr, "%s: %s: short read\n", progname, f1);
	  goto fail;
	}
      nread2 = read(fd2, buf2, CMPFILE_BUF_SIZE);
      if (nread2 < 0)
	{
	  xperror("read()", f2);
	  goto fail;
	}
      if (nread2 < CMPFILE_BUF_SIZE && nread2 != toread)
	{
	  fprintf(stderr, "%s: %s: short read\n", progname, f2);
	  goto fail;
	}
      if (nread1 != nread2)
	{
	  fprintf(stderr, "%s: %s, %s: read different number of bytes (%d vs. %d)\n",
		  progname, f1, f2, (int)nread1, (int)nread2);
	  goto fail;
	}
      if (memcmp(buf1, buf2, nread1)!=0)
	{
	  result = 0;
	  break;
	}
      toread -= nread1;
    }

  if (close(fd2)<0)
    {
      xperror("cannot close file, close()", f2);
      goto fail1;
    }
  if (close(fd1)<0)
    {
      xperror("cannot close file, close()", f1);
      return 0;
    }

  return result;

 fail:
  close(fd2);
 fail1:
  close(fd1);
  return 0;
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

  width_string = getenv("WIDTH");
  if (width_string != NULL)
    {
      unsigned width;
      char *ptr;

      width = strtoul(width_string, &ptr, 0);
      errno = 0;
      if (*ptr == '\0' && width > 0 && errno == 0 && width < UINT_MAX )
	return (unsigned)width;
    }

  if (ioctl(fileno(fd), TIOCGWINSZ, &ws) != -1)
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
      else if (printed_so_far + item_length + 2 > width)
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
#else /* ! HAVE_ACL */
	     "acl=no",
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
#if HAVE_GETXATTR
	     "xattr=yes",
#else /* ! HAVE_GETXATTR */
	     "xattr=no",
#endif /* ! HAVE_GETXATTR */
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
	 "Copyright (C) 1999-2019 Philippe Troin <phil+github-commits@fifi.org>.\n"
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
#if HAVE_ST_FLAGS
	 "   -f --flags     diffs flags (4.4BSD)\n"
#endif
	 "   -o --owner     diffs file owner\n"
	 "   -g --group     diffs file group\n"
	 "   -z --ctime     diffs ctime (inode modification time)\n"
	 "   -i --mtime     diffs mtime (contents modification time)\n"
	 "   -r --atime     diffs atime (access time)\n"
	 "   -s --size      diffs file size (for regular files, symlinks)\n"
	 "   -b --blocks    diffs file blocks (for regular files, symlinks & directories)\n"
	 "   -c --contents  diffs file contents (for regular files and symlinks)\n"
	 "   -n --nlinks    diffs the (hard) link count \n"
	 "   -j --major     diffs major device numbers (for device files)\n"
	 "   -k --minor     diffs minor device numbers (for device files)\n"
#if HAVE_GETXATTR
	 "   -q --xattr     diffs file extended attributes\n"
#endif
#if HAVE_ACL
	 "   -l --acl       diffs file ACLs\n"
#endif
	 "  Each of these options can be negated with an uppercase (short option)\n"
	 "  or with --no-option (eg -M --no-mode for not diffing modes)\n"
	 "   -a --all      equivalent to -dtm"
#if HAVE_ST_FLAGS
	 "f"
#endif
	 "ogsbcnejk"
#if HAVE_GETXATTR
	 "q"
#endif
#if HAVE_ACL
	 "l"
#endif
	 " (sets all but times)\n"
	 "   -A --no-all   clears all flags (will not report anything)\n"
	 " Miscellania:\n"
	 "   -x --exec <cmd> \\;         executes <cmd> between files if they are similar\n"
	 "                              (if file sizes are equal)\n"
	 "   -w --exec-always <cmd> \\;  always executes <cmd> between files\n"
	 "        <cmd> uses %%1 and %%2 as placeholders for files from <dir1> and <dir2>\n"
	 "   -W --exec-always-diff \\;   always executes \"diff -u\" between files\n"
	 "        equivalent to -w diff -u %%1 %%2 \\;\n"
	 "   -| --mode-or <bits>        applies <bits> OR mode before comparison\n"
	 "   -& --mode-and <bits>       applies <bits> AND mode before comparison\n"
	 "   -X --exclude <file>        omits <file> from report\n"
#if ! HAVE_GETOPT_LONG
	 "WARNING: your system does not have getopt_long (use a GNU system !)\n"
#endif
	 ,progname, progname);
}

int
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
      return 0;
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
  return 1;
}

int
get_octal_arg(const char* string, unsigned int* val)
{
  size_t len;
  char *ptr;
  unsigned long m;

  len = strlen(string);
  if (len > 2 && string[0]=='0' && string[1]=='x')
    m = strtoul(&string[2], &ptr, 16);
  else
    m = strtoul(string, &ptr, 8);

  if (*ptr)
    {
      fprintf(stderr, "%s: not a number \"%s\"\n", progname, string);
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
  POPT(dirs);
  POPT(type);
  POPT(mode);
  POPT(owner);
  POPT(group);
  POPT(ctime);
  POPT(mtime);
  POPT(atime);
  POPT(size);
  POPT(blocks);
  POPT(contents);
  POPT(nlinks);
  POPT(major);
  POPT(minor);
  POPT(xattr);
  POPT(acl);
  POPT(exec);
  POPT(exec_always);
#undef POPT
  printf("mode OR = %04o\n", o->mode_or);
  printf("mode AND = %04o\n", o->mode_and);
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

char*
xreadlink(const char* path)
{
  char *buf;
  int bufsize = XREADLINK_BUF_SIZE;
  int nstored;
  /**/
  buf = xmalloc(bufsize);
 again:
  if ((nstored=readlink(path, buf, bufsize))<0)
    {
      free(buf);
      xperror("cannot read link target, readlink()", path);
      return NULL;
    }
  if (nstored==bufsize-1)
    {
      buf = xrealloc(buf, bufsize*=2);
      goto again;
    }
  buf[nstored] = 0;
  return buf;
}

int
dodiff(options_t* opts, const char* p1, const char* p2)
{
  struct stat sbuf1;
  struct stat sbuf2;
  ic_ent_t *ice;
  char *icepath;
  int rv = XIT_OK;
  int localerr = 0;
  const char *subpath;
  int content_diff = 1;
  /**/

  ++ opts->stats.compared;

  /* Stat the paths */
  if (lstat(p1, &sbuf1)<0)
    {
      xperror("cannot get inode status, lstat()", p1);
      rv = XIT_SYS;
    }
  if (lstat(p2, &sbuf2)<0)
    {
      xperror("cannot get inode status, lstat()", p2);
      rv = XIT_SYS;
    }
  if (rv != XIT_OK)
    return rv;

  subpath = p1+opts->root1_length;
  switch(*subpath)
    {
    case '\0': subpath = "(top-level)"; break;
    case '/':  subpath++;               break;
    default:   abort();                 break;
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
  ice = xmalloc(sizeof(ic_ent_t));
  memset(ice, 0, sizeof(*ice));
  ice->ino[0] = sbuf1.st_ino;
  ice->dev[0] = sbuf1.st_dev;
  ice->ino[1] = sbuf2.st_ino;
  ice->dev[1] = sbuf2.st_dev;
  icepath = xstrdup(subpath);
  if (! ic_put(opts->inocache, ice, icepath))
    {
      ++ opts->stats.skipped_cache;
      if (opts->verbosityLevel >= VERB_SKIPS)
	{
	  const char* ptr;
	  ptr = ic_get(opts->inocache, ice);
	  fprintf(stderr, "%s: %s: already compared hard-linked files at %s\n",
		 progname, icepath, ptr);
	}
      free(ice);
      free(icepath);
      return rv;
    }

  /* Generic perms, owner, etc... */
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
	  localerr = 1;
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
	  localerr = 1;					  \
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
	  localerr = 1;
	}
    }
#endif /* HAVE_ST_FLAGS */
  if (opts->owner && sbuf1.st_uid != sbuf2.st_uid)
    {
      const struct passwd* pw;
      char *pn1=NULL, *pn2=NULL;
      /**/
      if ((pw = getpwuid(sbuf1.st_uid))) pn1 = xstrdup(pw->pw_name);
      if ((pw = getpwuid(sbuf2.st_uid))) pn2 = xstrdup(pw->pw_name);

      printf("%s: %s: owner: %s(%ld) %s(%ld)\n",
	     progname,
	     subpath,
	     pn1 ? pn1 : "", (long)sbuf1.st_uid,
	     pn2 ? pn2 : "", (long)sbuf2.st_uid);

      if (pn1) free(pn1);
      if (pn2) free(pn2);

      localerr = 1;
    }
  if (opts->group && sbuf1.st_gid != sbuf2.st_gid)
    {
      const struct group* gr;
      char *gn1=NULL, *gn2=NULL;
      /**/
      if ((gr = getgrgid(sbuf1.st_gid))) gn1 = xstrdup(gr->gr_name);
      if ((gr = getgrgid(sbuf2.st_gid))) gn2 = xstrdup(gr->gr_name);

      printf("%s: %s: group: %s(%ld) %s(%ld)\n",
	     progname,
	     subpath,
	     gn1 ? gn1 : "", (long)sbuf1.st_gid,
	     gn2 ? gn2 : "", (long)sbuf2.st_gid);

      if (gn1) free(gn1);
      if (gn2) free(gn2);

      localerr = 1;
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
      localerr = 1;
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
      localerr = 1;
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
	  localerr = 1;
	}

      if (sbuf1.st_size != sbuf2.st_size)
	{
	  if (opts->size)
	    {
	      printf("%s: %s: size: %lld %lld\n",
		     progname,
		     subpath,
		     (long long)sbuf1.st_size, (long long)sbuf2.st_size);
	      localerr = 1;
	    }
	  content_diff = 0;
	}
    }

  if (opts->nlinks && sbuf1.st_nlink != sbuf2.st_nlink)
    {
      printf("%s: %s: nlinks: %ld %ld\n",
	     progname,
	     subpath,
	     (long)sbuf1.st_nlink, (long)sbuf2.st_nlink);
      localerr = 1;
    }

#if HAVE_GETXATTR
  if (opts->xattr)
    {
      strl_t *xl1 = getXattrList(p1);
      strl_t *xl2 = getXattrList(p2);
      commonClientData_t clientData;
      int nrv;
      /**/
      clientData.opts = opts;
      nrv = compareStrList(p1, p2,
			   xl1, xl2,
			   reportMissingXattr,
			   compareXattrs,
			   &clientData);
      if (nrv > rv)
	rv = nrv;
      freeStrList(xl1);
      freeStrList(xl2);
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
      if (nrv > rv)
	rv = nrv;
    }

  if (opts->acl
      && ((sbuf1.st_mode)&S_IFMT) == S_IFDIR
      && ((sbuf2.st_mode)&S_IFMT) == S_IFDIR)
    {
      int nrv = diffacl(opts, p1, p2, ACL_TYPE_DEFAULT, "default");
      if (nrv > rv)
	rv = nrv;
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
	  localerr = 1;
	}
    }
  else
    /* Type specific checks */
    switch ((sbuf1.st_mode)&S_IFMT)
      {
      case S_IFDIR:
	{
	  strl_t *ct1, *ct2;
	  int nrv;
	  commonClientData_t clientData;
	  /**/

	  ct1 = getDirList(p1);
	  ct2 = getDirList(p2);
	  clientData.opts = opts;
	  nrv = compareStrList(p1, p2,
			       ct1, ct2,
			       reportMissingFile,
			       compareFileEntries,
			       &clientData);
	  if (nrv > rv)
	    rv = nrv;

	  freeStrList(ct1);
	  freeStrList(ct2);
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
		      localerr = 1;
		  }
		else if (!cmpFiles(p1, sbuf1, p2, sbuf2))
		  {
		    printf("%s: %s: contents differ\n",
			   progname, subpath);
		    localerr = 1;
		  }
	      }
	    else
	      {
		printf("%s: %s: contents differ\n",
		       progname, subpath);
		localerr = 1;
	      }
	    if (opts->exec_always)
	      if (!execprocess(&opts->exec_always_args, p1, p2))
		localerr=1;
	  }
	break;
#if HAVE_S_IFLNK
      case S_IFLNK:
	if (opts->contents)
	  {
	    char *lnk1 = xreadlink(p1);
	    char *lnk2 = xreadlink(p2);
	    if (lnk1 && lnk2 && strcmp(lnk1, lnk2) != 0)
	      {
		printf("%s: %s: symbolic links differ\n",
		       progname, subpath);
		localerr = 1;
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
	    localerr = 1;
	  }
	if (opts->minor && minor(sbuf1.st_rdev) != minor(sbuf2.st_rdev))
	  {
	    printf("%s: %s: minor: %ld %ld\n",
		   progname,
		   subpath,
		   (long)minor(sbuf1.st_rdev),
		   (long)minor(sbuf2.st_rdev));
	    localerr = 1;
	  }
	break;
      default:
	break;
      }

  if (localerr && XIT_DIFF>rv) rv = XIT_DIFF;
  return rv;
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

  /* options.verbosityLevel = 0; */
  options.dirs		    = 1;
  options.type		    = 1;
  options.mode		    = 1;
  options.flags		    = 1;
  options.owner		    = 1;
  options.group		    = 1;
  /* options.ctime	    = 0 ; */
  /* options.mtime	    = 0 ; */
  /* options.atime	    = 0 ; */
  options.size		    = 1;
  options.blocks	    = 1;
  options.contents	    = 1;
  options.nlinks	    = 1;
  options.major		    = 1;
  options.minor		    = 1;
  options.xattr		    = 1;
  options.acl		    = 1;
  /* options.exec           = 0; */
  /* options.exec_always    = 0; */
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
#if HAVE_ST_FLAGS
	{ "flags",             0, 0, 'f' },
	{ "no-flags",          0, 0, 'F' },
#endif
	{ "owner",             0, 0, 'o' },
	{ "no-owner",          0, 0, 'O' },
	{ "group",             0, 0, 'g' },
	{ "no-group",          0, 0, 'G' },
	{ "ctime",             0, 0, 'z' },
	{ "no-ctime",          0, 0, 'Z' },
	{ "mtime",             0, 0, 'i' },
	{ "no-mtime",          0, 0, 'I' },
	{ "atime",             0, 0, 'r' },
	{ "no-atime",          0, 0, 'R' },
	{ "size",              0, 0, 's' },
	{ "no-size",           0, 0, 'S' },
	{ "blocks",            0, 0, 'b' },
	{ "no-blocks",         0, 0, 'B' },
	{ "contents",          0, 0, 'c' },
	{ "no-contents",       0, 0, 'C' },
	{ "nlinks",            0, 0, 'n' },
	{ "no-nlinks",         0, 0, 'N' },
	{ "major",             0, 0, 'j' },
	{ "no-major",          0, 0, 'J' },
	{ "minor",             0, 0, 'k' },
	{ "no-minor",          0, 0, 'K' },
#if HAVE_GETXATTR
	{ "xattr",             0, 0, 'q' },
	{ "no-xattr",          0, 0, 'Q' },
#endif
#if HAVE_GETXATTR
	{ "acl",               0, 0, 'l' },
	{ "no-acl",            0, 0, 'L' },
#endif
	{ "all",               0, 0, 'a' },
	{ "nothing",           0, 0, 'A' },
	{ "no-all",            0, 0, 'A' },
	{ "exec",              0, 0, 'x' },
	{ "exec-always",       0, 0, 'w' },
	{ "exec-always-diff",  0, 0, 'W' },
	{ "mode-or",           1, 0, '|' },
	{ "mode-and",          1, 0, '&' },
	{ "exclude",           1, 0, 'X' },
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
#if HAVE_ST_FLAGS
	      "fF"
#endif
	      "oOgGzZiIrRsSbBcCnNjJkK"
#if HAVE_GETXATTR
	      "qQ"
#endif
#if HAVE_ACL
	      "lL"
#endif
	      "xwWaA|:&:X:"
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
#if HAVE_ST_FLAGS
	case 'f': options.flags           = 1; break;
	case 'F': options.flags           = 0; break;
#endif
	case 'o': options.owner		  = 1; break;
	case 'O': options.owner		  = 0; break;
	case 'g': options.group		  = 1; break;
	case 'G': options.group		  = 0; break;
	case 'z': options.ctime		  = 1; break;
	case 'Z': options.ctime		  = 0; break;
	case 'i': options.mtime		  = 1; break;
	case 'I': options.mtime		  = 0; break;
	case 'r': options.atime		  = 1; break;
	case 'R': options.atime		  = 0; break;
	case 's': options.size		  = 1; break;
	case 'S': options.size		  = 0; break;
	case 'b': options.blocks	  = 1; break;
	case 'B': options.blocks	  = 0; break;
	case 'c': options.contents	  = 1; break;
	case 'C': options.contents	  = 0; break;
	case 'j': options.major		  = 1; break;
	case 'J': options.major		  = 0; break;
	case 'n': options.nlinks	  = 1; break;
	case 'N': options.nlinks	  = 0; break;
	case 'k': options.minor		  = 1; break;
	case 'K': options.minor		  = 0; break;
#if HAVE_GETXATTR
	case 'q': options.xattr		  = 1; break;
	case 'Q': options.xattr		  = 0; break;
#endif
#if HAVE_ACL
	case 'l': options.acl		  = 1; break;
	case 'L': options.acl		  = 0; break;
#endif
	case 'a': options.dirs = options.type
		    = options.mode = options.flags = options.owner
		    = options.group
		    /* = options.ctime = options.mtime  = options.atime */
		    = options.size  = options.blocks = options.contents
		    = options.nlinks
		    = options.major = options.minor = options.xattr
		    = options.acl = 1; break;
	case 'A': options.dirs = options.type
		    = options.mode = options.flags = options.owner
		    = options.group
		    = options.ctime = options.mtime  = options.atime /* extra for -A */
		    = options.size  = options.blocks = options.contents
		    = options.nlinks
		    = options.major = options.minor = options.xattr
		    = options.acl = 0; break;
	case 'x':
	  if (options.exec)
	    {
	      fprintf(stderr, "%s: -x specified twice, only the last command line will run\n", progname);
	    }
	  if (get_exec_args(argv, &optind, &options.exec_args))
	    options.contents = options.exec = 1;
	  else
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
	      if (get_exec_args(argv, &optind, &options.exec_always_args))
		options.contents = options.exec_always = 1;
	      else
		end_after_options = EAO_error;
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
	case '|':
	  if (!get_octal_arg(optarg, &options.mode_or))
	    end_after_options = EAO_error;
	  break;
	case '&':
	  if (!get_octal_arg(optarg, &options.mode_and))
	    end_after_options = EAO_error;
	  break;
	case 'X':
	  gh_insert(options.exclusions, xstrdup(optarg), NULL);
	  break;
	default:
	  abort();
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

  options.inocache = ic_new();

  rv = dodiff(&options, argv[0], argv[1]);

  if (options.exec)
    free(options.exec_args.argv);
  if (options.exec_always)
    free(options.exec_always_args.argv);
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
    }

  if ( options.verbosityLevel >= VERB_MEM_STATS)
    {
      fprintf(stderr, "%s: memory stats before releasing inode cache:\n",
	      progname);
      pmem();
      fprintf(stderr, "%s: end\n", progname);
    }

  ic_delete(options.inocache);

  if ( options.verbosityLevel >= VERB_MEM_STATS)
    {
      fprintf(stderr, "%s: memory stats before exit:\n", progname);
      pmem();
      fprintf(stderr, "%s: end\n", progname);
    }

  exit(rv);
}
