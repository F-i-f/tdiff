# This is -*- mode: shell-script; sh-shell:sh; -*-

if [ "$(id -u)" -ne 0 ]
then
  if [ -n "${FAKEROOT-}" ]
  then
    if [ -z "${FAKEROOT_RUNNING-}" ]
    then
      exec env FAKEROOT_RUNNING=yes "$FAKEROOT" "$0" ${1+"$@"}
    else
      exit 99
    fi
  else
    exit 77
  fi
fi
