/*
  tdiff - tree diffs
  make_sock
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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int
max(int a, int b)
{
  if (a >= b) {
    return a;
  } else {
    return b;
  }
}

int
main(int argc, char* argv[])
{
  int i;
  int exitcode = 0;

  for (i=1; i < argc; ++i) {
    int fd;
    struct sockaddr_un sun;

    if (strlen(argv[i]) >= sizeof(sun.sun_path)) {
      fprintf(stderr, "%s: %s: path too long, maximum = %u\n",
	      argv[0], argv[i], (unsigned)sizeof(sun.sun_path));
      exitcode = max(exitcode, 1);
      continue;
    }

    fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
      fprintf(stderr, "%s: socket(AF_UNIX, SOCK_STREAM): %s\n",
	      argv[0], strerror(errno));
      exitcode = max(exitcode, 2);
      continue;
    }
    memset(&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    strncpy((char*)&sun.sun_path, argv[i], sizeof(sun.sun_path)-1);
    if (bind(fd, (const struct sockaddr*)&sun, sizeof(sun)) != 0) {
      fprintf(stderr, "%s: %s: bind(): %s\n",
	      argv[0], argv[i], strerror(errno));
      close(fd);
      exitcode = max(exitcode, 2);
      continue;
    }

    if (close(fd) != 0) {
      fprintf(stderr, "%s: %s: close(): %s\n",
	      argv[0], argv[i], strerror(errno));
      exitcode = max(exitcode, 2);
    }
  }

  exit(exitcode);
}
