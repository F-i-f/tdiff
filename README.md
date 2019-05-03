tdiff
=====
Compare files inode attributes
==============================

## Features

Compare two file trees, showing any differences in their:

* file size,
* file block count (physical storage size),
* owner user and group ids (uid & gid),
* access, modification and inode change times,
* hard link count, and sets of hard linked files,
* extended attributes (if supported),
* ACLs (if supported),
* file system flags (BSD UFS, MacOSX).

## Documentation

**tdiff** comes with an [extensive manual
page](https://htmlpreview.github.io/?https://raw.githubusercontent.com/F-i-f/tdiff/master/tdiff.1.html).

[View](https://htmlpreview.github.io/?https://raw.githubusercontent.com/F-i-f/tdiff/master/tdiff.1.html) or
download the manual page as:
[[HTML]](https://raw.githubusercontent.com/F-i-f/tdiff/master/tdiff.1.html),
[[PDF]](https://raw.githubusercontent.com/F-i-f/tdiff/master/tdiff.1.pdf) or
[[NROFF]](https://raw.githubusercontent.com/F-i-f/tdiff/master/tdiff.1).

## Examples

Check that the two file trees rooted at _directory1_ and _directory2_
are exactly the same, including symbolic link targets if any,
permissions, hard disk block usage, owner user or group ids, and if
supported, flags, ACLs and extended attributes:

```shell
tdiff directory1 directory2
```

&nbsp;

Same as previous example, but also check that the file modification
times are the same:

```shell
tdiff -i directory1 directory2
```

&nbsp;

Only report if any files are present in only one directory:

```shell
tdiff -0 --dirs directory1 directory2
```

&nbsp;

Report only ownership (user or group id) differences, ignore any
missing files:

```shell
tdiff -0 --uid --gid directory1 directory2
```

&nbsp;

Report only group permission bits differences, ignore any missing files:

```shell
tdiff -0 --mode --mode-and 70 directory1 directory2
```
or:

```shell
tdiff -0 --mode --mode-or 7707 directory1 directory2
```

&nbsp;

Report only sticky bits differences, ignore any missing files:

```shell
tdiff -0 --mode --mode-and 1000 directory1 directory2
```
or:

```shell
tdiff -0 --mode --mode-or 6777 directory1 directory2
```

&nbsp;

Run `cmp -l` on every file of the same size in both trees:

```shell
tdiff -0 --exec cmp -l %1 %2 \; directory1 directory2
```

&nbsp;

Run super-diff: diff files with diff -u and reports any other kind of differences in inode contents except for
times:

```shell
tdiff --exec-always-diff directory1 directory2
```

or more tersely:

```shell
tdiff -W directory1 directory2
```

&nbsp;

Same with file modification times:

```shell
tdiff -W --preset mtime directory1 directory2
```

or also:

```shell
tdiff -Wi directory1 directory2
```

## License

**tdiff** is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see [http://www.gnu.org/licenses/].

## Building from source

### Requirements

* C Compiler (eg. gcc)

* Make (eg. GNU make)

* libacl, optionally, for ACL support, if needed on your system.

* autotools (autoconf, automake) is only required if building from the
  repository.

* groff is optionally needed to generate the man page hard copies
  (HTML & PDF). It is only needed if you intend to update the manual
  page ROFF source.

### From a release

Download the [latest release from
GitHub](https://github.com/F-i-f/tdiff/releases/download/v0.7.2/tdiff-0.7.2.tar.gz)
or the [secondary mirror](http://ftp.fifi.org/phil/tdiff/tdiff-0.7.2.tar.gz):

* [Primary Site (GitHub)](https://github.com/F-i-f/tdiff/releases/):

  * Source:
	[https://github.com/F-i-f/tdiff/releases/download/v0.7.2/tdiff-0.7.2.tar.gz](https://github.com/F-i-f/tdiff/releases/download/v0.7.2/tdiff-0.7.2.tar.gz)

  * Signature:
	[https://github.com/F-i-f/tdiff/releases/download/v0.7.2/tdiff-0.7.2.tar.gz.asc](https://github.com/F-i-f/tdiff/releases/download/v0.7.2/tdiff-0.7.2.tar.gz.asc)

* [Secondary Site](http://ftp.fifi.org/phil/tdiff/):

  * Source:
	[http://ftp.fifi.org/phil/tdiff/tdiff-0.7.2.tar.gz](http://ftp.fifi.org/phil/tdiff/tdiff-0.7.2.tar.gz)

  * Signature:
	[http://ftp.fifi.org/phil/tdiff/tdiff-0.7.2.tar.gz.asc](http://ftp.fifi.org/phil/tdiff/tdiff-0.7.2.tar.gz.asc)


The source code release are signed with the GPG key ID `0x88D51582`,
available on your [nearest GPG server](https://pgp.mit.edu/) or
[here](http://ftp.fifi.org/phil/GPG-KEY).

You can also find all releases on the [GitHub release
page](https://github.com/F-i-f/tdiff/releases/) and on the [secondary
mirror](http://ftp.fifi.org/phil/tdiff/).

When downloading from the GitHub release pages, be careful to download
the source code from the link named with the full file name
(_tdiff-0.7.2.tar.gz_), and **not** from the links marked _Source code
(zip)_ or _Source code (tar.gz)_ as these are repository snapshots
generated automatically by GitHub and require specialized tools to
build (see [Building from GitHub](#from-the-github-repository)).


After downloading the sources, unpack and build with:

```shell
tar xvzf tdiff-0.7.2.tar.gz
cd tdiff-0.7.2
./configure
make
make check
make install
make install-pdf install-html # Optional
```

Alternately, you can create a RPM file by moving the source tar file
and the included `tdiff.spec` in your rpm build directory and running:

```shell
rpmbuild -ba SPECS/tdiff.spec
```

### From the GitHub repository

Clone the [repository](https://github.com/F-i-f/tdiff.git):

```shell
git clone https://github.com/F-i-f/tdiff.git
cd tdiff
autoreconf -i
./configure
make
make check
make install
make install-pdf install-html # Optional
```

## Changelog

### Version 0.7.2
#### April 30, 2019.

- Fix crash when an unreadable directory is encountered.
- Fix regression failures on odd systems.
- Fix display overflow in --version.

### Version 0.7.1
#### April 27, 2019.

- Fixed rare bug where files could be skipped (and misreported as
  having no differences) if two files with the name inode number but
  with different device numbers were scanned within the second
  argument's sub-tree.

### Version 0.7
#### April 26, 2019.

#### New features:

- Bash and Zsh completion functions are now included.

- A manual page is now included.

- New -W --always-exec-diff option.

#### User-visible changes:

- The (unimplemented) option -e / --hardlinks has been (temporarily)
  removed.

- When reporting mode discrepancies (-m / --mode), if no mask is used
  with --mode-or or --mode-and, then masked values are not shown.

- The -| / --mode-or and -&amp; / --mode-and options now take their
  arguments as octal (no need to lead them with a zero to force octal
  parsing).

- The -p / --no-mmap option has been removed.

- Using the -v / --verbose option more than once now increases the
  logging level.

- The -V / --version option now prints compiled in features (ACLs,
  xattrs, etc.)

- The command specified in -w / --exec-always is not run anymore if
  both files to be compared are hard-links of each other.

- Updated BSD / MacOSX flags.

#### Bugs fixed:

- Fixed bug where paths could be wrongly displayed (when one of the
  arguments was / or ../ for example).

- Change the inode cache hash function to DJB2 (performance).

- Times could sometimes be compared incorrectly.

- Compare symbolic links contents only when requested.

- Crash fixed when the command line for -x / --exec or -w /
  --exec-always did not reference both %1 and %2.

#### Internal changes

- Move to Git, git-aware build scripts.

- Open files with O_NOATIME.

- Factorize some autoconf code into macros.

- Fix build errors when cross-compiling.

- Support getdents64 on Linux.

- Portability fixes for FreeBSD 12, MacOSX, Android, Solaris.

- Add test suite.

- Include RPM spec file.

### Release 0.2
#### January 31, 2014.

- First public release.

- Now includes support for ACLs, extended attributes, and link count.

## Credits

**tdiff** was written by Philippe Troin ([F-i-f on GitHub](https://github.com/F-i-f)).

<!--  LocalWords:  tdiff inode uid gid ACLs UFS MacOSX NROFF nbsp eg
 -->
<!--  LocalWords:  MERCHANTABILITY gcc libacl ACL autotools autoconf
 -->
<!--  LocalWords:  automake groff GPG gz github Zsh hardlinks mmap
 -->
<!--  LocalWords:  xattrs DJB2 NOATIME getdents64 FreeBSD Solaris
 -->
<!--  LocalWords:  Troin Changelog directory1 directory2
 -->
