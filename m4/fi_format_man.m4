#  fi-autoconf-macros - A collection of autoconf macros
#  fi_format_man
#  Copyright (C) 2019 Philippe Troin <phil+github-commits@fifi.org>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Usage:
#   FI_FORMAT_MAN(MANPAGE ... , GROFF-OUTPUT-DEVICE ...)
# Effect:
#   Generate groff output devices files (html, pdf, ps) from manpages.
#   The first arguments is the list of manpages to format.
#   The second argument is the list of groff output devices.
#   There must be a dist_doc_DATA target in your Makefile.am so that
#   $(docdir) can be created properly.

AC_DEFUN([FI_FORMAT_MAN_INIT_ONCE],
	 [AM_MISSING_PROG([GROFF], [groff])])

AC_DEFUN([FI_FORMAT_MAN],
[AC_REQUIRE([FI_FORMAT_MAN_INIT_ONCE])
 m4_foreach_w([fi_format_man_manpage], [$1],
	      [m4_foreach_w([fi_format_man_device], [$2],
			    [m4_define([fi_format_man_sed_program],
				       [m4_if(fi_format_man_device, [pdf],
					      ['/\/\(Creation\|Mod\)Date (D:[[0-9]]*-/d'],
					      m4_if(fi_format_man_device, [html],
						    ['/<!-- CreationDate:/d'],
						    []))])
			     m4_define([fi_format_man_make_target],
				       [patsubst([fi_format_man_manpage], [^.*/], []).fi_format_man_device])
			     FI_AUTOMAKE_FRAGMENT(
[# Rule created by FI_FORMAT_MAN_DEVICE(]fi_format_man_manpage[, ]fi_format_man_device[)
.PRECIOUS: fi_format_man_make_target
fi_format_man_make_target: ]fi_format_man_manpage[
	-$(GROFF) -man -T]fi_format_man_device[ ./$< > $][@.new
	@trap 'rm -f $][@.new $][@.tmp' EXIT; \
	if test -s $][@.new; \
	then \
	  if test ! -e $][@; \
	  then \
	    (set -x; mv $][@.new $][@); \
	  else \
	    sed -e fi_format_man_sed_program $][@ > $][@.tmp; \
	    if ! sed -e fi_format_man_sed_program $][@.new | diff -q $][@.tmp -; \
	    then \
	      (set -x; mv $][@.new $][@); \
	    else \
	      (set -x; rm -f $][@.new); \
	      touch $][@; \
	    fi; \
	  fi; \
	fi

all-am: fi_format_man_make_target

maintainer-clean-am: maintainer-clean-fi-format-man-][fi_format_man_make_target

install-]fi_format_man_device[-am: install-]fi_format_man_device[-fi-format-man-][fi_format_man_make_target

.PHONY: install-]fi_format_man_device[-fi-format-man-][fi_format_man_make_target
install-]fi_format_man_device[-fi-format-man-][fi_format_man_make_target: fi_format_man_make_target install-dist_docDATA
	$(INSTALL_DATA) $(srcdir)/fi_format_man_make_target $(DESTDIR)$(docdir)/

uninstall-am: uninstall-fi-format-man-][fi_format_man_make_target

.PHONY: uninstall-fi-format-man-][fi_format_man_make_target
uninstall-fi-format-man-][fi_format_man_make_target:
	rm -f $(DESTDIR)$(docdir)/fi_format_man_make_target

.PHONY: maintainer-clean-fi-format-man-][fi_format_man_make_target
maintainer-clean-fi-format-man-][fi_format_man_make_target:
	rm -f fi_format_man_make_target

# End of FI_FORMAT_MAN_DEVICE(]fi_format_man_manpage[, ]fi_format_man_device[)
])])])
])
