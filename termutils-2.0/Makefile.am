PROGRAMS = tput tabs
LIBRARIES = util
TEXINFOS = tput.texi

util_SOURCES = version.c error.c xmalloc.c tcutil.c fgetline.c \
	getopt.c getopt1.c termcap.c tparam.c $(util_OPT_SOURCES)
util_OPT_SOURCES = bsearch.c
tput_SOURCES = tput.c conversions.c
tabs_SOURCES = tabs.c fspec.c

HEADERS = tput.h tabs.h tcutil.h getopt.h termcap.h
DIST_OTHER = COPYING ChangeLog install-sh texinfo.tex \
	$(HEADERS) fspec.def fspec.sh config.h.in

INCLUDES = -I$(srcdir) -I.

LDADD = libutil.a
CLEANFILES = fspec.c fspec.ct

$(PROGRAMS): libutil.a

fspec.c: fspec.sh fspec.def
	rm -f $@ $@t
	/bin/sh $(srcdir)/fspec.sh $(srcdir)/fspec.def > $@t
	mv $@t $@

conversions.o: tput.h
fspec.o: tabs.h
getopt.o: getopt.h
getopt1.o: getopt.h
tabs.o: termcap.h getopt.h tcutil.h tabs.h
tcutil.o: tcutil.h termcap.h
tput.o: termcap.h getopt.h tcutil.h tput.h

distname:
	echo termutils-`sed -e '/version_string/!d' -e 's/[^0-9.]*\([0-9.]*\).*/\1/' -e q version.c` > $@
