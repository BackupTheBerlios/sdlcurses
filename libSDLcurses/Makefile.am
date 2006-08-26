AUTOMAKE_OPTIONS = gnu
lib_LTLIBRARIES = libSDL_curses.la

libSDL_cursesincludedir = $(includedir)/SDL
libSDL_cursesinclude_HEADERS =	\
	sdl_ncurses.h

libSDL_curses_la_SOURCES =	\
	sdl_ncurses.c

EXTRA_DIST =			\
	ASCII.fon		\
	ASCII.rar

libSDL_curses_la_LDFLAGS = 	\
	-no-undefined		\
	-release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libSDL_curses_la_LIBADD = $(IMG_LIBS)

# Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: distcheck

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz