/*
  tdiff - tree diffs
  hard_links_cache.c - Remember all the hard links
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
#include "hard_links_cache.h"
#include <string.h>

hashval_t
hc_hash(const void* vent)
{
  return gh_binary_hash(vent, sizeof(hard_links_cache_key_t));
}

int
hc_equal(const void* ve1, const void* ve2)
{
  const hard_links_cache_key_t *e1 = (const hard_links_cache_key_t*)ve1;
  const hard_links_cache_key_t *e2 = (const hard_links_cache_key_t*)ve2;

  return e1->ino == e2->ino
      && e1->dev == e2->dev;
}

void
hc_add_hard_link(hard_links_cache_t* hc, const hard_links_cache_key_t* k, hard_links_cache_val_t* v, const char *name)
{
  if (v == NULL)
    {
      hard_links_cache_key_t *kc = xmalloc(sizeof(*k));
      memcpy(kc, k, sizeof(*k));
      str_list_new_size(&v, 1);
      hc_put(hc, kc, v);
    }

  str_list_push(v, name);
}
