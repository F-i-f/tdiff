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
      -o|--owner|\
      -O|--no-owner|\
      -g|--group|\
      -G|--no-group|\
      -z|--ctime|\
      -Z|--no-ctime|\
      -i|--mtime|\
      -I|--no-mtime|\
      -r|--atime|\
      -R|--no-atime|\
      -s|--size|\
      -S|--no-size|\
      -b|--blocks|\
      -B|--no-blocks|\
      -c|--contents|\
      -C|--no-contents|\
      -n|--nlink|\
      -N|--no-nlink|\
      -e|--hardlinks|\
      -E|--no-hardlinks|\
      -j|--major|\
      -J|--no-major|\
      -k|--minor|\
      -K|--no-minor|\
      -W|--exec-always-diff|\
      -f|--flags|\
      -F|--no-flags|\
      -q|--xattr|\
      -Q|--no-xattr|\
      -l|--acl|\
      -L|--no-acl)
	;;
      -\&|--mode-and|\
      -\||--mode-or)
	COMPREPLY=()
	return
	;;
      -x|--exec|\
      -w|--exec-always)
	words=(words[0] "$cur")
	cword=1
	_command
	return
	;;
      -X|--exclude)
	_filedir
	return;
    esac

    if [[ $cur == -* ]]; then
      COMPREPLY=( $( compgen -W '--verbose
				 --version
				 --help
				 --dirs --no-dirs
				 --type --no-type
				 --mode --no-mode
				 --owner --no-owner
				 --group --no-group
				 --ctime --no-ctime
				 --mtime --no-mtime
				 --atime --no-atime
				 --size --no-size
				 --blocks --no-blocks
				 --contents --no-contents
				 --nlink --no-nlink
				 --hardlinks --no-hardlinks
				 --major --no-major
				 --minor --no-minor
				 --exec-always-diff
				 --flags --no-flags
				 --xattr --no-xattr
				 --acl --no-acl
				 --mode-and
				 --mode-or
				 --exec
				 --exec-always
				 --exclude' -- "$cur" ) )
	return
    fi

    _filedir
} &&
complete -F _tdiff tdiff

# Local variables:
# mode: shell-script
# sh-shell: bash
# End:
