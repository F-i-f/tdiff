#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#include "config.h"

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
     _syscall3(int, getdents, uint, fd, struct dent*, buf, uint, count);
     int getdents(unsigned int fd, struct dent* buf, unsigned int count);
#  elif HAVE_SYS_DIRENT_H
#    include <sys/dirent.h>
#  else
#    error HAVE_GETDENTS is set, but i do not know how to get it !!!
#  endif
#endif

#if HAVE_MMAP
#  include <sys/mman.h>
#endif

#if HAVE_MALLINFO
#  include <malloc.h>
#endif

#if HAVE_GETOPT_LONG
#  include <getopt.h>
#endif

#define XIT_OK            0
#define XIT_INVOC         1
#define XIT_DIFF          2
#define XIT_SYS           3
#define XIT_INTERNALERROR 4

#define GETDIRLIST_INITIAL_SIZE 8
#define GETDIRLIST_DENTBUF_SIZE 8192
#define CMPFILE_BUF_SIZE 16384

char* progname;

typedef struct dirl_s
{
  size_t size;
  const char **files;
} dirl_t;

typedef struct tree_s
{
  size_t size;
  struct ent_s
  {
    const char *path;
    struct stat st;
  } *ents;
} tree_t;

typedef struct dexe_s
{
  char ** argv;
  char ** arg1;
  char ** arg2;
} dexe_t;

typedef struct option_s
{
  unsigned int dirs:1;
  unsigned int type:1;
  unsigned int mode:1;
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
  unsigned int exec:1;
  unsigned int exec_always:1;
  unsigned int mode_or;
  unsigned int mode_and;
  dexe_t exec_args;
  dexe_t exec_always_args;
} options_t;

void* xmalloc(size_t);
void* xrealloc(void*, size_t);
dirl_t *getDirList(const char* path);
void freeDirList(dirl_t *d);
tree_t* mergeTrees(tree_t* t1, tree_t* t2);
tree_t *getTree(const char* path);
void pmem(void);
int get_exec_args(char**, int*, dexe_t*);
void printopts(const options_t*);

void* 
xmalloc(size_t s)
{
  void *rv;
  /**/

  rv = malloc(s);
  if (!rv)
    {
      fprintf(stderr, "%s: out of memory in malloc()\n", progname);
      exit(XIT_SYS);
    }
  return rv;
}

void*
xrealloc(void* ptr, size_t nsize)
{
  void* rv;
  /**/

  rv = realloc(ptr, nsize);
  if (!rv)
    {
      fprintf(stderr, "%s: out of memory in realloc()\n", progname);
      exit(XIT_SYS);
    }
  return rv;
}

char* 
xstrdup(const char* s)
{
  char* rs;
  /**/
  rs = xmalloc(strlen(s)+1);
  strcpy(rs, s);
  return rs;
}

dirl_t *
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
  dirl_t *rv = NULL;
  int avail;
  int nread;
  /**/
  fd = open(path, O_RDONLY);
  if (fd<0)
    goto err;

  rv = xmalloc(sizeof(dirl_t));
  rv->size = 0;
  avail = GETDIRLIST_INITIAL_SIZE;
  rv->files = xmalloc(sizeof(const char*)*avail);

  while((nread = getdents(fd, &dentbuf.f_dent, 
			  GETDIRLIST_DENTBUF_SIZE)))
    {
      union dentptr_u dent;
      /**/
      for (dent.f_byte = dentbuf.f_byte;
	   dent.GETDENTS_RETURN_Q-dentbuf.GETDENTS_RETURN_Q<nread;
#if GETDENTS_NEXTDENT
	  dent.GETDENTS_RETURN_Q += dent.f_dent->GETDENTS_NEXTDENT_NAME)
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
      
	  if (rv->size == avail)
	    rv->files = xrealloc(rv->files, (avail*=2)*sizeof(const char*));

	  rv->files[rv->size++] = xstrdup(dent.f_dent->d_name);	  
	}
    }

  if (close(fd)<0)
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
  int avail;
  dirl_t *rv = NULL;
  /**/
  dir = opendir(path);
  if (!dir)
    goto err;

  rv = xmalloc(sizeof(dirl_t));
  rv->size = 0;
  avail = GETDIRLIST_INITIAL_SIZE;
  rv->files = xmalloc(sizeof(const char*)*avail);

  while ((dent = readdir(dir)))
    {
      if (!dent->d_name 
	  || (dent->d_name[0]=='.' 
	      && (dent->d_name[1] == 0
		  || (dent->d_name[1]=='.'
		      && dent->d_name[2]== 0))))
	continue;
      
      if (rv->size == avail)
	rv->files = xrealloc(rv->files, (avail*=2)*sizeof(const char*));

      rv->files[rv->size++] = xstrdup(dent->d_name);
    }

  if (closedir(dir))
    goto err;

  return rv;

 err:
  perror(path);
  return rv;
}
#endif

void
freeDirList(dirl_t *d)
{
  int i;
  /**/
  for (i=0; i<d->size; ++i)
    free((char*)d->files[i]);
  free(d->files);
  free(d);
}

tree_t* mergeTrees(tree_t* t1, tree_t* t2)
{
  tree_t* rt;
  int i1, i2, ir=0;
  /**/
  rt = xmalloc(sizeof(tree_t));
  rt->size  = t1->size+t2->size;
  rt->ents = xmalloc(rt->size * sizeof(struct ent_s));

  for ( i1 = 0, i2 = 0, ir = 0; i1 < t1->size || i2 < t2->size; )
    {
      const struct ent_s *pick = NULL;
      /**/

      if (i1 == t1->size)
	pick = &t2->ents[i2++];
      else if (i2 == t2->size)
	pick = &t1->ents[i1++];
      else
	{
	  int cmp;
	  /**/
	  cmp = strcmp(t1->ents[i1].path, t2->ents[i2].path);
	  if (cmp<0)
	    pick = &t1->ents[i1++];
	  else if (cmp>0)
	    pick = &t2->ents[i2++];
	  else
	    {
	      fprintf(stderr, "%s: duplicate path in mergeTree()\n",
		      progname);
	      exit(XIT_INTERNALERROR);
	    }
	}
      rt->ents[ir++] = *pick;
    }

  return rt;
}

tree_t *getTree(const char* path)
{
  struct stat st;
  tree_t *rt;
  /**/

  if (lstat(path, &st)<0)
    {
      perror(path);
      return NULL;
    }

  rt = xmalloc(sizeof(tree_t));
  rt->size = 1;
  rt->ents = xmalloc(sizeof(struct ent_s));
  rt->ents[0].path = path;
  rt->ents[0].st = st;

  if (S_ISDIR(st.st_mode))
    {
      dirl_t* dirl;
      int i;
      /**/
      if ((dirl=getDirList(path)))
	{
	  for (i=0; i<dirl->size; ++i)
	    {
	      char *npath;
	      tree_t *nt;
	      tree_t *mt;
	      /**/
	      npath = xmalloc(strlen(path)+1+strlen(dirl->files[i])+1);
	      sprintf(npath, "%s/%s", path, dirl->files[i]);
	      if ((nt = getTree(npath)))
		{
		  mt = mergeTrees(rt, nt);
		  free(rt->ents);
		  free(rt);
		  free(nt->ents);
		  free(nt);
		  rt = mt;
		}
	    }
	  freeDirList(dirl);
	}
    }
  
  return rt;
}

void
pmem(void)
{
#if HAVE_MALLINFO
  struct mallinfo minfo;
  minfo = mallinfo();
  fprintf(stderr,
	  "  brk memory = %7d bytes (%d top bytes unreleased)\n"
	  " mmap memory = %7d bytes\n"
	  "total memory = %7d bytes\n",
	  minfo.arena,
	  minfo.keepcost,
	  minfo.hblkhd,
	  minfo.uordblks);
#else
  fprintf(stderr, "memory statistics unavailable\n");
#endif
}

const char*
getFileType(mode_t m)
{
  switch(m & S_IFMT)
    {
    case S_IFDIR:  return "directory";
    case S_IFREG:  return "regular file";
    case S_IFCHR:  return "character device";
    case S_IFBLK:  return "block device";
    case S_IFIFO:  return "fifo";
    case S_IFLNK:  return "symbolic link";
    case S_IFSOCK: return "socket";
    default:       return "unknown";
    }
}

int 
cmpFiles(const char* f1, const char* f2)
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

  result = res_untested;

#if HAVE_MMAP
  if (!(ptr1 = mmap(NULL, fs1, PROT_READ, MAP_SHARED, fd1, 0)))
    goto map1_failed;
  if (!(ptr2 = mmap(NULL, fs2, PROT_READ, MAP_SHARED, fd2, 0)))
    goto map2_failed;

#if HAVE_MADVISE
  if (madvise(ptr1, fs1, MADV_SEQUENTIAL)<0)
    perror(f1);
  if (madvise(ptr2, fs2, MADV_SEQUENTIAL)<0)
    perror(f2);
#endif /* HAVE_MADVISE */

  result = memcmp(ptr1, ptr2, fs1)==0 ? res_same : res_diff;

  if (munmap(ptr2, fs2)<0)
    perror(f2);

 map2_failed:
  if (munmap(ptr1, fs1)<0)
    perror(f1);

 map1_failed:
#endif /* HAVE_MMAP */

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
	      fprintf(stderr, "%s: short read\n", f1);
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
	      fprintf(stderr, "%s: short read\n", f2);
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
  printf("tdiff version 0.1 (CVS $Header$)\n"
	 "Copyright (C) 1999 Philippe Troin\n");
}

void
show_help(void)
{
  printf("usage: %s [options]... <dir1> <dir2>\n"
	 " Standard options:\n"
	 "   -v --version: show %s version\n"
	 "   -h --help\n"
	 " Switch options:\n"
	 "   -d --dirs     diff directories (reports missing files)\n"
	 "   -t --type     diffs for file type differences\n"
	 "                 (if unset, also size, blocks, major, minor and contents\n"
	 "                  are not checked either)\n"
	 "   -m --mode     diffs file modes (permissions)\n"
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
	 "  Each of these options can be negated with an uppercase (short option)\n"
	 "  or with --no-option (eg -M --no-mode for not diffing modes\n"
	 "   -a --all      equivalent to -dtmogsbcj (sets all but times)\n"
	 "   -A --no-all   clears all flags (just reports missing files)\n"
	 " Miscellania:\n"
	 "   -x --exec <cmd>;         executes <cmd> between files if they are similar\n"
	 "                            (if file sizes are equal)\n"
	 "   -w --exec-always <cmd>;  always executes <cmd> between files\n"
	 "        <cmd> uses %%1 and %%2 as file from <dir1> and <dir2>\n"
	 "   -| --mode-or <bits>   applies <bits> OR mode before comparison\n"
	 "   -& --mode-and <bits>  applies <bits> AND mode before comparison\n"
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

int 
main(int argc, char*argv[])
{
  char* ptr;
  tree_t *t1, *t2;
  int i1, i2;
  int rootfd;
  options_t options = { 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, ~0};
  enum { EAO_no, EAO_ok, EAO_error } end_after_options = EAO_no;
  /**/

  pmem();

  for (progname=ptr=argv[0]; *ptr;) if (*ptr=='/') progname=++ptr; else ptr++;

  /* For getopt */
  argv[0] = progname;

  while(1)
    {
#if HAVE_GETOPT_LONG
      struct option long_options[] = {
	{ "version",     0, 0, 'v' },
	{ "help",        0, 0, 'h' },
	{ "type",        0, 0, 't' },
	{ "dirs",        0, 0, 'd' },
	{ "no-dirs",     0, 0, 'D' },
	{ "no-type",     0, 0, 'T' },
	{ "mode",        0, 0, 'm' },
	{ "no-mode",     0, 0, 'M' },
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
	{ "exec",        0, 0, 'x' },
	{ "exec-always", 0, 0, 'w' },
	{ "all",         0, 0, 'a' },
	{ "nothing",     0, 0, 'A' },
	{ "no-all",      0, 0, 'A' },
	{ "mode-or",     1, 0, '|' },
	{ "mode-and",    1, 0, '&' },
	{ 0,             0, 0, 0}
      };
#endif /* HAVE_GETOPT_LONG */
      switch(
#if HAVE_GETOPT_LONG
	     getopt_long
#else
	     getopt
#endif
	     (argc, argv, "vhdDtTmMoOgGzZiIrRsSbBcCjJnNxwaA|:&:"
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
	case 'v':
	  show_version();
	  end_after_options = 1;
	  break;
	case 'h':
	  show_help();
	  end_after_options = 1;
	  break;
	case 'd': options.dirs            = 1; break;
	case 'D': options.dirs            = 0; break;
	case 't': options.type  	  = 1; break;
	case 'T': options.type  	  = 0; break;
	case 'm': options.mode  	  = 1; break;
	case 'M': options.mode  	  = 0; break;
	case 'o': options.owner 	  = 1; break;
	case 'O': options.owner 	  = 0; break;
	case 'g': options.group 	  = 1; break;
	case 'G': options.group 	  = 0; break;
	case 'z': options.ctime 	  = 1; break;
	case 'Z': options.ctime 	  = 0; break;
	case 'i': options.mtime 	  = 1; break;
	case 'I': options.mtime 	  = 0; break;
	case 'r': options.atime 	  = 1; break;
	case 'R': options.atime 	  = 0; break;
	case 's': options.size  	  = 1; break;
	case 'S': options.size  	  = 0; break;
	case 'b': options.blocks   	  = 1; break;
	case 'B': options.blocks   	  = 0; break;
	case 'c': options.contents 	  = 1; break;
	case 'C': options.contents 	  = 0; break;
	case 'j': options.major    	  = 1; break;
	case 'J': options.major 	  = 0; break;
	case 'n': options.minor 	  = 1; break;
	case 'N': options.minor 	  = 0; break;
	case 'a': options.dirs = options.type 
		    = options.mode = options.owner = options.group
		    /* = options.ctime = options.mtime  = options.atime */
		    = options.size  = options.blocks = options.contents
		    = options.major = options.minor = 1; break;
	case 'A': options.dirs = options.type 
		    = options.mode = options.owner = options.group
		    /* = options.ctime = options.mtime  = options.atime */
		    = options.size  = options.blocks = options.contents
		    = options.major = options.minor = 0; break;
	case 'x': 
	  if (get_exec_args(argv, &optind, &options.exec_args))
	    options.contents = options.exec = 1;
	  else
	    end_after_options = EAO_error;
	  break;
	case 'w': 
	  if (get_exec_args(argv, &optind, &options.exec_always_args))
	    options.exec_always = 1;
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

  rootfd = open(".", O_RDONLY);
  if (rootfd<0)
    {
      fprintf(stderr, "%s: cannot open current directory: %s\n",
	      progname, strerror(errno));
      exit(XIT_SYS);
    }

  if (chdir(argv[0])<0)
    {
      perror(argv[0]);
      exit(XIT_INVOC);
    }
  t1 = getTree(xstrdup("."));

  if (fchdir(rootfd)<0)
    {
      fprintf(stderr, "%s: cannot come back to current directory: %s\n",
	      progname, strerror(errno));
      exit(XIT_SYS);
    }
  if (chdir(argv[1])<0)
    {
      perror(argv[1]);
      exit(XIT_INVOC);
    }
  t2 = getTree(xstrdup("."));

  if (fchdir(rootfd)<0)
    {
      fprintf(stderr, "%s: cannot come back to current directory: %s\n",
	      progname, strerror(errno));
      exit(XIT_SYS);
    }
  if (close(rootfd)<0)
    {
      fprintf(stderr, "%s: closing current directory: %s\n",
	      progname, strerror(errno));
      exit(XIT_SYS);
    }

  for (i1=0, i2=0; i1<t1->size || i2<t2->size;)
    {
      int cmp;
      /**/
      if (i1==t1->size)
	goto miss2;
      else if (i2==t2->size)
	goto miss1;
      cmp = strcmp(t1->ents[i1].path, t2->ents[i2].path);
      if (cmp==0)
	{
	  struct stat *s1 = &t1->ents[i1].st;
	  struct stat *s2 = &t2->ents[i2].st;
	  int regfile = 1;
	  /**/

	  if ((S_IFMT & (s1->st_mode))!=(S_IFMT &(s2->st_mode)))
	    {
	      printf("%s: %s vs %s\n",
		     t1->ents[i1].path+2, 
		     getFileType(s1->st_mode),
		     getFileType(s2->st_mode));
	      regfile = 0;
	    }
	  else if (!S_ISREG(s1->st_mode))
	    regfile = 0;

	  if ((~S_IFMT & (s1->st_mode))!=(~S_IFMT &(s2->st_mode)))
	    {
	      printf("%s: mode %04o vs %04o\n",
		     t1->ents[i1].path+2, 
		     ~S_IFMT & (s1->st_mode),
		     ~S_IFMT & (s2->st_mode));
	    }

	  if (s1->st_uid != s2->st_uid)
	    {
	      printf("%s: uid %d vs %d\n",
		     t1->ents[i1].path+2,
		     s1->st_uid,
		     s2->st_uid);
	    }

	  if (s1->st_gid != s2->st_gid)
	    {
	      printf("%s: gid %d vs %d\n",
		     t1->ents[i1].path+2,
		     s1->st_gid,
		     s2->st_gid);
	    }

	  if (regfile)
	    {
	      int rundiff=1;
	      /**/

	      if (s1->st_size != s2->st_size)
		{
		  printf("%s: size %ld vs %ld\n",
			 t1->ents[i1].path+2,
			 (long)s1->st_size,
			 (long)s2->st_size);
		  rundiff = 0;
		}

	      if (s1->st_blocks != s2->st_blocks)
		{
		  printf("%s: blocks %ld vs %ld\n",
			 t1->ents[i1].path+2,
			 (long)s1->st_blocks,
			 (long)s2->st_blocks);
		}

	      if (rundiff)
		{
		  char *f1, *f2;
		  f1= xmalloc(strlen(argv[0])+1+strlen(t1->ents[i1].path+2)+1);
		  f2= xmalloc(strlen(argv[1])+1+strlen(t1->ents[i1].path+2)+1);
		  sprintf(f1, "%s/%s", argv[0], t1->ents[i1].path+2);
		  sprintf(f2, "%s/%s", argv[1], t1->ents[i1].path+2);
		  if (!cmpFiles(f1, f2))
		    {
		      printf("%s: different\n", t1->ents[i1].path+2);
		    }
		  free(f1);
		  free(f2);
		}
	    }

	  free((char*)t1->ents[i1].path);
	  free((char*)t2->ents[i2].path);
	  i1++, i2++;
	}
      else if (cmp<0)
	{
	miss1:
	  printf("Only in %s: %s\n", argv[0], t1->ents[i1].path+2);
	  free((char*)t1->ents[i1].path);
	  i1++;
	}
      else 
	{
	miss2:
	  printf("Only in %s: %s\n", argv[1], t2->ents[i2].path+2);
	  free((char*)t2->ents[i2].path);
	  i2++;
	}
    }  

  free(t1->ents);
  free(t2->ents);
  free(t1);
  free(t2);

  pmem();

  exit(XIT_OK);
}
