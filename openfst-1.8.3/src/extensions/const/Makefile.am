AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)
LIBS = ../../lib/libfst.la -lm $(DL_LIBS)

libfstdir = @libfstdir@
libfst_LTLIBRARIES = const8-fst.la const16-fst.la const64-fst.la

lib_LTLIBRARIES = libfstconst.la

libfstconst_la_SOURCES = const8-fst.cc const16-fst.cc const64-fst.cc
libfstconst_la_LDFLAGS = -version-info 26:0:0

const8_fst_la_SOURCES = const8-fst.cc
const8_fst_la_LDFLAGS = -avoid-version -module

const16_fst_la_SOURCES = const16-fst.cc
const16_fst_la_LDFLAGS = -avoid-version -module

const64_fst_la_SOURCES = const64-fst.cc
const64_fst_la_LDFLAGS = -avoid-version -module
