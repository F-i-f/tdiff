Name:		tdiff
Version:	0.8
Release:	1%{?dist}
Summary:	Compare tree permissions, modes, ownership, xattrs, etc

License:	GPLv3
URL:		https://github.com/F-i-f/tdiff
Source0:	%{name}-%{version}.tar.gz
BuildRequires:	fakeroot

%description
Compare file system trees, showing any differences in their:
  - file size,
  - file block count (physical storage size),
  - owner user and group ids (uid & gid),
  - access, modification and inode change times,
  - hard link count, and sets of hard linked files,
  - extended attributes (if supported),
  - ACLs (if supported).

%prep
%setup -q

%build
%configure --enable-compiler-warnings
make %{?_smp_mflags} check

%install
rm -rf $RPM_BUILD_ROOT
%make_install

%files
%{_bindir}/tdiff
%{_mandir}/man1/tdiff.1.gz
%{_datadir}/bash-completion/completions/tdiff
%{_datadir}/zsh/site-functions/_tdiff
%{_docdir}/%{name}

%changelog
* Fri May  3 2019 Philippe Troin <phil@fifi.org> - 0.8-1
- Upstream updated to 0.8.
- Update description.
- Rely on make install to install documentation files.

* Tue Apr 30 2019 Philippe Troin <phil@fifi.org> - 0.7.2-1
- Upstream updated to 0.7.2.

* Sat Apr 27 2019 Philippe Troin <phil@fifi.org> - 0.7.1-1
- Upstream updated to 0.7.1.

* Fri Apr 26 2019 Philippe Troin <phil@fifi.org> - 0.7-1
- Upstream updated to 0.7.
- Install manpage, more documentation.

* Tue Mar 12 2019 Philippe Troin <phil@fifi.org> - 0.2.99.WIP-1.git_0.2+34.g7efe9c0
- Snapshot from github.
- Install bash and zsh completions.

* Mon Feb  3 2014 Philippe Troin <phil@fifi.org> - 0.2-1
- Created.
