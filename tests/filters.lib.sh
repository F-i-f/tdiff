# This is -*- mode: shell-script; sh-shell:sh; -*-

nlink_filter() {
  sed -e '/^tdiff: (top-level): nlink: /d'
}

xtime_filter() {
  sed -e 's!\(: '"$1"':\) \[[^]][^]]*\] \[[^]][^]]*\]$!\1 [XXX] [XXX]!'
}
