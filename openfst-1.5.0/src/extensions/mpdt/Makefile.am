AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)

if HAVE_BIN
bin_PROGRAMS = mpdtcompose mpdtexpand mpdtinfo mpdtreverse

LDADD = libfstmpdtscript.la \
	../../script/libfstscript.la \
	../../lib/libfst.la -lm -ldl

mpdtcompose_SOURCES = mpdtcompose.cc

mpdtexpand_SOURCES = mpdtexpand.cc

mpdtinfo_SOURCES = mpdtinfo.cc

mpdtreverse_SOURCES = mpdtreverse.cc
endif

if HAVE_SCRIPT
libfst_LTLIBRARIES = libfstmpdtscript.la
libfstmpdtscript_la_SOURCES = mpdtscript.cc
libfstmpdtscript_la_LDFLAGS = -version-info 1:5:0
endif

libfstdir = @libfstdir@
