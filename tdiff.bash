#  tdiff - tree diffs
#  tdiff.bash - Bash completion file
#  Copyright (C) 2019 Philippe Troin <phil+github-commits@fifi.org>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

_tdiff()
{
    local cur prev words cword
    _init_completion || return

    case $prev in
      -v|--verbose|\
      -V|--version|\
      -h|--help|\
      -d|--dirs|\
      -D|--no-dirs|\
      -t|--type|\
      -T|--no-type|\
      -m|--mode|\
      -M|--no-mode|\
      -u|--uid|\
      -U|--no-uid|\
      -g|--gid|\
      -G|--no-gid|\
      -n|--nlink|\
      -N|--no-nlink|\
      -e|--hardlinks|\
      -E|--no-hardlinks|\
      -i|--mtime|\
      -I|--no-mtime|\
      -y|--atime|\
      -Y|--no-atime|\
      -z|--ctime|\
      -Z|--no-ctime|\
      -s|--size|\
      -S|--no-size|\
      -b|--blocks|\
      -B|--no-blocks|\
      -c|--contents|\
      -C|--no-contents|\
      -j|--major|\
      -J|--no-major|\
      -k|--minor|\
      -K|--no-minor|\
      -l|--acl|\
      -L|--no-acl|\
      -f|--flags|\
      -F|--no-flags|\
      -q|--xattr|\
      -Q|--no-xattr|\
      -[0-9]|\
      -W|--exec-always-diff)
	;;
      -p|--preset)
	COMPREPLY=( $( compgen -W '0 none
				   1 missing type
				   2 mode
				   3 owner
				   4 hardlinks
				   5 contents
				   6 notimes default
				   7 mtime
				   8 amtimes
				   9 alltimes all' -- "$cur" ) )
	return
	;;
      -a|--mode-and|\
      -o|--mode-or)
	COMPREPLY=()
	return
	;;
      -w|--exec-always)
      -x|--exec|\
	words=(words[0] "$cur")
	cword=1
	_command
	return
	;;
      -X|--exclude)
	_filedir
	return
	;;
    esac

    if [[ $cur == -* ]]; then
      COMPREPLY=( $( compgen -W '--verbose
				 --version
				 --help
				 --dirs --no-dirs
				 --type --no-type
				 --mode --no-mode
				 --uid --no-uid
				 --gid --no-gid
				 --nlink --no-nlink
				 --hardlinks --no-hardlinks
				 --mtime --no-mtime
				 --atime --no-atime
				 --ctime --no-ctime
				 --size --no-size
				 --blocks --no-blocks
				 --contents --no-contents
				 --major --no-major
				 --minor --no-minor
				 --acl --no-acl
				 --flags --no-flags
				 --xattr --no-xattr
				 --preset
				 --mode-and
				 --mode-or
				 --exec-always
				 --exec-always-diff
				 --exec
				 --exclude
				 ' -- "$cur" ) )
	return
    fi

    _filedir
} &&
complete -F _tdiff tdiff

# Local variables:
# mode: shell-script
# sh-shell: bash
# End:
