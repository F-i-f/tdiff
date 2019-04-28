/*
  tdiff - tree diffs
  Prototypes for a generic hash.
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

#ifndef GENHASH_H
#define GENHASH_H

#include "config.h"
#include <sys/types.h>

/* Generic hash types */
typedef struct genhash_s genhash_t;

#if SIZEOF_SHORT==SIZEOF_VOID_P
typedef unsigned short hashval_t;
#elif SIZEOF_INT==SIZEOF_VOID_P
typedef unsigned int hashval_t;
#elif SIZEOF_LONG==SIZEOF_VOID_P
typedef unsigned long hashval_t;
#elif SIZEOF_LONG_LONG==SIZEOF_VOID_P
typedef unsigned long long hashval_t;
#else
#  error Cannot find pointer-sized integral type !
#endif

/* Generic hash interface */
genhash_t* gh_new(hashval_t(*)(const void*),
		  int(*)(const void*, const void*),
		  void (*)(void*),
		  void (*)(void*));
int        gh_insert(genhash_t*, void*, void*);
int        gh_find(const genhash_t*, const void*, void**);
void       gh_delete(genhash_t* gh);
void       gh_stats(const genhash_t *gh, const char* name);

/* Hash and equality functions for strings, identify */
hashval_t gh_string_hash(const void*);
hashval_t gh_binary_hash(const void*, size_t);
hashval_t gh_identity_hash(const void*);
int gh_string_equal(const void*, const void*);
int gh_identity_equal(const void*, const void*);

#endif /* ndef GENHASH_H */
