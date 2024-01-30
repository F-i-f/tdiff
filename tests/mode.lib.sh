# This is -*- mode: shell-script; sh-shell:sh; -*-

# tdiff - tree diffs
# tests/mode.lib.sh
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

set -eu

setup() {
  echo "File 1" > "$1"/file
  chmod 400 "$1"/file
  mkdir "$1/sub"
  chmod 712 "$1"/sub
  echo "File 1a" > "$1"/sub/file
  chmod 717 "$1"/sub/file
  touch "$1"/sub/only-in-first

  echo "File 1" > "$2"/file
  chmod 401 "$2"/file
  mkdir "$2/sub"
  chmod 722 "$2"/sub
  mkdir "$2"/sub/file
  chmod 617 "$2"/sub/file
  touch "$2"/sub/only-in-second
}

cleanup_test() {
  chmod -R u+rwx "$1" "$2"
}
cleanup_test=cleanup_test
