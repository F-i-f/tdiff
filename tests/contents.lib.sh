# This is -*- mode: shell-script; sh-shell:sh; -*-

setup() {
  echo "1234" > "$1"/file1
  echo "123" > "$1"/file2
  ln -s /dev/null "$1/file3"
  ln -s /dev/null "$1/file4"
  echo "identical" > "$1"/file5
  echo "123" > "$1"/file6
  ln "$1"/file6 "$1"/file7
  ln "$1"/file6 "$1"/file8
  echo "meh" > "$1"/file9

  echo "12345" > "$2"/file1
  echo "FILE 2" > "$2"/file2
  ln -s x "$2"/file3
  echo "notlink" > "$2"/file4
  echo "identical" > "$2"/file5
  echo "abc" > "$2"/file6
  ln "$2"/file6 "$2"/file7
  ln "$2"/file6 "$2"/file8

  ln "$1"/file9 "$2"/file9
}
