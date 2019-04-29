/*
  tdiff - tree diffs
  str_list.h - Lists of strings
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

#ifndef STR_LIST_H
#define STR_LIST_H

#include "config.h"
#include <sys/types.h>

typedef struct str_list_s
{
  size_t	size;
  size_t	avail;
  const char ** strings;
} str_list_t;

struct str_list_client_data_s;

#define STR_LIST_NEW_INITIAL_SIZE 8

void str_list_new_size(str_list_t **l, size_t);
static inline
void str_list_new(str_list_t **l)
{
  str_list_new_size(l, STR_LIST_NEW_INITIAL_SIZE);
}
void str_list_destroy(str_list_t *d);
void str_list_push(str_list_t *l, const char* s);
void str_list_push_length(str_list_t *l, const char* s, size_t sz);
int  str_list_compare(const char *p1, const char* p2,
		      const str_list_t *ct1, const str_list_t *ct2,
		      void (*reportMissing)(int, const char*, const char*, struct str_list_client_data_s*),
		      int (*compareEntries)(const char* p1, const char* p2,
					    const char* e1, const char* e2,
					    struct str_list_client_data_s*),
		      struct str_list_client_data_s *clientData);

#endif /* STR_LIST_H */
