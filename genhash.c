/*
  tdiff - tree diffs
  Generic hash implementation.
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

#include "genhash.h"
#include "utils.h"

#include <string.h>
#include <stdio.h>

#define GH_INITIAL_SIZE 97

static int primelist[] =
{
  101, 211, 401, 809, 1601, 3203, 6421, 12809, 25601, 51203, 102407,
  204803, 409609, 819229, 1638463, 3276803, 6553621, 13107229, 26214401,
  52428841, 104857601, 209715263, 0
};

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
  int primidx;
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
  gh->entries	   = 0;
  gh->primidx      = 0;
  gh->hashfunc	   = hashfunc;
  gh->cmpfunc	   = cmpfunc;
  gh->delkeyfunc   = delkeyfunc;
  gh->delvaluefunc = delvaluefunc;
  gh->buckets      = xmalloc(sizeof(hashbucket_t*)*primelist[0]);
  memset(gh->buckets, 0, sizeof(hashbucket_t*)*primelist[0]);

  return gh;
}

void
gh_delete(genhash_t* gh)
{
  int i;
  hashbucket_t *cbucket;
  hashbucket_t *nbucket;
  /**/

  for (i=0; i < primelist[gh->primidx]; i++)
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

  newsize = primelist[gh->primidx+1];
  if (newsize==0) abort(); /* Prime list exhausted */
  newbuckets = xmalloc(sizeof(hashbucket_t*)*newsize);
  memset(newbuckets, 0, sizeof(hashbucket_t*)*newsize);

  for (i=0; i < primelist[gh->primidx]; i++)
    for (cbucket = gh->buckets[i]; cbucket; )
      {
	nbucket = cbucket->next;
	newval = cbucket->hash % newsize;
	cbucket->next = newbuckets[newval];
	newbuckets[newval] = cbucket;
	cbucket = nbucket;
      }

  free(gh->buckets);
  gh->primidx++;
  gh->buckets = newbuckets;
}

static int
gh_find_withhash(const genhash_t *gh, const void* key, void** value,
		 hashval_t hv)
{
  hashbucket_t *cbucket;
  /**/

  for (cbucket = gh->buckets[hv % primelist[gh->primidx]];
       cbucket;
       cbucket = cbucket->next)
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

  if (gh->entries > primelist[gh->primidx]*2/3)
    gh_grow(gh);

  gh->entries++;

  nbucket = xmalloc(sizeof(hashbucket_t));
  nbucket->hash  = hv;
  nbucket->key   = key;
  nbucket->value = value;
  nbucket->next  = gh->buckets[hv % primelist[gh->primidx]];
  gh->buckets[hv % primelist[gh->primidx]] = nbucket;

  return 1;
}

int
gh_remove(genhash_t *gh, const void* key)
{
  return 0;
}

hashval_t
gh_string_hash_old(const void* vstring)
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

/* DJB2 hash */
hashval_t
gh_string_hash(const void* vstr)
{
  const char* str = (const char*)vstr;
  hashval_t val = 5381;
  hashval_t c;

  while ((c = *str++))
    val = ((val << 5) + val) + c; /* val * 33 + c */

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

void
gh_stats(const genhash_t *gh, const char* name)
{
  int i;
  hashbucket_t *cbucket;
  int obucks=0;
  int maxbucklen=0;
  /**/

  for (i=0; i<primelist[gh->primidx]; i++)
    {
      int cbuck = 0;
      for (cbucket = gh->buckets[i]; cbucket; cbucket = cbucket->next)
	cbuck++;

      if (cbuck)
	{
	  obucks++;
	  if (cbuck>maxbucklen)
	    maxbucklen = cbuck;
	}
    }

  fprintf(stderr,
	  " Hashing statistics for %s (@%p):\n"
	  "    Table size              : %8d\n"
	  "    Entry count             : %8d\n"
	  "    Occupied buckets        : %8d\n"
	  "    Distribution efficiency : %8.04g%%\n"
	  "    Average bucket length   : %8.04g\n"
	  "    Max bucket length       : %8d\n",
	  name, gh,
	  primelist[gh->primidx], gh->entries,
	  obucks, ((double)obucks)/gh->entries*100,
	  ((double)gh->entries)/obucks,
	  maxbucklen);
}
