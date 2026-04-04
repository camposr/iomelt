VERSION		= 1.2
CC			= gcc
CFLAGS		= -O -Wall
EFILE		= iomelt
SRCFILES	= src/main.c src/opts.c src/signals.c src/verbose.c src/io.c src/time.c src/drive.c
HEADERS		= src/iomelt.h
PKG			= iomelt-$(VERSION).tar.gz
PKGFILES	= iomelt/Makefile iomelt/Makefile.am iomelt/configure.ac iomelt/autogen.sh iomelt/README iomelt/src/ iomelt/man1/
INSTALLPATH	= /usr/bin
MANPATH		= /usr/share/man/man1
MANFILE		= man1/iomelt.1

# Enable GNU extensions on Linux (needed for get_current_dir_name, vasprintf)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Linux)
	CFLAGS += -D_GNU_SOURCE
endif

iomelt:		$(SRCFILES) $(HEADERS)
			$(CC) $(CFLAGS) -o $(EFILE) $(SRCFILES)

package:	$(SRCFILES)
			tar cvzfC $(PKG) ../ --exclude=.DS_Store $(PKGFILES)

clean:
			rm -f $(EFILE) $(PKG)

install:	iomelt
			cp $(EFILE) $(INSTALLPATH)/$(EFILE)-$(VERSION)
			strip $(INSTALLPATH)/$(EFILE)-$(VERSION)
			ln -sf $(INSTALLPATH)/$(EFILE)-$(VERSION) $(INSTALLPATH)/$(EFILE)
			cp $(MANFILE) $(MANPATH)
