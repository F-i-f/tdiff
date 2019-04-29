# This is -*- mode: shell-script; sh-shell:sh; -*-

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
