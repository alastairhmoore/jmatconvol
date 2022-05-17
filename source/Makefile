# ----------------------------------------------------------------------------
#
#  Copyright (C) 2010-2020 Fons Adriaensen <fons@linuxaudio.org>
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
#
# ----------------------------------------------------------------------------


PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

VERSION = 0.5.2
CPPFLAGS += -MMD -MP -DVERSION=\"$(VERSION)\"
CXXFLAGS += -O2 -Wall
CXXFLAGS += -march=native
LDLIBS += -lsndfile -lfftw3f -lfftw3 -lpthread


all:	jmatconvol fmatconvol 


JMATCONVOL_O =	jmatconvol.o denseconv.o config.o audiofile.o \
	dither.o sstring.o pxthread.o jclient.o
jmatconvol:	LDLIBS += -ljack
jmatconvol:	$(JMATCONVOL_O)
	$(CXX) $(LDFLAGS) -o $@ $(JMATCONVOL_O) $(LDLIBS)
$(JMATCONVOL_O):
-include $(JMATCONVOL_O:%.o=%.d)



FMATCONVOL_O =	fmatconvol.o denseconv.o config.o audiofile.o \
	dither.o sstring.o pxthread.o
fmatconvol:	$(FMATCONVOL_O)
	$(CXX) $(LDFLAGS) -o $@ $(FMATCONVOL_O) $(LDLIBS)
$(FMATCONVOL_O):
-include $(FMATCONVOL_O:%.o=%.d)



install:	all
	install -m 755 jmatconvol $(DESTDIR)$(BINDIR)
	install -m 755 fmatconvol $(DESTDIR)$(BINDIR)


uninstall:
	rm -f $(DESTDIR)$(BINDIR)/jmatconvol
	rm -f $(DESTDIR)$(BINDIR)/fmatconvol


clean:
	/bin/rm -f *~ *.o *.a *.d *.so jmatconvol fmatconvol

