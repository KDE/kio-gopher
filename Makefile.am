## Makfile.am for kio_gopher
## Edit from Makefile.am of kdebase/kioslave/man

INCLUDES= $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)

####### Files

kde_module_LTLIBRARIES = kio_gopher.la

kio_gopher_la_SOURCES = kio_gopher.cpp
kio_gopher_la_LIBADD = -lkio
kio_gopher_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = kio_gopher.h

kdelnk_DATA = gopher.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kio_gopher.pot
