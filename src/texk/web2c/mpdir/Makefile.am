# Copyright 2008 Taco Hoekwater.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AUTOMAKE_OPTIONS = foreign

KPSEBUILDDIR = ../../../texk/kpathsea/.libs
KPSESRCDIR = ../../../../src/texk/kpathsea
KPSELIB=$(KPSEBUILDDIR)/libkpathsea.a
AM_CFLAGS=-I. -I../.. -I$(KPSESRCDIR)/.. -I$(KPSESRCDIR) -I../../../../src/libs/lua51
CORE_MP_SOURCES=mp.c psout.c tfmin.c memio.c

noinst_LTLIBRARIES    = libmplib.la
libmplib_la_SOURCES   = avl.c $(CORE_MP_SOURCES)

bin_PROGRAMS          = mpost
mpost_SOURCES         = mpost.c mpxout.c
mpost_DEPENDENCIES    = libmplib.la
mpost_LDADD           = libmplib.la $(KPSELIB) -lm

pkglib_LTLIBRARIES    =
if WITH_LUA
pkglib_LTLIBRARIES    += mplib.la
endif
mplib_la_SOURCES      = lmplib.c
mplib_la_DEPENDENCIES = libmplib.la
mplib_la_LIBADD       = libmplib.la
mplib_la_LDFLAGS      = -module -avoid-version

lmplib.c: mp.c

mp.c:     psout.c

mpost.c:  mpxout.c

.w.c:
	$(CTANGLE) $< $@

clean::
	$(RM) mp*.h $(CORE_MP_SOURCES) mpost.c mpxout.c
	$(RM) .libs/* libmplib.a mpost *.lo *.la *.o 

distclean:: clean
	$(RM) Makefile config.h config.status config.log libtool
