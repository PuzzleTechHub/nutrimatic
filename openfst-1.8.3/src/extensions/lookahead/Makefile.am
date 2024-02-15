AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)
LIBS = ../../lib/libfst.la -lm $(DL_LIBS)

libfstdir = @libfstdir@
libfst_LTLIBRARIES = arc_lookahead-fst.la \
ilabel_lookahead-fst.la olabel_lookahead-fst.la

lib_LTLIBRARIES = libfstlookahead.la

libfstlookahead_la_SOURCES = arc_lookahead-fst.cc ilabel_lookahead-fst.cc \
                             olabel_lookahead-fst.cc
libfstlookahead_la_LDFLAGS = -version-info 26:0:0

arc_lookahead_fst_la_SOURCES = arc_lookahead-fst.cc
arc_lookahead_fst_la_LDFLAGS = -avoid-version -module

ilabel_lookahead_fst_la_SOURCES = ilabel_lookahead-fst.cc
ilabel_lookahead_fst_la_LDFLAGS = -avoid-version -module

olabel_lookahead_fst_la_SOURCES = olabel_lookahead-fst.cc
olabel_lookahead_fst_la_LDFLAGS = -avoid-version -module
