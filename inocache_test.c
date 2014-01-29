/*
  tdiff - tree diffs
  inocache_test.c - Simple tests for inode cache hash function.
  Copyright (C) 1999, 2014 Philippe Troin <phil@fifi.org>

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
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "inocache.h"
#include "utils.h"
#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char* argv[])
{
  int i;
  struct stat sbuf1;
  struct stat sbuf2;
  ic_ent_t ice;
  /**/

  setprogname(argv[0]);

  if (argc%2 != 1)
    {
      fprintf(stderr, "need pairs of arguments\n");
      return 1;
    }

  for (i=1; i<argc; i+=2)
    {
      if (stat(argv[i], &sbuf1)<0)
	{
	  perror(argv[i]);
	  continue;
	}
      if (stat(argv[i+1], &sbuf2)<0)
	{
	  perror(argv[i+1]);
	  continue;
	}
      ice.ino[0] = sbuf1.st_ino;
      ice.dev[0] = sbuf1.st_dev;
      ice.ino[1] = sbuf2.st_ino;
      ice.dev[1] = sbuf2.st_dev;
      printf("%s: dev=%0*lX ino=%0*lX\n"
	     "%s: dev=%0*lX ino=%0*lX\n"
	     "    hash=%0*lX\n",
	     argv[i],
	     SIZEOF_DEV_T*2, (long)ice.dev[0],
	     SIZEOF_INO_T*2, (long)ice.ino[0],
	     argv[i+1],
	     SIZEOF_DEV_T*2, (long)ice.dev[1],
	     SIZEOF_INO_T*2, (long)ice.ino[1],
	     SIZEOF_VOID_P*2, (long)ic_hash(&ice));
    }
  return 0;
}
