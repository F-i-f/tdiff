/*
  tdiff - tree diffs
  Generic hash implementation.
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

#include "genhash.h"
#include "utils.h"

#include <string.h>

#define GH_INITIAL_SIZE 97

typedef struct hashbucket_s
{
  hashval_t hash;
  void *key;
  void *value;
  struct hashbucket_s *next;
} hashbucket_t;

struct genhash_s
{
  int entries;
  int size;
  hashval_t (*hashfunc)(const void*);
  int (*cmpfunc)(const void*, const void*);
  void (*delkeyfunc)(void*);
  void (*delvaluefunc)(void*);
  hashbucket_t** buckets;
};

genhash_t*
gh_new(hashval_t (*hashfunc)(const void*), 
       int (*cmpfunc)(const void*, const void*),
       void (*delkeyfunc)(void*),
       void (*delvaluefunc)(void*))
{
  genhash_t* gh;
  /**/
  
  gh = xmalloc(sizeof(genhash_t));
  gh->entries  	   = 0;
  gh->size     	   = GH_INITIAL_SIZE;
  gh->hashfunc 	   = hashfunc;
  gh->cmpfunc  	   = cmpfunc;
  gh->delkeyfunc   = delkeyfunc;
  gh->delvaluefunc = delvaluefunc;
  gh->buckets      = xmalloc(sizeof(hashbucket_t*)*gh->size);
  memset(gh->buckets, 0, sizeof(hashbucket_t*)*gh->size);

  return gh;
}

void 
gh_delete(genhash_t* gh)
{
  int i;
  hashbucket_t *cbucket;
  hashbucket_t *nbucket;
  /**/

  for (i=0; i < gh->size; i++)
    for (cbucket = gh->buckets[i]; cbucket; )
      {
	if (gh->delkeyfunc)
	  gh->delkeyfunc(cbucket->key);
	if (gh->delvaluefunc)
	  gh->delvaluefunc(cbucket->value);
	nbucket = cbucket->next;
	free(cbucket);
	cbucket = nbucket;
      }

  free(gh->buckets);
  free(gh);
}

static void 
gh_grow(genhash_t* gh)
{
  int newsize;
  hashbucket_t **newbuckets;
  int i;
  hashbucket_t *cbucket;
  hashbucket_t *nbucket;
  hashval_t newval;
  /**/

  newsize = gh->size*2-1;
  newbuckets = xmalloc(sizeof(hashbucket_t*)*newsize);
  memset(newbuckets, 0, sizeof(hashbucket_t*)*newsize);

  for (i=0; i < gh->size; i++)
    for (cbucket = gh->buckets[i]; cbucket; )
      {
	nbucket = cbucket->next;
	newval = cbucket->hash % newsize;
	cbucket->next = newbuckets[newval];
	newbuckets[newval] = cbucket;
	cbucket = nbucket;
      }
  
  free(gh->buckets);
  gh->size = newsize;
  gh->buckets = newbuckets;
}

static int
gh_find_withhash(const genhash_t *gh, const void* key, void** value, 
		 hashval_t hv)
{
  hashbucket_t *cbucket;
  /**/

  for (cbucket = gh->buckets[hv % gh->size]; cbucket; cbucket = cbucket->next)
    if (cbucket->hash == hv && gh->cmpfunc(key, cbucket->key))
      break;

  if (cbucket)
    {
      if (value)
	*value = cbucket->value;
      return 1;
    }
  else
    return 0;
}

int 
gh_find(const genhash_t *gh, const void* key, void** value)
{
  return gh_find_withhash(gh, key, value, gh->hashfunc(key));
}

int
gh_insert(genhash_t *gh, void* key, void* value)
{
  hashval_t hv;
  hashbucket_t *nbucket;
  /**/

  hv = gh->hashfunc(key);
  if (gh_find_withhash(gh, key, NULL, hv))
    return 0;
  
  if (gh->entries > gh->size)
    gh_grow(gh);

  gh->entries++;

  nbucket = xmalloc(sizeof(hashbucket_t));
  nbucket->hash  = hv;
  nbucket->key   = key;
  nbucket->value = value;
  nbucket->next  = gh->buckets[hv % gh->size];
  gh->buckets[hv % gh->size] = nbucket;

  return 1;
}

int
gh_remove(genhash_t *gh, const void* key)
{
  return 0;
}

hashval_t
gh_string_hash(const void* vstring) 
{
  // Each letter will be shifted from 0 to 23 bits according to the
  // sequence: 17,10,3,20,13,6,23,16,9,2,19,12,5,22,15,8,1,18,11,4,
  // 21,14,7,0
  const char* string = vstring;
  hashval_t val = 0;
  int i = 17;
  int c;

  while ((c = (*string++)) != 0) {
    val = val ^ (c<<i);
    if ((i-=7)<0) i += 24;
  }
  return val;
}

hashval_t
gh_identity_hash(const void* v)
{
  return (hashval_t)v;
}

int
gh_string_equal(const void* s1, const void* s2)
{
  return strcmp((const char*)s1, (const char*)s2)==0;
}

int
gh_identify_equal(const void* s1, const void* s2)
{
  return s1 == s2;
}
