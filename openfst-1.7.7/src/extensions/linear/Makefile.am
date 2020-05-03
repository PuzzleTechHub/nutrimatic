AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)
LIBS = ../../lib/libfst.la -lm $(DL_LIBS)

if HAVE_BIN
bin_PROGRAMS = fstlinear fstloglinearapply

fstlinear_SOURCES = fstlinear.cc fstlinear-main.cc
fstlinear_LDADD = libfstlinearscript.la ../../script/libfstscript.la

fstloglinearapply_SOURCES = fstloglinearapply.cc fstloglinearapply-main.cc
fstloglinearapply_LDADD = libfstlinearscript.la ../../script/libfstscript.la
endif

if HAVE_SCRIPT
lib_LTLIBRARIES = libfstlinearscript.la

libfstlinearscript_la_SOURCES = linearscript.cc
libfstlinearscript_la_LDFLAGS = -version-info 20:0:0
libfstlinearscript_la_LIBADD = ../../script/libfstscript.la
endif

libfst_LTLIBRARIES = linear_tagger-fst.la linear_classifier-fst.la

libfstdir = @libfstdir@

linear_tagger_fst_la_SOURCES = linear-tagger-fst.cc
linear_tagger_fst_la_LDFLAGS = -avoid-version -module

linear_classifier_fst_la_SOURCES = linear-classifier-fst.cc
linear_classifier_fst_la_LDFLAGS = -avoid-version -module
