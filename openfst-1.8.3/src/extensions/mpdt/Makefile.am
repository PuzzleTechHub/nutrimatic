AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)

if HAVE_BIN
bin_PROGRAMS = mpdtcompose mpdtexpand mpdtinfo mpdtreverse

LDADD = libfstmpdtscript.la      \
    ../pdt/libfstpdtscript.la    \
    ../../script/libfstscript.la \
    ../../lib/libfst.la -lm $(DL_LIBS)

mpdtcompose_SOURCES = mpdtcompose.cc mpdtcompose-main.cc

mpdtexpand_SOURCES = mpdtexpand.cc mpdtexpand-main.cc

mpdtinfo_SOURCES = mpdtinfo.cc mpdtinfo-main.cc

mpdtreverse_SOURCES = mpdtreverse.cc mpdtreverse-main.cc
endif

if HAVE_SCRIPT
lib_LTLIBRARIES = libfstmpdtscript.la
libfstmpdtscript_la_SOURCES = mpdtscript.cc
libfstmpdtscript_la_LDFLAGS = -version-info 26:0:0
libfstmpdtscript_la_LIBADD = ../../script/libfstscript.la \
                             ../../lib/libfst.la -lm $(DL_LIBS)
endif
