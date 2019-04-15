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
ic_hash(const void* vent)
{
  const char *bytes = (const char*)vent;
  hashval_t hv = 5381;
  size_t i;
  /**/

  for (i=0; i < sizeof(ic_ent_t); ++i)
    hv = (( hv << 5 ) + hv) + bytes[i]; /* hv * 33 + bytes[i] */

  return hv;
}

static
int
ic_equal(const void* ve1, const void* ve2)
{
  const ic_ent_t *e1 = (const ic_ent_t*)ve1;
  const ic_ent_t *e2 = (const ic_ent_t*)ve2;

  return e1->ino[0] == e2->ino[0] && e1->ino[1] == e2->ino[1]
    && e1->dev[0] == e2->dev[0] && e2->dev[1] == e2->dev[1];
}

inocache_t*
ic_new(void)
{
  return gh_new(&ic_hash, &ic_equal, &free, &free);
}

void
ic_delete(inocache_t* ic)
{
  gh_delete(ic);
}
const char*
ic_get(const inocache_t* ic, const ic_ent_t *ent)
{
  void* rv;
  if (gh_find(ic, ent, &rv))
    return (const char*)rv;
  else
    return NULL;
}

int ic_put(inocache_t* ic, const ic_ent_t* ent, const char* str)
{
  return gh_insert(ic, (void*)ent, (void*)str);
}
