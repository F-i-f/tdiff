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

#define XIT_OK            0
#define XIT_INVOC         1
#define XIT_DIFF          2
#define XIT_SYS           3
#define XIT_INTERNALERROR 4

#define GETDIRLIST_INITIAL_SIZE 8
#define GETDIRLIST_DENTBUF_SIZE 8192
#define CMPFILE_BUF_SIZE 16384

const char* progname;

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

typedef struct dirl_s
{
  size_t size;
  const char **files;
} dirl_t;

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

typedef struct tree_s
{
  size_t size;
  struct ent_s
  {
    const char *path;
    struct stat st;
  } *ents;
} tree_t;

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

int 
main(int argc, char*argv[])
{
  char* ptr;
  tree_t *t1, *t2;
  int i1, i2;
  int rootfd;
   /**/

  pmem();

  for (progname=ptr=argv[0]; *ptr;) if (*ptr=='/') progname=++ptr; else ptr++;


  if (argc!=3)
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

  if (chdir(argv[1])<0)
    {
      perror(argv[1]);
      exit(XIT_INVOC);
    }
  t1 = getTree(xstrdup("."));

  if (fchdir(rootfd)<0)
    {
      fprintf(stderr, "%s: cannot come back to current directory: %s\n",
	      progname, strerror(errno));
      exit(XIT_SYS);
    }
  if (chdir(argv[2])<0)
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
		  f1= xmalloc(strlen(argv[1])+1+strlen(t1->ents[i1].path+2)+1);
		  f2= xmalloc(strlen(argv[2])+1+strlen(t1->ents[i1].path+2)+1);
		  sprintf(f1, "%s/%s", argv[1], t1->ents[i1].path+2);
		  sprintf(f2, "%s/%s", argv[2], t1->ents[i1].path+2);
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
	  printf("Only in %s: %s\n", argv[1], t1->ents[i1].path+2);
	  free((char*)t1->ents[i1].path);
	  i1++;
	}
      else 
	{
	miss2:
	  printf("Only in %s: %s\n", argv[2], t2->ents[i2].path+2);
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
