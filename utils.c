/*
  tdiff - tree diffs
  Misc utilities.
  Copyright (C) 1999, 2014, 2019 Philippe Troin <phil+github-commits@fifi.org>

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
#include "utils.h"

#include <stdio.h>
#include <string.h>

#if HAVE_MALLINFO
#  include <malloc.h>
#endif

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

void
setprogname(const char *argvname)
{
  const char* ptr;
  for (progname=ptr=argvname; *ptr;)
    if (*ptr=='/')
      progname=++ptr;
    else
      ptr++;
}

void
pmem(void)
{
#if HAVE_MALLINFO
  struct mallinfo minfo;
  minfo = mallinfo();
  fprintf(stderr,
	  "  brk memory = %7ld bytes (%ld top bytes unreleased)\n"
	  " mmap memory = %7ld bytes\n"
	  "total memory = %7ld bytes\n",
	  (long)minfo.arena,
	  (long)minfo.keepcost,
	  (long)minfo.hblkhd,
	  (long)minfo.uordblks);
#else
  fprintf(stderr, "memory statistics unavailable\n");
#endif
}
