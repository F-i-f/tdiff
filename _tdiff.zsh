#compdef tdiff

#  tdiff - tree diffs
#  _tdiff.zsh - Bash completion file
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

local -a args

args=('*'{-v,--verbose}'[increase verbosity level]'
      {-V,--version}'[show version information]'
      {-h,--help}'[show terse help message]'

      '*'{-X,--exclude}'[exclude file]:file:_files'

      '1:first directory:_files'
      '2:second directory:_files'

      + '(dirs)'
      {-d,--dirs}'[diff directories (reports missing files)]'
      {-D,--no-dirs}'[do not diff directories (do not report missing files)]'

      + '(type)'
      {-t,--type}'[diff file types]'
      {-T,--no-type}'[do not diff file types (implies -BSJKC)]'

      + '(mode)'
      {-m,--mode}'[diff file modes (permissions)]'
      {-M,--no-mode}'[do not diff file modes (permissions)]'

      + '(uid)'
      {-u,--uid}'[diff file owner user id]'
      {-U,--no-uid}'[do not diff owner user id]'

      + '(gid)'
      {-g,--gid}'[diff file group id]'
      {-G,--no-gid}'[do not diff group id]'

      + '(nlink)'
      {-n,--nlink}'[diff file (hard) link count]'
      {-N,--no-nlink}'[do not diff (hard) link count]'

      + '(hardlinks)'
      {-e,--hardlinks}'[diff file hard link targets]'
      {-E,--no-hardlinks}'[do not diff hard link targets]'

      + '(mtime)'
      {-i,--mtime}'[diff file mtime (contents modification time)]'
      {-I,--no-mtime}'[do not diff mtime (contents modification time)]'

      + '(atime)'
      {-y,--atime}'[diff file atime (access time)]'
      {-Y,--no-atime}'[do not diff atime (access time)]'

      + '(ctime)'
      {-z,--ctime}'[diff file ctime (inode modification time)]'
      {-Z,--no-ctime}'[do not diff ctime (inode modification time)]'

      + '(size)'
      {-s,--size}'[diff file size (for regular files and symbolic links)]'
      {-S,--no-size}'[do not diff size (for regular files and symbolic links)]'

      + '(blocks)'
      {-b,--blocks}'[diff file blocks (for regular files, directories and symbolic links)]'
      {-B,--no-blocks}'[do not diff blocks (for regular files, directories and symbolic links)]'

      + '(contents)'
      '(exec)'{-c,--contents}'[diff file contents (for regular files and symbolic links)]'
      '(exec)'{-C,--no-contents}'[do not diff contents (for regular files and symbolic links)]'

      + '(major)'
      {-j,--major}'[diff file major device numbers (for device files)]'
      {-J,--no-major}'[do not diff major device numbers (for device files)]'

      + '(minor)'
      {-k,--minor}'[diff file minor device numbers (for device files)]'
      {-K,--no-minor}'[do not diff minor device numbers (for device files)]'

      + '(preset)'
      {-p,--preset}'=[preset]:preset name:((0\:"clear all toggles, no comparisons"
					 none\:"level 0\: nothing"
					    1\:"only missing files and differing types"
				      missing\:"level 1\: only missing files and differing types"
					 type\:"level 1\: only missing files and differing types"
					    2\:"add modes"
					 mode\:"level 2\: missing, type, mode"
					    3\:"add uid, gid, ACLs"
					owner\:"level 3\: missing, type, mode, uid, gid, ACLs"
					    4\:"add link counts and hard linked files"
				    hardlinks\:"level 4\: missing, type, mode, ids, ACLs, hard links"
					    5\:"add size, block count, contents, major & minor"
				     contents\:"level 5\: add size, block count, contents, major & minor"
					    6\:"add flags (BSD), xattrs"
				      notimes\:"level 6\: everything but times"
				      default\:"level 6\: everything but times"
					    7\:"add mtime"
					mtime\:"level 7\: everything but atime/ctime"
					    8\:"add atime"
				      amtimes\:"level 8\: everything but ctime"
					    9\:"add ctime, all comparisons"
				     alltimes\:"level 9\: everything"
					  all\:"level 9\: everything"
					    ))'
      '-0[preset: nothing]'
      '-1[preset: only missing files and differing types]'
      '-2[preset: add modes]'
      '-3[preset: add uid, gid, ACLs]'
      '-4[preset: add link counts and hard linked files]'
      '-5[preset: add size, block vount, contents, major & minor]'
      '-6[preset: add flags (BSD), xattrs.  All but times.]'
      '-7[preset: add mtime.  All but atime/ctime.]'
      '-8[preset: add atime.  All but ctime.]'
      '-9[preset: add ctime.  Everything.]'

      + '(mode-and)'
      {-a,--mode-and}'[applies an AND mask to file modes]:mask (octal)'

      + '(mode-or)'
      {-o,--mode-or}'[applies an OR mask to file modes]:mask (octal)'

      + '(exec-always)'
      {-w,--exec-always}'[execute a command on file pairs]:program: _command_names -e:*\;::program arguments: _normal'
      {-W,--exec-always-diff}'[executes "diff -u" for every file pair]'

      + '(exec)'
      {-x,--exec}'[execute a command to check if files are similar]:program: _command_names -e:*\;::program arguments: _normal'

     )

local tdiff_out="$(tdiff --help)"
local has_flags has_acl has_xattr

if [[ -n ${(M)tdiff_out:+--flags} ]]
then
  has_flags=yes
  args+=( + '(flags)'
	  {-f,--flags}'[diff file flags (BSD flags: nodump, uimmutable, etc)]'
	  {-F,--no-flags}'[do not diff flags (BSD flags: nodump, uimmutable, etc)]'
	)
fi

if [[ -n ${(M)tdiff_out:+--acl} ]]
then
  has_acl=yes
  args+=( + '(acl)'
	  {-l,--acl}'[diff file ACLs (access control lists)]'
	  {-L,--no-acl}'[do not diff ACLs (access control lists)]'
	)
fi

if [[ -n ${(M)tdiff_out:+--xattr} ]]
then
  has_xattr=yes
  args+=( + '(xattr)'
	  {-q,--xattr}'[diff file extended attributes]'
	  {-Q,--no-xattr}'[do not diff extended attributes]'
	)
fi

_arguments -s -S $args[*]

# Local variables:
# mode: shell-script
# sh-shell: zsh
# End:
