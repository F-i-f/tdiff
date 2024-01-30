# This is -*- mode: shell-script; sh-shell:sh; -*-

# tdiff - tree diffs
# tests/fakeroot.lib.sh
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

if [ "$(id -u)" -ne 0 ]
then
  if [ -n "${FAKEROOT-}" ]
  then
    if [ -z "${FAKEROOT_RUNNING-}" ]
    then
      exec env FAKEROOT_RUNNING=yes "$FAKEROOT" "$0" ${1+"$@"}
    else
      exit 99
    fi
  else
    exit 77
  fi
fi
