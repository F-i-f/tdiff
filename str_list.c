/*
  tdiff - tree diffs
  str_list.c - Lists of strings
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

#include "config.h"
#include "str_list.h"

#include <string.h>
#include "tdiff.h"
#include "utils.h"

void
str_list_new_size(str_list_t **l, size_t sz)
{
  str_list_t* rv;
  /**/
  *l = NULL;
  rv = xmalloc(sizeof(str_list_t));
  rv->size = 0;
  rv->avail = sz;
  if (rv->avail > 0)
    rv->strings = xmalloc(sizeof(const char*)*rv->avail);
  else
    rv->strings = 0;

  *l =rv;
}

static inline void
str_list_grow(str_list_t *l)
{
  if (l->size == l->avail)
    {
      l->avail = l->avail ? l->avail*2 : 1;
      l->strings = xrealloc(l->strings, l->avail*sizeof(const char*));
    }
}

void
str_list_push(str_list_t *l, const char* s)
{
  str_list_grow(l);
  l->strings[l->size++] = xstrdup(s);
}

void
str_list_push_length(str_list_t *l, const char* s, size_t sz)
{
  char *buf;
  /**/

  str_list_grow(l);
  buf = xmalloc(sz);
  memcpy(buf, s, sz);
  l->strings[l->size++] = buf;
}

static
int strpcmp(const char** s1, const char** s2)
{
  return (strcmp(*s1, *s2));
}

static inline void
str_list_sort(const str_list_t *l)
{
  qsort(l->strings, l->size, sizeof(char*),
	(int(*)(const void*, const void*))strpcmp);
}

void
str_list_destroy(str_list_t *d)
{
  size_t i;
  /**/
  for (i=0; i<d->size; ++i)
    free((char*)d->strings[i]);
  free(d->strings);
  free(d);
}

int
str_list_compare(const char *p1, const char* p2,
		 const str_list_t *ct1, const str_list_t *ct2,
		 void (*reportMissing)(int, const char*, const char*, struct str_list_client_data_s*),
		 int (*compareEntries)(const char* p1, const char* p2,
				       const char* e1, const char* e2,
				       struct str_list_client_data_s*),
		 struct str_list_client_data_s *clientData)
{
  size_t	i1, i2;
  int		cmpres;
  int		rv	     = XIT_OK;
  int		localerr     = 0;
  /**/
  str_list_sort(ct1);
  str_list_sort(ct2);

  for (i1 = i2 = 0; i1 < ct1->size || i2 < ct2->size; )
    {
      if (i1 == ct1->size)
	{
	  reportMissing(2, p2, ct2->strings[i2], clientData);
	  i2++;
	}
      else if (i2 == ct2->size)
	{
	  reportMissing(1, p1, ct1->strings[i1], clientData);
	  i1++;
	}
      else if (!(cmpres = strcmp(ct1->strings[i1], ct2->strings[i2])))
	{
	  if (compareEntries != NULL)
	    {
	      int nrv;
	      /**/
	      nrv = compareEntries(p1, p2, ct1->strings[i1], ct2->strings[i2], clientData);
	      if (nrv>rv) rv = nrv;
	    }
	  i1++;
	  i2++;
	}
      else if (cmpres<0)
	{
	  reportMissing(1, p1, ct1->strings[i1], clientData);
	  i1++;
	}
      else /* cmpres>0 */
	{
	  reportMissing(2, p2, ct2->strings[i2], clientData);
	  i2++;
	}

    }

  if (localerr && XIT_DIFF>rv) rv = XIT_DIFF;

  return rv;
}
