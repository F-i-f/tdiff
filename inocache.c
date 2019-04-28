/*
  tdiff - tree diffs
  inocache.c - Cache the inode already looked up so that we don't have to
	       run tests twice on them.
  Copyright (C) 1999, 2006, 2014, 2019 Philippe Troin <phil+github-commits@fifi.org>

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
#include "inocache.h"
#include "genhash.h"
#include <stdlib.h>

/* DJB2 hash */
hashval_t
epc_hash(const void* vent)
{
  const char *bytes = (const char*)vent;
  hashval_t hv = 5381;
  size_t i;
  /**/

  for (i=0; i < sizeof(ent_pair_cache_key_t); ++i)
    hv = (( hv << 5 ) + hv) + bytes[i]; /* hv * 33 + bytes[i] */

  return hv;
}

static
int
ic_equal(const void* ve1, const void* ve2)
{
  const ent_pair_cache_key_t *e1 = (const ent_pair_cache_key_t*)ve1;
  const ent_pair_cache_key_t *e2 = (const ent_pair_cache_key_t*)ve2;

  return e1->ent1.ino == e2->ent1.ino && e1->ent2.ino == e2->ent2.ino
      && e1->ent1.dev == e2->ent1.dev && e1->ent2.dev == e2->ent2.dev;
}

ent_pair_cache_t*
epc_new(void)
{
  return gh_new(&epc_hash, &ic_equal, &free, &free);
}

void
epc_destroy(ent_pair_cache_t* ic)
{
  gh_delete(ic);
}
const char*
epc_get(const ent_pair_cache_t* ic, const ent_pair_cache_key_t *ent)
{
  void* rv;
  if (gh_find(ic, ent, &rv))
    return (const char*)rv;
  else
    return NULL;
}

int epc_put(ent_pair_cache_t* ic, const ent_pair_cache_key_t* ent, const char* str)
{
  return gh_insert(ic, (void*)ent, (void*)str);
}
