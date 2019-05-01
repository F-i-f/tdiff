# This is -*- mode: shell-script; sh-shell:sh; -*-

set -eu

setup() {
  echo "File 1" > "$1"/entry1
  chmod 600 "$1"/entry1
  echo foo > "$1"/entry2
  ln "$1"/entry2 "$1"/entry3
  echo bar > "$1"/entry4
  mkdir "$1"/dir1
  touch "$1"/dir1/missing

  mkdir "$2"/entry1
  chmod 700 "$2"/entry1
  echo foo > "$2"/entry2
  echo foo > "$2"/entry3
  dd bs=64k count=4 if=/dev/zero of="$2"/entry4
  mkdir "$2"/dir1
  touch "$2"/missing

  # On Solaris block counts aren't updated until they're on storage.
  sync
}

preset_filter() {
 #$ Combining both sed statements fails on FreeBSD 12.0p3
  sed -e '/^tdiff: entry1: blocks: /d' \
      -e '/^tdiff: entry1: size: /d' \
      -e 's!^\(tdiff: entry4: blocks:\) [0-9][0-9]* [0-9][0-9]*!\1 XX XX!'
}

toplevel_atime_filter() {
  sed -e '/^tdiff: (top-level): atime: /d'
}
