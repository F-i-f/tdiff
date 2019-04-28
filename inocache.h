/*
  tdiff - tree diffs
  inocache.h - Cache the inode already looked up so that we don't have to
	       run tests twice on them.
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

#ifndef INOCACHE_H
#define INOCACHE_H

#include "genhash.h"
#include <sys/types.h>

typedef struct ent_key_s
{
  ino_t ino;
  dev_t dev;
} ent_key_t;

typedef struct ent_pair_cache_key_s
{
  ent_key_t ent1;
  ent_key_t ent2;
} ent_pair_cache_key_t;

struct genhash_s;
typedef struct genhash_s ent_pair_cache_t;

ent_pair_cache_t* epc_new(void);
void epc_destroy(ent_pair_cache_t*);
const char* epc_get(const ent_pair_cache_t*, const ent_pair_cache_key_t*);
int epc_put(ent_pair_cache_t*, const ent_pair_cache_key_t*, const char*);
hashval_t epc_hash(const void* vent);

#endif /* INOCACHE_H */
