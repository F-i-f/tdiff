/*
  tdiff - tree diffs
  Generic hash test program.
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
#include <stdlib.h>
#include <stdio.h>

#define INHASH_STRINGS 100000
#define DUMMY_STRINGS 100000

char* genString(void)
{
  int nchars = random()%64;
  int i;
  char *str = xmalloc(nchars+1);
  for (i=0; i<nchars; i++)
    str[i] = random()%255+1;
  str[nchars] = 0;
  return str;
}

int 
main(void)
{
  char * inhash[INHASH_STRINGS];
  int    assocs[INHASH_STRINGS];
  char * dummies[DUMMY_STRINGS];
  int i;
  genhash_t* hash;
  void *valueptr;

  srandom(0);

  pmem();

  setprogname("genhash_test");

  hash = gh_new(gh_string_hash, gh_string_equal, free, NULL);

  for (i=0; i<INHASH_STRINGS; i++)
    {
      char* nstring;
      do 
	nstring = genString();
      while (gh_find(hash, nstring, NULL) ? (free(nstring), 1) : 0);

      if (!gh_insert(hash, inhash[i]=nstring, (void*)(assocs[i] = random())))
	printf("duplicate inserted ?\n");
    }
  
  for (i=0; i<DUMMY_STRINGS; i++)
    {
      char* nstring;
      do 
	nstring = genString();
      while (gh_find(hash, nstring, NULL) ? (free(nstring), 1) : 0);

      dummies[i] = nstring;
    }

  for (i=0; i<DUMMY_STRINGS; i++)
    {
      if (gh_find(hash, dummies[i], NULL))
	printf("found dummy !\n");
      free(dummies[i]);
    }

  for (i=0; i<INHASH_STRINGS; i++)
    {
      if (!gh_find(hash, inhash[i], &valueptr))
	printf("couldn't find inhash key !\n");
      if ((int)valueptr != assocs[i])
	printf("stored value changed !\n");
    }

  gh_delete(hash);

  pmem();

  return 0;
}
