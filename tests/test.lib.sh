# This is -*- mode: shell-script; sh-shell:sh; -*-

set -eu

# Utilities
sleep_if_coarse_time() {
  if [ "x$HAVE_NANOSECOND_TIME_RESOLUTION" = xyes -a "x${TRAVIS_JOB_NUMBER-}" = x ]
  then
    xitcode=0
    ./have_subsecond_times || xitcode=$?
    case $xitcode in
      0)   return;;
      1|2) ;;
      *)   exit $xitcode;;
    esac
  fi

  sleep "$@"
}

sync_if_needed() {
  # On Solaris block counts aren't updated until they're on storage.
  case "$TARGET_OS" in
    solaris*)
      sync
      ;;
  esac
}

# Filters
acl_filter() {
  sed -e 's!\(: \(access\|default\) acl \(user\|group\):\)[^[:space:]]\+\(: .*\)$!\1XX\4!'
}

xid_filter() {
  sed -e 's!\(: '"$1"':\) [^[:space:]]*(\([0-9][0-9]*\)) [^[:space:]]*(\([0-9][0-9]*\))$!\1 \2 \3!'
}

xtime_filter() {
  sed -e 's!\(: '"$1"':\) \[[^]][^]]*\] \[[^]][^]]*\]$!\1 [XXX] [XXX]!'
}

# Main program
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

  "${reset:-:}" "$dir1" "$dir2"

  echo 1>&2 "$progname: tdiff $what..."
  echo 1>&2 "$progname:    ${TDIFF:-./tdiff} -vv $* ${TDIFF_ARG1:-$dir1} ${TDIFF_ARG2:-$dir2}"
  (
    exitcode=0
    ${TDIFF:-./tdiff} -vv "$@" "${TDIFF_ARG1:-$dir1}" "${TDIFF_ARG2:-$dir2}" \
      || exitcode=$?
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
if [ "${tdiff_options2-}" != "" ]
then
  run_test "(short options #2)" $tdiff_options2
fi
if [ "${tdiff_options3-}" != "" ]
then
  run_test "(short options #3)" $tdiff_options3
fi

if [ "x$HAVE_GETOPT_LONG" != xno ]
then
  run_test "(long options)"  $tdiff_long_options
  if [ "${tdiff_long_options2-}" != "" ]
  then
    run_test "(long options #2)"  $tdiff_long_options2
  fi
fi

if [ $exitcode -eq 0 ]
then
  "${cleanup_test:-:}" "$dir1" "$dir2" || :
  rm -fr "$dir1" "$dir2"
fi
exit $exitcode
