AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)

if HAVE_BIN
bin_PROGRAMS = fstcompress

LDADD = libfstcompressscript.la \
        ../../script/libfstscript.la \
        ../../lib/libfst.la \
        -lm $(DL_LIBS)

fstcompress_SOURCES = fstcompress.cc fstcompress-main.cc
endif

if HAVE_SCRIPT
libfstcompressscript_la_SOURCES = compressscript.cc
libfstcompressscript_la_LDFLAGS = -version-info 26:0:0
libfstcompressscript_la_LIBADD = ../../script/libfstscript.la \
                                 ../../lib/libfst.la \
                                 -lm $(DL_LIBS)
endif

if HAVE_SCRIPT
lib_LTLIBRARIES = libfstcompressscript.la
endif
