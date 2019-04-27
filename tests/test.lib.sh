# This is -*- mode: shell-script; sh-shell:sh; -*-

set -eu

progname="$(basename "$0" .test)"
dir1="tests/$progname.dir1"
dir2="tests/$progname.dir2"
out="tests/$progname.out"
gold="$srcdir/tests/$progname.out.gold"

tmpfiles=""
clean() {
  exitcode=$?
  rm -f "$tmpfiles"
  trap - EXIT
  exit $exitcode
}

trap clean EXIT TERM QUIT HUP INT

echo 1>&2 "$progname: set-up..."
rm -fr "$dir1" "$dir2"
mkdir "$dir1" "$dir2"
setup "$dir1" "$dir2"

echo 1>&2 "$progname: tdiff..."
(
  exitcode=0
  ./tdiff -vv $tdiff_options "$dir1" "$dir2" || exitcode=$?
  echo "tdiff exit code=$exitcode"
) > "$out" 2>&1
echo 1>&2 "$progname: tdiff output:"
sed -e 's!^!  !' "$out"

if [ -n "${filter-}" ]
then
  echo 1>&2 "$progname: filtering..."
  tmpfiles="$(mktemp "tests/$progname.tmp.XXXXXXXX")"
  cat "$out" > "$tmpfiles"
  "$filter" < "$tmpfiles" > "$out"
  echo 1>&2 "$progname: filter output:"
  sed -e 's!^!  !' "$out"
fi

echo 1>&2 "$progname: diff"
exitcode=0
diff "$gold" "$out" || exitcode=$?
if [ $exitcode -eq 0 ]
then
  rm -f "$out"
  "${cleanup_test:-:}" "$dir1" "$dir2" || :
  rm -fr "$dir1" "$dir2"
fi
exit $exitcode