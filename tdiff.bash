# -*- shell-script -*-

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
      -n|--nlinks|\
      -N|--no-nlinks|\
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
				 --nlinks --no-nlinks
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

# ex: filetype=sh
