#compdef tdiff
# Local variables:
# mode: shell-script
# End:

local -a args

args=('-v[increase verbosity level]'
      '--verbose[increase verbosity level]'

      '-V[show version information]'
      '--version[show version information]'

      '-h[show terse help message]'
      '--help[show terse help message]'

      '-d[diff directories (reports missing files)]'
      '--dirs[diff directories (reports missing files)]'
      '-D[do not diff directories (do not report missing files)]'
      '--no-dirs[do not diff directories (do not report missing files)]'

      '-t[diff file types]'
      '--type[diff file types]'
      '-T[do not diff file types (implies -BSJKC)]'
      '--no-type[do not diff file types (implies -BSJKC)]'

      '-m[diff file modes (permissions)]'
      '--mode[diff file modes (permissions)]'
      '-M[do not diff file modes (permissions)]'
      '--no-mode[do not diff file modes (permissions)]'

      '-o[diff file owner]'
      '--owner[diff file owner]'
      '-O[do not diff owner]'
      '--no-owner[do not diff owner]'

      '-g[diff file group]'
      '--group[diff file group]'
      '-G[do not diff group]'
      '--no-group[do not diff group]'

      '-z[diff file ctime (inode modification time)]'
      '--ctime[diff file ctime (inode modification time)]'
      '-Z[do not diff ctime (inode modification time)]'
      '--no-ctime[do not diff ctime (inode modification time)]'

      '-i[diff file mtime (contents modification time)]'
      '--mtime[diff file mtime (contents modification time)]'
      '-I[do not diff mtime (contents modification time)]'
      '--no-mtime[do not diff mtime (contents modification time)]'

      '-r[diff file atime (access time)]'
      '--atime[diff file atime (access time)]'
      '-R[do not diff atime (access time)]'
      '--no-atime[do not diff atime (access time)]'

      '-s[diff file size (for regular files and symbolic links)]'
      '--size[diff file size (for regular files and symbolic links)]'
      '-S[do not diff size (for regular files and symbolic links)]'
      '--no-size[do not diff size (for regular files and symbolic links)]'

      '-b[diff file blocks (for regular files, directories and symbolic links)]'
      '--blocks[diff file blocks (for regular files, directories and symbolic links)]'
      '-B[do not diff blocks (for regular files, directories and symbolic links)]'
      '--no-blocks[do not diff blocks (for regular files, directories and symbolic links)]'

      '-c[diff file contents (for regular files and symbolic links)]'
      '--contents[diff file contents (for regular files and symbolic links)]'
      '-C[do not diff contents (for regular files and symbolic links)]'
      '--no-contents[do not diff contents (for regular files and symbolic links)]'

      '-n[diff file (hard) link count]'
      '--nlinks[diff file (hard) link count]'
      '-N[do not diff (hard) link count]'
      '--no-nlinks[do not diff (hard) link count]'

      '-e[diff file hard link targets]'
      '--hardlinks[diff file hard link targets]'
      '-E[do not diff hard link targets]'
      '--no-hardlinks[do not diff hard link targets]'

      '-j[diff file major device numbers (for device files)]'
      '--major[diff file major device numbers (for device files)]'
      '-J[do not diff major device numbers (for device files)]'
      '--no-major[do not diff major device numbers (for device files)]'

      '-k[diff file minor device numbers (for device files)]'
      '--minor[diff file minor device numbers (for device files)]'
      '-K[do not diff minor device numbers (for device files)]'
      '--no-minor[do not diff minor device numbers (for device files)]'

      '-|[applies an OR mask to file modes]:mask (octal)'
      '--mode-or[applies an OR mask to file modes]:mask (octal)'

      '-x[execute a command to check if files are similar]:program: _command_names -e:*\;::program arguments: _normal'
      '--exec[execute a command to check if files are similar]:program: _command_names -e:*\;::program arguments: _normal'

      '-w[execute a command on file pairs]:program: _command_names -e:*\;::program arguments: _normal'
      '--exec-always[execute a command on file pairs]:program: _command_names -e:*\;::program arguments: _normal'

      '-W[executes "diff -uN" for every file pair]'
      '--exec-always-diff[executes "diff -uN" for every file pair]'

      '-&[applies an AND mask to file modes]:mask (octal)'
      '--mode-and[applies an AND mask to file modes]:mask (octal)'

      '-X[exclude file]:file:_files'
      '--exclude[exclude file]:file:_files'
     )

local tdiff_out="$(tdiff --help)"
local has_flags has_acl has_xattr

if [[ -n ${(M)tdiff_out:+--flags} ]]
then
  has_flags=yes
  args+=(
	  '-f[diff file flags (BSD flags: nodump, uimmutable, etc)]'
	  '--flags[diff file flags (BSD flags: nodump, uimmutable, etc)]'
	  '-F[do not diff flags (BSD flags: nodump, uimmutable, etc)]'
	  '--no-flags[do not diff flags (BSD flags: nodump, uimmutable, etc)]'
    )
fi

if [[ -n ${(M)tdiff_out:+--xattrs} ]]
then
  has_xattr=yes
  args+=(
	  '-q[diff file extended attributes]'
	  '--xattr[diff file extended attributes]'
	  '-Q[do not diff extended attributes]'
	  '--no-xattr[do not diff extended attributes]'
    )
fi

if [[ -n ${(M)tdiff_out:+--acl} ]]
then
  has_acl=yes
  args+=(
	  '-l[diff file ACLs (access control lists)]'
	  '--acl[diff file ACLs (access control lists)]'
	  '-L[do not diff ACLs (access control lists)]'
	  '--no-acl[do not diff ACLs (access control lists)]'
    )
fi

local all_opts='dtm'
if [[ -n $has_flags ]]
then
  all_opts+='f'
fi
all_opts+='ogsbcnejk'
if [[ -n $has_xattr ]]
then
  all_opts+='q'
fi
if [[ -n $has_acl ]]
then
  all_opts+='l'
fi


args+=(	"-a[diff all but times, equivalent to -$all_opts]"
	"--all[diff all but times, equivalent to -$all_opts]"
	'-A[do not diff anything]'
	'--no-all[do not diff anything]'
      )

_arguments -s -S $args[*] \
	   '1:[first directory]:_files' \
	   '2:[second directory]:_files'
