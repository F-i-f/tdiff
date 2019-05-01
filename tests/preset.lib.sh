# This is -*- mode: shell-script; sh-shell:sh; -*-

set -eu

setup() {
  echo "File 1" > "$1"/entry1
  chmod 700 "$1"/entry1
  echo foo > "$1"/entry2
  ln "$1"/entry2 "$1"/entry3
  echo bar > "$1"/entry4
  mkdir "$1"/dir1
  touch "$1"/dir1/missing

  mkdir "$2"/entry1
  chmod 600 "$2"/entry1
  echo foo > "$2"/entry2
  echo foo > "$2"/entry3
  dd bs=64k count=4 if=/dev/zero of="$2"/entry4
  mkdir "$2"/dir1
  touch "$2"/missing

}
