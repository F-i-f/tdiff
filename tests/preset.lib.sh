# This is -*- mode: shell-script; sh-shell:sh; -*-

set -eu

with_acl=0
with_flags=0
with_root=0
with_xattr=0

case "$(basename "$0" .test)" in
  *-acl-*)
    . "$srcdir"/tests/require-acl.lib.sh
    with_acl=1
    ;;
  *-flags-*)
    if [ "x$HAVE_ST_FLAGS" != xyes ]
    then
      exit 77
    fi
    pre_check_file() {
      chflags nodump "$@"
    }
    . "$srcdir"/tests/pre-check-file.lib.sh
    with_flags=1
    ;;
  *-root-*)
    . "$srcdir"/tests/fakeroot.lib.sh
    with_root=1
    ;;
  *-xattr-*)
    . "$srcdir"/tests/require-xattr.lib.sh
    with_xattr=1
    ;;
esac

preset_reset() {
  touch -t 198001010000 "$1/dir1" "$1"/entry1 "$1"/entry2 "$1"/entry4
}
reset=preset_reset

setup() {
  echo "File 1" > "$1"/entry1
  chmod 600 "$1"/entry1
  echo foo > "$1"/entry2
  ln "$1"/entry2 "$1"/entry3
  echo bar > "$1"/entry4
  if [ $with_acl -ne 0 ]
  then
    setfacl -m u:3:r "$1"/entry4
  fi
  if [ $with_flags -ne 0 ]
  then
    chflags nodump "$1"/entry4
  fi
  if [ $with_root -ne 0 ]
  then
    chown 1 "$1"/entry4
    chgrp 1 "$1"/entry4
    mknod "$1"/node1 c 4 5
    mknod "$1"/node2 c 7 8
    mknod "$1"/node3 b 17 18
  fi
  if [ $with_xattr -ne 0 ]
  then
    setfattr -n user.test -v boo "$1"/entry4
  fi
  mkdir "$1"/dir1
  touch "$1"/dir1/missing

  sleep_if_coarse_time 1

  mkdir "$2"/entry1
  chmod 700 "$2"/entry1
  echo foo > "$2"/entry2
  echo foo > "$2"/entry3
  dd bs=64k count=4 if=/dev/zero of="$2"/entry4
  if [ $with_root -ne 0 ]
  then
    chown 2 "$2"/entry4
    chgrp 2 "$2"/entry4
    mknod "$2"/node1 b 2 3
    mknod "$2"/node2 c 7 9
    mknod "$2"/node3 b 16 18
  fi
  mkdir "$2"/dir1
  touch "$2"/missing

  sleep_if_coarse_time 1

  # On Solaris block counts aren't updated until they're on storage.
  sync
}

if [ $with_acl -ne 0 ]
then
  preset_acl_filter() {
    acl_filter
  }
else
  preset_acl_filter() {
    cat
  }
fi

if [ $with_root -ne 0 ]
then
  preset_xid_filter() {
    xid_filter '[ug]id'
  }
else
  preset_xid_filter() {
    cat
  }
fi

preset_hardlinks_filter() {
  # Hard link counts are wierd for directories on HFS.
  sed -e '/^tdiff: (top-level): nlink: /d' \
      -e '/^tdiff: dir1: nlink: /d'
}

preset_size_filter() {
  #$ Combining both sed statements fails on FreeBSD 12.0p3
  sed -e '/^tdiff: entry1: blocks: /d' \
      -e '/^tdiff: entry1: size: /d' \
      -e 's!^\(tdiff: entry4: blocks:\) [0-9][0-9]* [0-9][0-9]*!\1 XX XX!'
}

preset_atime_filter() {
  sed -e '/^tdiff: (top-level): atime: /d' \
      -e '/^tdiff: entry3: atime: /d'
}
