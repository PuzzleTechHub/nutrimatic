AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)

if HAVE_BIN
bin_PROGRAMS = pdtcompose pdtexpand pdtinfo pdtreplace pdtreverse \
               pdtshortestpath

LDADD = libfstpdtscript.la \
        ../../script/libfstscript.la \
        ../../lib/libfst.la -lm $(DL_LIBS)

pdtcompose_SOURCES = pdtcompose.cc pdtcompose-main.cc

pdtexpand_SOURCES = pdtexpand.cc pdtexpand-main.cc

pdtinfo_SOURCES = pdtinfo.cc pdtinfo-main.cc

pdtreplace_SOURCES = pdtreplace.cc pdtreplace-main.cc

pdtreverse_SOURCES = pdtreverse.cc pdtreverse-main.cc

pdtshortestpath_SOURCES = pdtshortestpath.cc pdtshortestpath-main.cc
endif

if HAVE_SCRIPT
lib_LTLIBRARIES = libfstpdtscript.la
libfstpdtscript_la_SOURCES = getters.cc pdtscript.cc
libfstpdtscript_la_LDFLAGS = -version-info 26:0:0
libfstpdtscript_la_LIBADD = ../../script/libfstscript.la \
                            ../../lib/libfst.la -lm $(DL_LIBS)
endif
