/*
  tdiff - tree diffs
  Prototypes for a generic hash.
  Copyright (C) 1999 Philippe Troin <phil@fifi.org>

  $Id$
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef GENHASH_H
#define GENHASH_H

#include "config.h"

/* Generic hash types */
typedef struct genhash_s genhash_t;

#if SIZEOF_INT==4
typedef unsigned int hashval_t;
#elif SIZEOF_SHORT==4
typedef unsigned short hashval_t;
#else
#  error Cannot find 4 bytes integral type !
#endif

/* Generic hash interface */
genhash_t* gh_new(hashval_t(*)(const void*), 
		  int(*)(const void*, const void*),
		  void (*)(void*),
		  void (*)(void*));
int        gh_insert(genhash_t*, void*, void*);
int        gh_find(const genhash_t*, const void*, void**);
int        gh_remove(genhash_t*, const void*);
void       gh_delete(genhash_t* gh);

/* Hash and equality functions for strings, identify */
hashval_t gh_string_hash(const void*);
hashval_t gh_identity_hash(const void*);
int gh_string_equal(const void*, const void*);
int gh_identity_equal(const void*, const void*);

#endif /* ndef GENHASH_H */
