/*
  tdiff - tree diffs
  ent_key.h - A file entry (device + inode)
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

#ifndef ENT_KEY_H
#define ENT_KEY_H

#include <sys/types.h>

typedef struct ent_key_s
{
  ino_t ino;
  dev_t dev;
} ent_key_t;

#endif /* ENT_KEY_T */
