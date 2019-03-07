/*
  tdiff - tree diffs
  inocache.c - Cache the inode already looked up so that we don't have to
	       run tests twice on them.
  Copyright (C) 1999, 2006, 2014 Philippe Troin <phil+github-commits@fifi.org>

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
#include "config.h"
#include "genhash.h"
#include <stdlib.h>

hashval_t
ic_hash(const void* vent)
{
  const ic_ent_t *ent = (const ic_ent_t*)vent;
  /**/
  hashval_t hv;
#if (SIZEOF_INO_T == 8 || SIZEOF_DEV_T == 8) && SIZEOF_VOID_P == 4
  union
  {
    hashval_t hvs[2];
    ino_t ino;
    dev_t dev;
  } demux;
#endif

#if SIZEOF_VOID_P == 4

  /* Hashing for 4 bytes */
#if SIZEOF_INO_T == 4
  hv = ent->ino[0] ^ ent->ino[1];
#elif SIZEOF_INO_T == 8
  demux.ino = ent->ino[0];
  hv = demux.hvs[0] ^ demux.hvs[1];
  demux.ino = ent->ino[1];
  hv ^= demux.hvs[0] ^ demux.hvs[1];
#else
# error Unknown ino_t size !
#endif

#if SIZEOF_DEV_T == 4
  hv ^= ent->dev[0] ^ ent->dev[1];
#elif SIZEOF_DEV_T == 8
  demux.dev = ent->dev[0];
  hv ^= demux.hvs[0] ^ demux.hvs[1];
  demux.dev = ent->dev[1];
  hv ^= demux.hvs[0] ^ demux.hvs[1];
#else
# error Unknown dev_t size !
#endif

#elif SIZEOF_VOID_P == 8
  /* Hashing for 8 bytes */

#if SIZEOF_INO_T == 4
  hv = ((hashval_t)ent->ino[0])<<32 | ent->ino[1];
#elif SIZEOF_INO_T == 8
  hv = ent->ino[0] ^ ent->ino[1];
#else
# error Unknown ino_t size !
#endif

#if SIZEOF_DEV_T == 4
  hv ^= ((hashval_t)ent->dev[0])<<32 | ent->dev[1];
#elif SIZEOF_DEV_T == 8
  hv ^= ent->dev[0] ^ ent->dev[1];
#else
# error Unknown dev_t size !
#endif

#else
#error Non 4 or 8 bytes pointer !
#endif

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
