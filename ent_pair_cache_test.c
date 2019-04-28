/*
  tdiff - tree diffs
  ent_pair_cache_test.c - Simple tests for the entry pair cache.
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
#include "ent_pair_cache.h"
#include "utils.h"
#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char* argv[])
{
  int			i;
  struct stat		sbuf1;
  struct stat		sbuf2;
  ent_pair_cache_key_t	ice;
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
      ice.ent1.ino = sbuf1.st_ino;
      ice.ent1.dev = sbuf1.st_dev;
      ice.ent2.ino = sbuf2.st_ino;
      ice.ent2.dev = sbuf2.st_dev;
      printf("%s: dev=%0*lX ino=%0*lX\n"
	     "%s: dev=%0*lX ino=%0*lX\n"
	     "    hash=%0*lX\n",
	     argv[i],
	     SIZEOF_DEV_T*2, (long)ice.ent1.dev,
	     SIZEOF_INO_T*2, (long)ice.ent1.ino,
	     argv[i+1],
	     SIZEOF_DEV_T*2, (long)ice.ent2.dev,
	     SIZEOF_INO_T*2, (long)ice.ent2.ino,
	     SIZEOF_VOID_P*2, (long)epc_hash(&ice));
    }
  return 0;
}
