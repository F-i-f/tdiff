# This is -*- mode: shell-script; sh-shell:sh; -*-

# tdiff - tree diffs
# tests/type.lib.sh
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

make_file() {
  echo "This is a file" > "$1"
  chmod 777 "$1"
}

make_dir() {
  mkdir "$1"
}

make_fifo() {
  mkfifo "$1"
}

make_symlink() {
  ln -s /dev/null "$1"
}

file_types="file dir fifo symlink"

setup() {
  umask="$(umask)"
  umask 0
  for i in $file_types
  do
    for j in $file_types
    do
      "make_$i" "$1/$i-vs-$j"
      "make_$j" "$2/$i-vs-$j"
    done
  done
  umask "$umask"
}
