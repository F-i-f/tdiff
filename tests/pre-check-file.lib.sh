# This is -*- mode: shell-script; sh-shell:sh; -*-

sigs='TERM INT QUIT HUP EXIT'
tmpfile=
test_clean() {
  rm -f "$tmpfile"
  exit 99
}
trap test_clean $sigs
tmpfile="$(mktemp ./$(basename "$0").tmp.XXXXXXXX)"
exitcode=0
pre_check_file "$tmpfile" || exitcode=$?
rm -f "$tmpfile"
trap - $sigs

if [ $exitcode -ne 0 ]
then
  exit 77
fi

unset sigs tmpfile exitcode
