AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)

EXTRA_DIST = build_defs.bzl

if HAVE_SCRIPT
lib_LTLIBRARIES = libfstfar.la libfstfarscript.la
else
lib_LTLIBRARIES = libfstfar.la
endif

libfstfar_la_SOURCES = sttable.cc stlist.cc
libfstfar_la_LDFLAGS = -version-info 26:0:0
libfstfar_la_LIBADD = ../../lib/libfst.la -lm $(DL_LIBS)

if HAVE_SCRIPT
libfstfarscript_la_SOURCES = compile-strings.cc far-class.cc farscript.cc \
							 getters.cc script-impl.cc sttable.cc stlist.cc
libfstfarscript_la_LDFLAGS = -version-info 26:0:0
libfstfarscript_la_LIBADD = libfstfar.la ../../script/libfstscript.la \
        					../../lib/libfst.la -lm $(DL_LIBS)
endif

if HAVE_BIN
bin_PROGRAMS = farcompilestrings farconvert farcreate farencode farequal \
			   farextract farinfo farisomorphic farprintstrings

LDADD = libfstfarscript.la ../../script/libfstscript.la \
        ../../lib/libfst.la -lm $(DL_LIBS)

farcompilestrings_SOURCES = farcompilestrings.cc farcompilestrings-main.cc

farconvert_SOURCES = farconvert.cc farconvert-main.cc

farcreate_SOURCES = farcreate.cc farcreate-main.cc

farequal_SOURCES = farequal.cc farequal-main.cc

farencode_SOURCES = farencode.cc farencode-main.cc

farextract_SOURCES = farextract.cc farextract-main.cc

farinfo_SOURCES = farinfo.cc farinfo-main.cc

farisomorphic_SOURCES = farisomorphic.cc farisomorphic-main.cc

farprintstrings_SOURCES = farprintstrings.cc farprintstrings-main.cc
endif
