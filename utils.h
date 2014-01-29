/*
  tdiff - tree diffs
  Prototypes for misc utilities.
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

#ifndef UTILS_H
#define UTILS_H

#include "config.h"
#include "tdiff.h"
#include <stdlib.h> /* for size_t */

void* xmalloc(size_t);
void* xrealloc(void*, size_t);
char* xstrdup(const char*);
void setprogname(const char*);

#if DEBUG
void pmem(void);
#else /* ! DEBUG */
#  define pmem()
#endif /* DEBUG */

#endif /* ndef UTILS_H */
