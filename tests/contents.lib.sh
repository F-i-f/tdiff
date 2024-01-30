# This is -*- mode: shell-script; sh-shell:sh; -*-

# tdiff - tree diffs
# tests/contents.lib.sh
# Copyright (C) 2019 Philippe Troin <phil+github-commits@fifi.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

setup() {
  echo "1234" > "$1"/file1
  echo "123" > "$1"/file2
  ln -s /dev/null "$1/file3"
  ln -s /dev/null "$1/file4"
  echo "identical" > "$1"/file5
  echo "123" > "$1"/file6
  ln "$1"/file6 "$1"/file7
  ln "$1"/file6 "$1"/file8
  echo "meh" > "$1"/file9

  echo "12345" > "$2"/file1
  echo "FILE 2" > "$2"/file2
  ln -s x "$2"/file3
  echo "notlink" > "$2"/file4
  echo "identical" > "$2"/file5
  echo "abc" > "$2"/file6
  ln "$2"/file6 "$2"/file7
  ln "$2"/file6 "$2"/file8

  ln "$1"/file9 "$2"/file9
}
