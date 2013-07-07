VERSION		= 0.7
CC				= gcc
CFLAGS		= -O -Wall -D_GNU_SOURCE
EFILE			= iomelt
SRCFILES		= src/iomelt.h src/main.c src/opts.c src/signals.c src/verbose.c src/io.c src/time.c
PKG			= iomelt-$(VERSION).tar.gz
PKGFILES		= iomelt/Makefile iomelt/README iomelt/src/ iomelt/man1/
INSTALLPATH = /usr/bin
MANPATH		= /usr/share/man/man1
MANFILE		= man1/iomelt.1

iomelt:		$(SRCFILES)
				$(CC) $(CFLAGS) -o $(EFILE) $(SRCFILES)

package:		$(SRCFILES)
				/bin/tar cvzfC $(PKG) ../ --exclude=.DS_Store $(PKGFILES)

clean:
				rm -I -f $(EFILE) $(PKG)

install:		iomelt
				cp $(EFILE) $(INSTALLPATH)/$(EFILE)-$(VERSION)
				strip $(INSTALLPATH)/$(EFILE)-$(VERSION)
				ln -sf $(INSTALLPATH)/$(EFILE)-$(VERSION) $(INSTALLPATH)/$(EFILE)
				cp $(MANFILE) $(MANPATH)
