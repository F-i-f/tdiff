/*
  tdiff - tree diffs
  make_door
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

#if 1
#include <door.h>
#else
typedef struct door_desc door_desc_t;
void door_return(void*, int, void*, int);
int door_create(void*, int, int);
int door_revoke(int);
int fattach(int, const char*);
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#if __GNUC__
# define ATTRIBUTE_UNUSED __attribute__((unused))
#else /* ! __GNUC__ */
# define ATTRIBUTE_UNUSED /**/
#endif /* ! __GNUC__ */

static int
max(int a, int b)
{
  if (a >= b) {
    return a;
  } else {
    return b;
  }
}

static void
door_proc(ATTRIBUTE_UNUSED void* cookie,
	  ATTRIBUTE_UNUSED char* argp,
	  ATTRIBUTE_UNUSED size_t arg_size,
	  ATTRIBUTE_UNUSED door_desc_t *dp,
	  ATTRIBUTE_UNUSED unsigned flags)
{
  door_return(NULL, 0, NULL, 0);
}

int
main(int argc, char* argv[])
{
  int i;
  int exitcode = 0;

  for (i=1; i < argc; ++i) {
    int fd_door, fd_file;

    fd_door = door_create(&door_proc, 0, 0);
    if (fd_door < 0) {
      fprintf(stderr, "%s: door_create(): %s\n",
	      argv[0], strerror(errno));
      exitcode = max(exitcode, 2);
      continue;
    }

    fd_file = open(argv[i], O_CREAT|O_EXCL|O_WRONLY, 0666);
    if (fd_file < 0) {
      fprintf(stderr, "%s: open(%s): %s\n",
	      argv[0], argv[i], strerror(errno));
      exitcode = max(exitcode, 1);
      continue;
    }
    if (close(fd_file) != 0) {
      fprintf(stderr, "%s: close(%s): %s\n",
	      argv[0], argv[i], strerror(errno));
      exitcode = max(exitcode, 2);
      continue;
    }

    if (fattach(fd_door, argv[i]) != 0) {
      fprintf(stderr, "%s: fattach(%s): %s\n",
	      argv[0], argv[i], strerror(errno));
      exitcode = max(exitcode, 2);
      continue;
    }

    if (door_revoke(fd_door) != 0) {
      fprintf(stderr, "%s: door_revoke(%s): %s\n",
	      argv[0], argv[i], strerror(errno));
      exitcode = max(exitcode, 2);
      continue;
    }
  }

  exit(exitcode);
}
