/*
  tdiff - tree diffs
  Misc utilities.
  Copyright (C) 1999 Philippe Troin <phil@fifi.org>

  $Id$
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"
#include "utils.h"

#include <stdio.h>

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

#if DEBUG
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
#endif /* DEBUG */
