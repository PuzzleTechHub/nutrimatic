AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)

libfstgzfile_la_SOURCES = fstgzfile.cc
libfstgzfile_la_LDFLAGS = -version-info 1:5:0

if HAVE_BIN
bin_PROGRAMS = fstcompress fstrandmod

LDADD = libfstgzfile.la libfstcompressscript.la \
        ../../script/libfstscript.la \
        ../../lib/libfst.la \
        -lfstgzfile -lz -lm -ldl

fstcompress_SOURCES = fstcompress.cc
fstrandmod_SOURCES = fstrandmod.cc
endif

if HAVE_SCRIPT
libfstcompressscript_la_SOURCES = compress-script.cc fstgzfile.cc
libfstcompressscript_la_LDFLAGS = -version-info 1:5:0
endif

if HAVE_SCRIPT
libfst_LTLIBRARIES = libfstcompressscript.la libfstgzfile.la
else
libfst_LTLIBRARIES = libfstgzfile.la
endif

libfstdir = @libfstdir@
