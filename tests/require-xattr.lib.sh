# This is -*- mode: shell-script; sh-shell:sh; -*-

if [ "x$HAVE_XATTRS" != xyes ]
then
  exit 77
fi

pre_check_file() {
  setfattr -n user.test -v value "$@"
}

. "$srcdir"/tests/pre-check-file.lib.sh
