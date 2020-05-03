AM_CPPFLAGS = -I$(srcdir)/../../include -I$(srcdir)/../../bin $(ICU_CPPFLAGS)
LIBS = ../../lib/libfst.la -lm $(DL_LIBS)

if HAVE_BIN
bin_PROGRAMS = fstspecial

fstspecial_SOURCES = fstspecial.cc phi-fst.cc rho-fst.cc sigma-fst.cc
fstspecial_CPPFLAGS = $(AM_CPPFLAGS)
fstspecial_LDADD = ../../script/libfstscript.la
endif

libfstdir = @libfstdir@
libfst_LTLIBRARIES = phi-fst.la rho-fst.la sigma-fst.la

lib_LTLIBRARIES = libfstspecial.la

libfstspecial_la_SOURCES = phi-fst.cc rho-fst.cc sigma-fst.cc
libfstspecial_la_LDFLAGS = -version-info 20:0:0

phi_fst_la_SOURCES = phi-fst.cc
phi_fst_la_LDFLAGS = -avoid-version -module

rho_fst_la_SOURCES = rho-fst.cc
rho_fst_la_LDFLAGS = -avoid-version -module

sigma_fst_la_SOURCES = sigma-fst.cc
sigma_fst_la_LDFLAGS = -avoid-version -module
