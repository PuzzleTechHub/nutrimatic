AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)
LIBS = ../../lib/libfst.la -lm $(DL_LIBS)

libfstdir = @libfstdir@
libfst_LTLIBRARIES = ngram-fst.la

lib_LTLIBRARIES = libfstngram.la

ngram_fst_la_SOURCES = bitmap-index.cc ngram-fst.cc nthbit.cc
ngram_fst_la_LDFLAGS = -avoid-version -module

libfstngram_la_SOURCES = bitmap-index.cc ngram-fst.cc nthbit.cc
libfstngram_la_LDFLAGS = -version-info 26:0:0
