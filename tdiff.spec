Name:		tdiff
Version:	0.8.6
Release:	1%{?dist}
Summary:	Compare tree permissions, modes, ownership, xattrs, etc

License:	GPLv3
URL:		https://github.com/F-i-f/tdiff
Source:		https://github.com/F-i-f/%{name}/releases/download/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:	zsh
BuildRequires:	bash
BuildRequires:	gcc
BuildRequires:	make
BuildRequires:	autoconf
BuildRequires:	automake
BuildRequires:	libacl-devel
%if 0%{?rhel} == 0
BuildRequires:	fakeroot
%endif

# Work-around for Mageia mucking with config.aux files
%if 0%{?mgaversion} > 0
%define _disable_libtoolize 1
%endif

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
%configure --enable-compiler-warnings --docdir="%{_docdir}/%{name}"
make %{?_smp_mflags}
make %{?_smp_mflags} check || ( ./tests-show-results ; exit 1 )

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
* Sat May 21 2022 Philippe Troin <phil@fifi.org> - 0.8.6-1
- Upstream release 0.8.6:
  * Handle mallinfo2 and glibc 2.34.

* Sun Nov  3 2019 Philippe Troin <phil@fifi.org> - 0.8.5-2
- Fix build on RHEL which does not have fakeroot.

* Sat Nov  2 2019 Philippe Troin <phil@fifi.org> - 0.8.5-1
- Upstream release 0.8.5:
  * Minor bug fixes terminal width handling.
  * Improvements in .spec cross-distribution compatibility.

* Sun Aug 25 2019 Philippe Troin <phil@fifi.org> - 0.8.4-3
- Add to BuildRequires.
- Configure with --docdir for non-RHEL/Fedora distributions.
- Run test-show-results upon failure.

* Sun Aug 25 2019 Philippe Troin <phil@fifi.org> - 0.8.4-2
- BuildRequires zsh.
- Use URL in Source.

* Fri Jun 14 2019 Philippe Troin <phil@fifi.org> - 0.8.4-1
- Upstream updated to 0.8.4.

* Wed May 22 2019 Philippe Troin <phil@fifi.org> - 0.8.3-1
- Upstream updated to 0.8.3.

* Sun May 12 2019 Philippe Troin <phil@fifi.org> - 0.8.2-1
- Upstream updated to 0.8.2.

* Tue May  7 2019 Philippe Troin <phil@fifi.org> - 0.8.1-1
- Upstream updated to 0.8.1.
- BuildRequires fakeroot.

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
