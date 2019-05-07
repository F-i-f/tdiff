/*
  tdiff - tree diffs
  st_xtime_ns.h - Definition for nanosecond stat times.
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

#ifndef ST_XTIME_NS_H
#define ST_XTIME_NS_H

#include "config.h"

#if HAVE_ST_ATIMESPEC
# define ST_ATIMENSEC st_atimespec
# define ST_CTIMENSEC st_ctimespec
# define ST_MTIMENSEC st_mtimespec
#elif HAVE_ST_TIMENSEC
# define ST_ATIMENSEC st_atim
# define ST_CTIMENSEC st_ctim
# define ST_MTIMENSEC st_mtim
#endif

#endif /* ndef ST_XTIME_NS_H */
