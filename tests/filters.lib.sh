# This is -*- mode: shell-script; sh-shell:sh; -*-

xtime_filter() {
  sed -e 's!\(: '"$1"':\) \[[^]][^]]*\] \[[^]][^]]*\]$!\1 [XXX] [XXX]!'
}
