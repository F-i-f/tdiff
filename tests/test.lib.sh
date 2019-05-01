# This is -*- mode: shell-script; sh-shell:sh; -*-

set -eu

progname="$(basename "$0" .test)"
dir1="tests/$progname.dir1"
dir2="tests/$progname.dir2"
out="tests/$progname.out"
gold="$srcdir/tests/$progname.out.gold"

tmpfile=""
clean() {
  exitcode=$?
  rm -f "$tmpfile"
  trap - EXIT
  exit $exitcode
}

trap clean EXIT TERM QUIT HUP INT

echo 1>&2 "$progname: set-up..."
"${cleanup_test:-:}" "$dir1" "$dir2" > /dev/null 2>&1 || :
rm -fr "$dir1" "$dir2"
mkdir "$dir1" "$dir2"
setup "$dir1" "$dir2"

run_test()
{
  what="$1"
  shift

  echo 1>&2 "$progname: tdiff $what..."
  (
    exitcode=0
    ${TDIFF:-./tdiff} -vv "$@" "$dir1" "$dir2" || exitcode=$?
    echo "tdiff exit code=$exitcode"
  ) > "$out" 2>&1
  echo 1>&2 "$progname: tdiff output:"
  sed -e 's!^!  !' "$out"

  if [ -n "${filter-}" ]
  then
    echo 1>&2 "$progname: filtering..."
    tmpfile="$(mktemp "tests/$progname.tmp.XXXXXXXX")"
    cat "$out" > "$tmpfile"
    "$filter" < "$tmpfile" > "$out"
    rm -f "$tmpfile"
    echo 1>&2 "$progname: filter output:"
    sed -e 's!^!  !' "$out"
  fi

  echo 1>&2 "$progname: diff $what"
  exitcode=0
  diff "$gold" "$out" || exitcode=$?
  if [ $exitcode -eq 0 ]
  then
    rm -f "$out"
  else
    exit $exitcode
  fi
}

run_test "(short options)" $tdiff_options
if [ "x$HAVE_GETOPT_LONG" != xno ]
then
  run_test "(long options)"  $tdiff_long_options
fi

if [ $exitcode -eq 0 ]
then
  "${cleanup_test:-:}" "$dir1" "$dir2" || :
  rm -fr "$dir1" "$dir2"
fi
exit $exitcode
