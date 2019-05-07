/*
  tdiff - tree diffs
  have_subsecond_times.c - Check if the build directory has subsecond times.
  Copyright (C) 2019 Philippe Troin <phil+github-commits@fifi.org>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "st_xtime_ns.h"

#if __GNUC__
# define ATTRIBUTE_UNUSED __attribute__((unused))
#else /* ! __GNUC__ */
# define ATTRIBUTE_UNUSED /**/
#endif /* ! __GNUC__ */



static const char * const paths[] = { ".",
				      "tdiff.h",
				      "tdiff.c",
				      "condif.h",
				      NULL
};


int
main(int argc, char* argv[])
{
#ifdef ST_ATIMENSEC
  const char *progname;
  const char *try_path;
  struct stat sbuf;

  if (argc > 0)
    {
      progname = strrchr(argv[0], '/');
      if (progname)
	++progname;
      else
	progname = argv[0];
    }
  else
    progname = "have_subsecond_times";

  for (try_path = paths[0]; *try_path; ++try_path)
    {
      if (stat(try_path, &sbuf) != 0)
	{
	  fprintf(stderr, "%s: stat(%s): %s\n",
		  progname, try_path, strerror(errno));
	  exit(99);
	}
      if (sbuf.ST_ATIMENSEC.tv_nsec != 0
	  || sbuf.ST_MTIMENSEC.tv_nsec != 0
	  || sbuf.ST_CTIMENSEC.tv_nsec != 0)
	exit(0);
    }
  exit(1);
#else /* ! defined(ST_ATIMENSEC) */
  exit(2);
#endif
}
