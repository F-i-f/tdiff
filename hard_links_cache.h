/*
  tdiff - tree diffs
  hard_links_cache.h - Remember all the hard links
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

#ifndef HARD_LINKS_CACHE_H
#define HARD_LINKS_CACHE_H

#include "config.h"
#include "ent_key.h"
#include "genhash.h"
#include "str_list.h"
#include "utils.h"
#include <stdlib.h>

typedef ent_key_t  hard_links_cache_key_t;
typedef str_list_t hard_links_cache_val_t;

struct genhash_s;
typedef struct genhash_s hard_links_cache_t;

hashval_t	hc_hash(const void* vent);
int		hc_equal(const void* ve1, const void* ve2);
void		hc_add_hard_link(hard_links_cache_t* hc,
				 const hard_links_cache_key_t* k,
				 hard_links_cache_val_t* v,
				 const char *name);

static inline hard_links_cache_t*
hc_new(void)
{
  return gh_new(&hc_hash, &hc_equal, &free, (void(*)(void*))&str_list_destroy);
}

static inline void
hc_destroy(hard_links_cache_t* ic)
{
  gh_delete(ic);
}
static inline hard_links_cache_val_t*
hc_get(const hard_links_cache_t* ic, const hard_links_cache_key_t *ent)
{
  void* rv;
  if (gh_find(ic, ent, &rv))
    return (hard_links_cache_val_t*)rv;
  else
    return NULL;
}

static inline int
hc_put(hard_links_cache_t* ic, const hard_links_cache_key_t* ent, hard_links_cache_val_t* hcv)
{
  return gh_insert(ic, (void*)ent, (void*)hcv);
}

#endif /* HARD_LINKS_CACHE_H */
