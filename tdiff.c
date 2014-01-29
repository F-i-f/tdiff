/*
  tdiff - tree diffs
  Main()
  Copyright (C) 1999, 2008, 2014 Philippe Troin <phil@fifi.org>

  $Id$

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#include "tdiff.h"
#include "config.h"
#include "utils.h"
#include "genhash.h"
#include "inocache.h"

#if HAVE_GETDENTS
#  if HAVE_GETDENTS_SYSCALL_H
#    include <errno.h>
#    include <syscall.h>
     struct dent {
       long           d_ino;
       off_t          d_off;
       unsigned short d_reclen;
       char           d_name[NAME_MAX+1];
     };
     int
     getdents(int fd, char* buf, unsigned count)
     {
       return syscall(SYS_getdents, fd, buf, count);
     }
#  elif HAVE_SYS_DIRENT_H
#    include <sys/dirent.h>
#  else
#    error HAVE_GETDENTS is set, but i do not know how to get it !!!
#  endif
#endif

#if HAVE_MMAP
#  include <sys/mman.h>
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
#else
#  error Cannot find major() and minor()
#endif

#if HAVE_GETXATTR
#include <attr/xattr.h>
#endif

#define GETSTRLIST_INITIAL_SIZE 8
#define XATTR_BUF_SIZE 16384
#define GETDIRLIST_DENTBUF_SIZE 8192
#define XREADLINK_BUF_SIZE 1024
#define CMPFILE_BUF_SIZE 16384

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

typedef struct option_s
{
  unsigned int verbose:1;
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
  unsigned int major:1;
  unsigned int minor:1;
  unsigned int xattr:1;
  unsigned int nommap:1;
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
} options_t;

strl_t *getDirList(const char* path);
void freeStrlist(strl_t *d);
int get_exec_args(char**, int*, dexe_t*);
int dodiff(const options_t* opt, const char* p1, const char* p2);
char* pconcat(const char* p1, const char* p2);
int execprocess(const dexe_t *dex, const char* p1, const char* p2);

#if DEBUG
void printopts(const options_t*);
#else
#  define printopts(a)
#endif

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
  int i;
  /**/
  for (i=0; i<d->size; ++i)
    free((char*)d->strings[i]);
  free(d->strings);
  free(d);
}

int
compareStrList(const char *p1, const char* p2,
	       const strl_t *ct1, const strl_t *ct2,
	       void (*reportMissing)(int, const char*, const char*, void*),
	       int (*compareEntries)(const char* p1, const char* p2,
				     const char* e,
				     void*),
	       void *clientData)
{
  int	i1, i2;
  int	cmpres;
  int	rv	 = XIT_OK;
  int	localerr = 0;
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
	  nrv = compareEntries(p1, p2, ct1->strings[i1], clientData);
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
	return rv;
      case ERANGE:
	free(buf);
	buf = xmalloc(bufSize *= 2);
	goto again;
      default:
	perror(path);
	free(buf);
	return NULL;
      }

  newStrList(&rv);
  for (p = buf, p_e = buf+rSize;
       p < p_e && *p;
       p += strlen(p)+1)
    pushStrList(rv, p);

  return rv;
}

typedef struct xattrCompareClientData_s
{
  const options_t *opt;
} xattrCompareClientData_t;

static void
reportMissingXattr(int which, const char* f, const char* xn, void* voidClientData)
{
  xattrCompareClientData_t*	clientData = (xattrCompareClientData_t*)voidClientData;
  const char*			subp;
  int				rootlen;
  /**/
  switch(which)
    {
    case 1:
      subp = f+(rootlen = clientData->opt->root1_length)+1;
      break;
    case 2:
      subp = f+(rootlen = clientData->opt->root2_length)+1;
      break;
    default:
      abort();
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
	      const char* e,
	      void* voidClientData)
{
  xattrCompareClientData_t* clientData = (xattrCompareClientData_t*)voidClientData;
  int rv = XIT_OK;
  size_t sz1 = 0;
  size_t sz2 = 0;
  void* buf1 = getXattr(p1, e, &sz1);
  void* buf2 = getXattr(p2, e, &sz2);

  if (!buf1)
    {
      rv = XIT_SYS;
      perror(p1);
      goto ret;
    }

  if (!buf2)
    {
      rv = XIT_SYS;
      perror(p2);
      goto ret;
    }

  if (sz1 != sz2 || memcmp(buf1, buf2, sz1))
    {
      printf("%s: %s: xattr %s: contents differ\n",
	     progname, p1+clientData->opt->root1_length+1, e);
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
#endif

/*
 * Directory comparisons
 */
strl_t *
getDirList(const char* path)
#if HAVE_GETDENTS
{
  union dent_u
  {
    struct GETDENTS_STRUCT f_dent;
    char                   f_byte[GETDIRLIST_DENTBUF_SIZE];
  } dentbuf;
  union dentptr_u
  {
    struct GETDENTS_STRUCT *f_dent;
    char                   *f_byte;
  };
  int fd;
  strl_t *rv = NULL;
  int nread;
  /**/
  fd = open(path, O_RDONLY);
  if (fd<0)
    goto err;

  newStrList(&rv);

  while((nread = getdents(fd, (char*)&dentbuf.f_dent,
			  GETDIRLIST_DENTBUF_SIZE))>0)
    {
      union dentptr_u dent;
      /**/
      for (dent.f_byte = dentbuf.f_byte;
	   dent.GETDENTS_RETURN_Q-dentbuf.GETDENTS_RETURN_Q<nread;
#ifdef GETDENTS_NEXTDENT
	  dent.GETDENTS_RETURN_Q += dent.f_dent->GETDENTS_NEXTDENT)
#else
	  dent.f_dent++)
#endif
	{
	  if (!dent.f_dent->d_name
	      || (dent.f_dent->d_name[0]=='.'
		  && (dent.f_dent->d_name[1] == 0
		      || (dent.f_dent->d_name[1]=='.'
			  && dent.f_dent->d_name[2]== 0))))
	    continue;

	  pushStrList(rv, dent.f_dent->d_name);
	}
    }

  if (nread<0 || close(fd)<0)
    goto err;

  return rv;

 err:
  perror(path);
  return rv;
}
#else
{
  DIR *dir;
  struct dirent *dent;
  strl_t *rv = NULL;
  /**/
  dir = opendir(path);
  if (!dir)
    goto err;

  newStrList(&rv);

  while ((dent = readdir(dir)))
    {
      if (!dent->d_name
	  || (dent->d_name[0]=='.'
	      && (dent->d_name[1] == 0
		  || (dent->d_name[1]=='.'
		      && dent->d_name[2]== 0))))
	continue;

      pushStrList(rv, dent->d_name);
    }

  if (closedir(dir))
    goto err;

  return rv;

 err:
  perror(path);
  return rv;
}
#endif

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

int
cmpFiles(const options_t *opt, const char* f1, const char* f2)
{
  enum { res_untested, res_same, res_diff } result;
  int fd1, fd2;
  off_t fs1, fs2;
#if HAVE_MMAP
  caddr_t ptr1, ptr2;
#endif
  /**/

  if ((fd1 = open(f1, O_RDONLY))<0)
    {
      perror(f1);
      return 0;
    }
  if ((fs1=lseek(fd1, 0, SEEK_END))<0)
    {
      perror(f1);
      return 0;
    }
  if (lseek(fd1, 0, SEEK_SET)<0)
    {
      perror(f1);
      return 0;
    }
  if ((fd2 = open(f2, O_RDONLY))<0)
    {
      perror(f2);
      return 0;
    }
  if ((fs2=lseek(fd2, 0, SEEK_END))<0)
    {
      perror(f2);
      return 0;
    }
  if (lseek(fd2, 0, SEEK_SET)<0)
    {
      perror(f2);
      return 0;
    }
  if (fs1 != fs2)
    {
      fprintf(stderr, "%s: cmpFiles called on files of different sizes\n",
	      progname);
      exit(XIT_INTERNALERROR);
    }

  if (fs1 == 0)
    {
      result = res_same;
      goto closefiles;
    }

  result = res_untested;

  if (opt->nommap)
    goto map1_failed;

#if HAVE_MMAP
  if (!(ptr1 = mmap(NULL, fs1, PROT_READ, MAP_SHARED, fd1, 0)))
    goto map1_failed;
  if (!(ptr2 = mmap(NULL, fs2, PROT_READ, MAP_SHARED, fd2, 0)))
    goto map2_failed;

#if HAVE_MADVISE && defined(MADV_SEQUENTIAL)
  if (madvise(ptr1, fs1, MADV_SEQUENTIAL)<0)
    perror(f1);
  if (madvise(ptr2, fs2, MADV_SEQUENTIAL)<0)
    perror(f2);
#endif /* HAVE_MADVISE */

#if HAVE_MADVISE && defined(MADV_WILLNEED)
  if (madvise(ptr1, fs1, MADV_WILLNEED)<0)
    perror(f1);
  if (madvise(ptr2, fs2, MADV_WILLNEED)<0)
    perror(f2);
#endif /* HAVE_MADVISE */

  result = memcmp(ptr1, ptr2, fs1)==0 ? res_same : res_diff;

  if (munmap(ptr2, fs2)<0)
    perror(f2);

 map2_failed:
  if (munmap(ptr1, fs1)<0)
    perror(f1);
#endif /* HAVE_MMAP */

 map1_failed:
  if (result == res_untested)
    {
      char buf1[CMPFILE_BUF_SIZE];
      char buf2[CMPFILE_BUF_SIZE];
      ssize_t toread;
      /**/

      for (toread = fs1; toread>0; )
	{
	  int nread1, nread2;
	  /**/
	  nread1 = read(fd1, buf1, CMPFILE_BUF_SIZE);
	  if (nread1 < 0)
	    {
	      perror(f1);
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
	      perror(f2);
	      goto fail;
	    }
	  if (nread2 < CMPFILE_BUF_SIZE && nread2 != toread)
	    {
	      fprintf(stderr, "%s: %s: short read\n", progname, f2);
	      goto fail;
	    }
	  if (nread1 != nread2)
	    {
	      fprintf(stderr, "%s: read different number of bytes\n",
		      progname);
	      goto fail;
	    }
	  if (memcmp(buf1, buf2, nread1)!=0)
	    {
	      result = res_diff;
	      break;
	    }
	  toread -= nread1;
	}
      if (result==res_untested)
	result = res_same;
    }

 closefiles:
  if (close(fd1)<0)
    {
      perror(f1);
      goto fail;
    }
  if (close(fd2)<0)
    {
      perror(f2);
      goto fail;
    }

  switch(result)
    {
    case res_same:
      return 1;
    case res_diff:
      return 0;
    default:
      fprintf(stderr, "%s: programming error in cmpFiles\n", progname);
      exit(XIT_INTERNALERROR);
    }

 fail:
  close(fd1);
  close(fd2);
  return 0;
}

void
show_version(void)
{
  printf("tdiff version 0.1 (CVS $Id$)\n"
	 "Copyright (C) 1999 Philippe Troin.\n"
	 "Tdiff comes with ABSOLUTELY NO WARRANTY.\n"
	 "This is free software, and you are welcome to redistribute it\n"
	 "under certain conditions.\n"
	 "You should have received a copy of the GNU General Public License\n"
	 "along with this program; if not, write to the Free Software\n"
	 "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n");
}

void
show_help(void)
{
  printf("usage: %s [options]... <dir1> <dir2>\n"
	 " Standard options:\n"
	 "   -v --verbose: tell more about inode cached comparisons\n"
	 "   -V --version: show %s version\n"
	 "   -h --help\n"
	 " Switch options:\n"
	 "   -d --dirs     diff directories (reports missing files)\n"
	 "   -t --type     diffs for file type differences\n"
	 "                 (if unset, also size, blocks, major, minor and contents\n"
	 "                  are not checked either)\n"
	 "   -m --mode     diffs file modes (permissions)\n"
#if HAVE_ST_FLAGS
	 "   -f --flags    diffs flags (4.4BSD)\n"
#endif
	 "   -o --owner    diffs file owner\n"
	 "   -g --group    diffs file groups\n"
	 "   -z --ctime    diffs ctime (inode modification time)\n"
	 "   -i --mtime    diffs mtime (contents modification time)\n"
	 "   -r --atime    diffs atime (access time)\n"
	 "   -s --size     diffs file size (for regular files, symlinks)\n"
	 "   -b --blocks   diffs file blocks (for regular files,symlinks and directories\n"
	 "   -c --contents diffs file contents (for regular files and symlinks)\n"
	 "   -j --major    diffs major device numbers (for device files)\n"
	 "   -n --minor    diffs minor device numbers (for device files)\n"
#if HAVE_GETXATTR
	 "   -q --xattr    diffs file extended attributes\n"
#endif
	 "  Each of these options can be negated with an uppercase (short option)\n"
	 "  or with --no-option (eg -M --no-mode for not diffing modes\n"
	 "   -a --all      equivalent to -dtmogsbcj (sets all but times)\n"
	 "   -A --no-all   clears all flags (will not report anything)\n"
	 " Miscellania:\n"
	 "   -x --exec <cmd>;         executes <cmd> between files if they are similar\n"
	 "                            (if file sizes are equal)\n"
	 "   -w --exec-always <cmd>;  always executes <cmd> between files\n"
	 "        <cmd> uses %%1 and %%2 as file from <dir1> and <dir2>\n"
	 "   -| --mode-or <bits>   applies <bits> OR mode before comparison\n"
	 "   -& --mode-and <bits>  applies <bits> AND mode before comparison\n"
	 "   -X --exclude <file>   omits <file> from report\n"
	 "   -p --no-mmap          do not use mmap() for comparisons (saves on vmem\n"
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
  return 1;
}

int
get_numeric_arg(const char* string, unsigned int* val)
{
  size_t len;
  char *ptr;
  unsigned long m;

  len = strlen(string);
  if (len >=1 && string[0]=='0')
    {
      if (len >= 2 && string[1]=='x')
	m = strtoul(&string[2], &ptr, 16);
      else
	m = strtoul(&string[1], &ptr, 8);
    }
  else
    m = strtoul(&string[0], &ptr, 10);

  if (*ptr)
    {
      fprintf(stderr, "%s: not a number \"%s\"\n", progname, string);
      return 0;
    }
  else
    {
      *val = m;
      return 1;
    }
}

#if DEBUG
void
printopts(const options_t* o)
{
#define POPT(x) printf(#x "= %s\n", o->x ? "yes" : "no")
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
  POPT(major);
  POPT(minor);
  POPT(xattr);
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
  *(dex->arg1) = (char*)p1;
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
      perror(path);
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

typedef struct fileCompareClientData_s
{
  const options_t *opt;
} fileCompareClientData_t;

static void
reportMissingFile(int which, const char* d, const char *f, void* voidClientData)
{
  fileCompareClientData_t *clientData = (fileCompareClientData_t*)voidClientData;
  if ( clientData->opt->dirs && !gh_find(clientData->opt->exclusions, f, NULL) )
    {
      const char*	subp;
      int		rootlen;
      size_t		fp_len = strlen(f)+strlen(d)+1;
      char		fp[fp_len];

      switch(which)
	{
	case 1:
	  subp = d+(rootlen = clientData->opt->root1_length);
	  break;
	case 2:
	  subp = d+(rootlen = clientData->opt->root2_length);
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
      snprintf(fp, fp_len, "%s%s%s", subp, *subp ? "/" : "", f);
      printf("%s: %s: only present in %.*s\n",
	     progname, fp, rootlen, d);
    }
}

static int
compareFileEntries(const char* p1, const char* p2,
		   const char* e,
		   void* voidClientData)
{
  fileCompareClientData_t *clientData = (fileCompareClientData_t*)voidClientData;
  int rv = XIT_OK;
  /**/
  if (!gh_find(clientData->opt->exclusions, e, NULL))
    {
      char *np1 = pconcat(p1, e);
      char *np2 = pconcat(p2, e);
      rv = dodiff(clientData->opt, np1, np2);
      free(np1);
      free(np2);
    }
  return rv;
}

int
dodiff(const options_t* opt, const char* p1, const char* p2)
{
  struct stat sbuf1;
  struct stat sbuf2;
  ic_ent_t *ice;
  char *icepath;
  int rv = XIT_OK;
  int localerr = 0;
  /**/

  /* Stat the paths */
  if (lstat(p1, &sbuf1)<0)
    {
      perror(p1);
      rv = XIT_SYS;
    }
  if (lstat(p2, &sbuf2)<0)
    {
      perror(p2);
      rv = XIT_SYS;
    }
  if (rv != XIT_OK)
    return rv;

  /* Check if we're comparing the same dev/inode pair */
  if (sbuf1.st_ino == sbuf2.st_ino && sbuf1.st_dev == sbuf2.st_dev)
    {
      if (opt->verbose)
	printf("%s: %s: same dev/ino pair, skipping\n",
	       progname, p1+opt->root1_length+1);
      if (opt->exec_always && S_ISREG(sbuf1.st_mode))
	if (!execprocess(&opt->exec_always_args, p1, p2))
	  rv=XIT_DIFF;
      return rv;
    }

  /* Check if we have compared these two guys before */
  ice = xmalloc(sizeof(ic_ent_t));
  ice->ino[0] = sbuf1.st_ino;
  ice->dev[0] = sbuf1.st_dev;
  ice->ino[1] = sbuf2.st_ino;
  ice->dev[1] = sbuf2.st_dev;
  icepath = xstrdup(p1+opt->root1_length+1);
  if (! ic_put(opt->inocache, ice, icepath))
    {
      const char* ptr;
      ptr = ic_get(opt->inocache, ice);
      if (opt->verbose)
	printf("%s: %s: already compared hard-linked files at %s\n",
	       progname, icepath, ptr);
      free(ice);
      free(icepath);
      return rv;
    }

  /* Generic perms, owner, etc... */
  if (opt->mode)
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

      mm1 = (rm1&(opt->mode_and))|(opt->mode_or);
      mm2 = (rm2&(opt->mode_and))|(opt->mode_or);

      if (mm1 != mm2)
	{
	  printf("%s: %s: mode: %04o %04o (masked %04o %04o)\n",
		 progname, p1+opt->root1_length+1, rm1, rm2, mm1, mm2);
	  localerr = 1;
	}
    }
#if HAVE_ST_FLAGS
  if (opt->flags && sbuf1.st_flags != sbuf2.st_flags)
    {
      printf("%s: %s: flags: %08X %08X\n",
	     progname, p1+opt->root1_length+1, sbuf1.st_flags, sbuf2.st_flags);
      localerr = 1;
    }
#endif /* HAVE_ST_FLAGS */
  if (opt->owner && sbuf1.st_uid != sbuf2.st_uid)
    {
      const struct passwd* pw;
      char *pn1=NULL, *pn2=NULL;
      /**/
      if ((pw = getpwuid(sbuf1.st_uid))) pn1 = xstrdup(pw->pw_name);
      if ((pw = getpwuid(sbuf2.st_uid))) pn2 = xstrdup(pw->pw_name);

      printf("%s: %s: owner: %s(%ld) %s(%ld)\n",
	     progname,
	     p1+opt->root1_length+1,
	     pn1 ? pn1 : "", (long)sbuf1.st_uid,
	     pn2 ? pn2 : "", (long)sbuf2.st_uid);

      if (pn1) free(pn1);
      if (pn2) free(pn2);

      localerr = 1;
    }
  if (opt->group && sbuf1.st_gid != sbuf2.st_gid)
    {
      const struct group* gr;
      char *gn1=NULL, *gn2=NULL;
      /**/
      if ((gr = getgrgid(sbuf1.st_gid))) gn1 = xstrdup(gr->gr_name);
      if ((gr = getgrgid(sbuf2.st_gid))) gn2 = xstrdup(gr->gr_name);

      printf("%s: %s: group: %s(%ld) %s(%ld)\n",
	     progname,
	     p1+opt->root1_length+1,
	     gn1 ? gn1 : "", (long)sbuf1.st_gid,
	     gn2 ? gn2 : "", (long)sbuf2.st_gid);

      if (gn1) free(gn1);
      if (gn2) free(gn2);

      localerr = 1;
    }
  if (opt->ctime && sbuf1.st_ctime != sbuf2.st_ctime
#if HAVE_ST_XTIMESPEC
      && sbuf1.st_ctimespec.tv_nsec != sbuf2.st_ctimespec.tv_nsec
#endif
      )
    {
      char *t1 = xstrdup(ctime(&sbuf1.st_ctime));
      char *t2 = xstrdup(ctime(&sbuf2.st_ctime));
      t1[strlen(t1)-1] = 0;
      t2[strlen(t2)-1] = 0;
      printf("%s: %s: ctime: [%s] [%s]\n",
	     progname, p1+opt->root1_length+1, t1, t2);
      free(t1);
      free(t2);
      localerr = 1;
    }
  if (opt->mtime && sbuf1.st_mtime != sbuf2.st_mtime
#if HAVE_ST_XTIMESPEC
      && sbuf1.st_mtimespec.tv_nsec != sbuf2.st_mtimespec.tv_nsec
#endif
      )
    {
      char *t1 = xstrdup(ctime(&sbuf1.st_mtime));
      char *t2 = xstrdup(ctime(&sbuf2.st_mtime));
      t1[strlen(t1)-1] = 0;
      t2[strlen(t2)-1] = 0;
      printf("%s: %s: mtime: [%s] [%s]\n",
	     progname, p1+opt->root1_length+1, t1, t2);
      free(t1);
      free(t2);
      localerr = 1;
    }
  if (opt->atime && sbuf1.st_atime != sbuf2.st_atime
#if HAVE_ST_XTIMESPEC
      && sbuf1.st_atimespec.tv_nsec != sbuf2.st_atimespec.tv_nsec
#endif
      )
    {
      char *t1 = xstrdup(ctime(&sbuf1.st_atime));
      char *t2 = xstrdup(ctime(&sbuf2.st_atime));
      t1[strlen(t1)-1] = 0;
      t2[strlen(t2)-1] = 0;
      printf("%s: %s: atime: [%s] [%s]\n",
	     progname, p1+opt->root1_length+1, t1, t2);
      free(t1);
      free(t2);
      localerr = 1;
    }

#if HAVE_GETXATTR
  if (opt->xattr)
    {
      strl_t *xl1 = getXattrList(p1);
      strl_t *xl2 = getXattrList(p2);
      xattrCompareClientData_t clientData;
      int nrv;
      /**/
      clientData.opt = opt;
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

  /* Type tests */
  if (opt->type && ((sbuf1.st_mode)&S_IFMT) != ((sbuf2.st_mode)&S_IFMT))
    {
      printf("%s: %s: type: %s %s\n",
	     progname,
	     p1+opt->root1_length+1,
	     getFileType(sbuf1.st_mode), getFileType(sbuf2.st_mode));
      localerr = 1;
    }
  else
    /* Type specific checks */
    switch ((sbuf1.st_mode)&S_IFMT)
      {
      case S_IFDIR:
	{
	  strl_t *ct1, *ct2;
	  int nrv;
	  fileCompareClientData_t clientData;
	  /**/
	  ct1 = getDirList(p1);
	  ct2 = getDirList(p2);
	  clientData.opt = opt;
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
	{
	  int content_diff = 1;
	  /**/
	  if (opt->blocks && sbuf1.st_blocks != sbuf2.st_blocks)
	    {
	      printf("%s: %s: blocks: %ld %ld\n",
		     progname,
		     p1+opt->root1_length+1,
		     (long)sbuf1.st_blocks, (long)sbuf2.st_blocks);
	      localerr = 1;
	    }
	  if (sbuf1.st_size != sbuf2.st_size)
	    {
	      if (opt->size)
		{
		  printf("%s: %s: size: %lld %lld\n",
			 progname,
			 p1+opt->root1_length+1,
			 (long long)sbuf1.st_size, (long long)sbuf2.st_size);
		  localerr = 1;
		}
	      content_diff = 0;
	    }
	  if (opt->contents)
	    {
	      if (content_diff)
		{
		  if (opt->exec)
		    {
		      if (!execprocess(&opt->exec_args, p1, p2))
			localerr = 1;
		    }
		  else if (!cmpFiles(opt, p1, p2))
		    {
		      printf("%s: %s: contents differ\n",
			     progname, p1+opt->root1_length+1);
		      localerr = 1;
		    }
		}
	      else
		{
		  printf("%s: %s: contents differ\n",
			 progname, p1+opt->root1_length+1);
		  localerr = 1;
	      }
	      if (opt->exec_always)
		if (!execprocess(&opt->exec_always_args, p1, p2))
		  localerr=1;
	    }
	}
	break;
#if HAVE_S_IFLNK
      case S_IFLNK:
	{
	  char *lnk1 = xreadlink(p1);
	  char *lnk2 = xreadlink(p2);
	  if (lnk1 && lnk2)
	    if (strcmp(lnk1, lnk2)!=0)
	      {
		printf("%s: %s: symbolic links differ\n",
		       progname, p1+opt->root1_length+1);
		localerr = 1;
	      }
	  if (lnk1) free(lnk1);
	  if (lnk2) free(lnk2);
	}
	break;
#endif /* HAVE_S_IFLNK */
      case S_IFBLK:
      case S_IFCHR:
	if (opt->major && major(sbuf1.st_rdev) != major(sbuf2.st_rdev))
	  {
	    printf("%s: %s: major: %ld %ld\n",
		   progname,
		   p1+opt->root1_length+1,
		   (long)major(sbuf1.st_rdev),
		   (long)major(sbuf2.st_rdev));
	    localerr = 1;
	  }
	if (opt->minor && minor(sbuf1.st_rdev) != minor(sbuf2.st_rdev))
	  {
	    printf("%s: %s: minor: %ld %ld\n",
		   progname,
		   p1+opt->root1_length+1,
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
  options_t options = { 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, ~0, NULL};
  enum { EAO_no, EAO_ok, EAO_error } end_after_options = EAO_no;
  int rv;
  /**/

  pmem();

  setprogname(argv[0]);

  options.exclusions = gh_new(&gh_string_hash, gh_string_equal, &free, NULL);

  /* For getopt */
  argv[0] = (char*)progname;

  while(1)
    {
#if HAVE_GETOPT_LONG
      struct option long_options[] = {
	{ "verbose",     0, 0, 'v' },
	{ "version",     0, 0, 'V' },
	{ "help",        0, 0, 'h' },
	{ "dirs",        0, 0, 'd' },
	{ "no-dirs",     0, 0, 'D' },
	{ "type",        0, 0, 't' },
	{ "no-type",     0, 0, 'T' },
	{ "mode",        0, 0, 'm' },
	{ "no-mode",     0, 0, 'M' },
#if HAVE_ST_FLAGS
	{ "flags",       0, 0, 'f' },
	{ "no-flags",    0, 0, 'F' },
#endif
	{ "owner",       0, 0, 'o' },
	{ "no-owner",    0, 0, 'O' },
	{ "group",       0, 0, 'g' },
	{ "no-group",    0, 0, 'G' },
	{ "ctime",       0, 0, 'z' },
	{ "no-ctime",    0, 0, 'Z' },
	{ "mtime",       0, 0, 'i' },
	{ "no-mtime",    0, 0, 'I' },
	{ "atime",       0, 0, 'r' },
	{ "no-atime",    0, 0, 'R' },
	{ "size",        0, 0, 's' },
	{ "no-size",     0, 0, 'S' },
	{ "blocks",      0, 0, 'b' },
	{ "no-blocks",   0, 0, 'B' },
	{ "contents",    0, 0, 'c' },
	{ "no-contents", 0, 0, 'C' },
	{ "major",       0, 0, 'j' },
	{ "no-major",    0, 0, 'J' },
	{ "minor",       0, 0, 'n' },
	{ "no-minor",    0, 0, 'N' },
#if HAVE_GETXATTR
	{ "xattr",       0, 0, 'q' },
	{ "no-xattr",    0, 0, 'Q' },
#endif
	{ "all",         0, 0, 'a' },
	{ "nothing",     0, 0, 'A' },
	{ "no-all",      0, 0, 'A' },
	{ "exec",        0, 0, 'x' },
	{ "exec-always", 0, 0, 'w' },
	{ "no-mmap",     0, 0, 'p' },
	{ "mode-or",     1, 0, '|' },
	{ "mode-and",    1, 0, '&' },
	{ "exclude",     1, 0, 'X' },
	{ 0,             0, 0, 0}
      };
#endif /* HAVE_GETOPT_LONG */
      switch(
#if HAVE_GETOPT_LONG
	     getopt_long
#else
	     getopt
#endif
	     (argc, argv, "vVhdDtTmM"
#if HAVE_ST_FLAGS
	      "fF"
#endif
	      "oOgGzZiIrRsSbBcCjJnN"
#if HAVE_GETXATTR
	      "qQ"
#endif
	      "xwaAp|:&:X:W"
#if HAVE_GETOPT_LONG
	      , long_options, NULL
#endif
	      ))
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
	case 'v': options.verbose         = 1; break;
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
	case 'n': options.minor		  = 1; break;
	case 'N': options.minor		  = 0; break;
#if HAVE_GETXATTR
	case 'q': options.xattr		  = 1; break;
	case 'Q': options.xattr		  = 0; break;
#endif
	case 'a': options.dirs = options.type
		    = options.mode = options.flags = options.owner
		    = options.group
		    /* = options.ctime = options.mtime  = options.atime */
		    = options.size  = options.blocks = options.contents
		    = options.major = options.minor = options.xattr = 1; break;
	case 'A': options.dirs = options.type
		    = options.mode = options.flags = options.owner
		    = options.group
		    /* = options.ctime = options.mtime  = options.atime */
		    = options.size  = options.blocks = options.contents
		    = options.major = options.minor = options.xattr = 0; break;
	case 'p': options.nommap = 1; break;
	case 'x':
	  if (options.exec)
	    {
	      fprintf(stderr, "%s: cannot specify -x twice\n", progname);
	      end_after_options = EAO_error;
	    }
	  else if (get_exec_args(argv, &optind, &options.exec_args))
	    options.contents = options.exec = 1;
	  else
	    end_after_options = EAO_error;
	  break;
	case 'w':
	  if (options.exec_always)
	    {
	      fprintf(stderr, "%s: cannot specify -w twice\n", progname);
	      end_after_options = EAO_error;
	    }
	  else if (get_exec_args(argv, &optind, &options.exec_always_args))
	    options.contents = options.exec_always = 1;
	  else
	    end_after_options = EAO_error;
	  break;
	case '|':
	  if (!get_numeric_arg(optarg, &options.mode_or))
	    end_after_options = EAO_error;
	  break;
	case '&':
	  if (!get_numeric_arg(optarg, &options.mode_and))
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

#if DEBUG
  gh_stats(options.inocache, "inode cache");
#endif
  ic_delete(options.inocache);

  pmem();

  exit(rv);
}
