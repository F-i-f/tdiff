# This is -*- mode: shell-script; sh-shell:sh; -*-

if [ "x$HAVE_ACLS" != xyes ]
then
  exit 77
fi

pre_check_file() {
  setfacl -m g::- "$@"
}

. "$srcdir"/tests/pre-check-file.lib.sh
