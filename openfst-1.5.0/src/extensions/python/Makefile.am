python_LTLIBRARIES = fst.la

pyexec_LTILIBRARIES = fst.la
fst_la_SOURCES = fst.cc
fst_la_CPPFLAGS = -I$(srcdir)/../../include $(PYTHON_CPPFLAGS)
fst_la_LDFLAGS = $(PYTHON_LDFLAGS) -avoid-version -module
fst_la_LIBADD = ../../script/libfstscript.la ../../lib/libfst.la \
								-lfstscript -lfst -lm -ldl

# NB: we use the Cython-generated .cc, rather than the .pyx source file, so 
# modifications to the .pyx will not influence the build.
