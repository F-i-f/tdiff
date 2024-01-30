# This is -*- mode: shell-script; sh-shell:sh; -*-

# tdiff - tree diffs
# tests/require-acl.lib.sh
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

if [ "x$HAVE_ACLS" != xyes ]
then
  exit 77
fi

pre_check_file() {
  setfacl -m g::- "$@"
}

. "$srcdir"/tests/pre-check-file.lib.sh
