# This is -*- mode: shell-script; sh-shell:sh; -*-

make_file() {
  echo "This is a file" > "$1"
  chmod 777 "$1"
}

make_dir() {
  mkdir "$1"
}

make_fifo() {
  mkfifo "$1"
}

make_symlink() {
  ln -s /dev/null "$1"
}

file_types="file dir fifo symlink"

setup() {
  umask="$(umask)"
  umask 0
  for i in $file_types
  do
    for j in $file_types
    do
      "make_$i" "$1/$i-vs-$j"
      "make_$j" "$2/$i-vs-$j"
    done
  done
  umask "$umask"
}
