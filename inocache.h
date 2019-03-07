/*
  tdiff - tree diffs
  inocache.h - Cache the inode already looked up so that we don't have to
	       run tests twice on them.
  Copyright (C) 1999, 2014 Philippe Troin <phil@fifi.org>

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

#ifndef INOCACHE_H
#define INOCACHE_H

#include "genhash.h"
#include <sys/types.h>

typedef struct ic_ent_s
{
  ino_t ino[2];
  dev_t dev[2];
} ic_ent_t;

struct genhash_s;
typedef struct genhash_s inocache_t;

inocache_t* ic_new(void);
void ic_delete(inocache_t*);
const char* ic_get(const inocache_t*, const ic_ent_t*);
int ic_put(inocache_t*, const ic_ent_t*, const char*);
hashval_t ic_hash(const void* vent);

#endif /* INOCACHE_H */
