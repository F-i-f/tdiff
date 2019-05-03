/*
  tdiff - tree diffs
  General definitions.
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

#ifndef TDIFF_H
#define TDIFF_H

#define XIT_OK            0
#define XIT_INVOC         1
#define XIT_DIFF          2
#define XIT_SYS           3
#define XIT_INTERNALERROR 4

extern const char* progname;

#define BUMP_EXIT_CODE(current_code, new_code)	\
  do {						\
    if (new_code > current_code)		\
      current_code = new_code;			\
  } while (0)

#endif /* ndef TDIFF_H */
