AM_CPPFLAGS = -I$(srcdir)/../include
AM_LDFLAGS = -lm -ldl

lib_LTLIBRARIES = libfstmain.la
libfstmain_la_SOURCES = main.cc
libfstmain_la_LDFLAGS = -version-info 0:0:0

bin_PROGRAMS = fstarcsort fstclosure fstcompile fstcompose fstconcat \
fstconnect fstconvert fstdeterminize fstdifference fstdraw fstencode \
fstepsnormalize fstequal fstequivalent fstinfo fstintersect fstinvert \
fstmap fstminimize fstprint fstproject fstprune fstpush fstrandgen \
fstrelabel fstreplace fstreverse fstreweight fstrmepsilon \
fstshortestdistance fstshortestpath fstsynchronize fsttopsort fstunion

fstarcsort_SOURCES = fstarcsort.cc arcsort-main.h
fstarcsort_LDADD = ../lib/libfst.la libfstmain.la

fstclosure_SOURCES = fstclosure.cc closure-main.h
fstclosure_LDADD = ../lib/libfst.la libfstmain.la

fstcompile_SOURCES = fstcompile.cc compile-main.h
fstcompile_LDADD = ../lib/libfst.la libfstmain.la

fstcompose_SOURCES = fstcompose.cc compose-main.h
fstcompose_LDADD = ../lib/libfst.la libfstmain.la

fstconcat_SOURCES = fstconcat.cc concat-main.h
fstconcat_LDADD = ../lib/libfst.la libfstmain.la

fstconnect_SOURCES = fstconnect.cc connect-main.h
fstconnect_LDADD = ../lib/libfst.la libfstmain.la

fstconvert_SOURCES = fstconvert.cc convert-main.h
fstconvert_LDADD = ../lib/libfst.la libfstmain.la

fstdeterminize_SOURCES = fstdeterminize.cc determinize-main.h
fstdeterminize_LDADD = ../lib/libfst.la libfstmain.la

fstdifference_SOURCES = fstdifference.cc difference-main.h
fstdifference_LDADD = ../lib/libfst.la libfstmain.la

fstdraw_SOURCES = fstdraw.cc draw-main.h
fstdraw_LDADD = ../lib/libfst.la libfstmain.la

fstencode_SOURCES = fstencode.cc encode-main.h
fstencode_LDADD = ../lib/libfst.la libfstmain.la

fstepsnormalize_SOURCES = fstepsnormalize.cc epsnormalize-main.h
fstepsnormalize_LDADD = ../lib/libfst.la libfstmain.la

fstequal_SOURCES = fstequal.cc equal-main.h
fstequal_LDADD = ../lib/libfst.la libfstmain.la

fstequivalent_SOURCES = fstequivalent.cc equivalent-main.h
fstequivalent_LDADD = ../lib/libfst.la libfstmain.la

fstinfo_SOURCES = fstinfo.cc info-main.h
fstinfo_LDADD = ../lib/libfst.la libfstmain.la

fstintersect_SOURCES = fstintersect.cc intersect-main.h
fstintersect_LDADD = ../lib/libfst.la libfstmain.la

fstinvert_SOURCES = fstinvert.cc invert-main.h
fstinvert_LDADD = ../lib/libfst.la libfstmain.la

fstmap_SOURCES = fstmap.cc map-main.h
fstmap_LDADD = ../lib/libfst.la libfstmain.la

fstminimize_SOURCES = fstminimize.cc minimize-main.h
fstminimize_LDADD = ../lib/libfst.la libfstmain.la

fstprint_SOURCES = fstprint.cc print-main.h
fstprint_LDADD = ../lib/libfst.la libfstmain.la

fstproject_SOURCES = fstproject.cc project-main.h
fstproject_LDADD = ../lib/libfst.la libfstmain.la

fstprune_SOURCES = fstprune.cc prune-main.h
fstprune_LDADD = ../lib/libfst.la libfstmain.la

fstpush_SOURCES = fstpush.cc push-main.h
fstpush_LDADD = ../lib/libfst.la libfstmain.la

fstrandgen_SOURCES = fstrandgen.cc randgen-main.h
fstrandgen_LDADD = ../lib/libfst.la libfstmain.la

fstrelabel_SOURCES = fstrelabel.cc relabel-main.h
fstrelabel_LDADD = ../lib/libfst.la libfstmain.la

fstreplace_SOURCES = fstreplace.cc replace-main.h
fstreplace_LDADD = ../lib/libfst.la libfstmain.la

fstreverse_SOURCES = fstreverse.cc reverse-main.h
fstreverse_LDADD = ../lib/libfst.la libfstmain.la

fstreweight_SOURCES = fstreweight.cc reweight-main.h
fstreweight_LDADD = ../lib/libfst.la libfstmain.la

fstrmepsilon_SOURCES = fstrmepsilon.cc rmepsilon-main.h
fstrmepsilon_LDADD = ../lib/libfst.la libfstmain.la

fstshortestdistance_SOURCES = fstshortestdistance.cc shortest-distance-main.h
fstshortestdistance_LDADD = ../lib/libfst.la libfstmain.la

fstshortestpath_SOURCES = fstshortestpath.cc shortest-path-main.h
fstshortestpath_LDADD = ../lib/libfst.la libfstmain.la

fstsynchronize_SOURCES = fstsynchronize.cc synchronize-main.h
fstsynchronize_LDADD = ../lib/libfst.la libfstmain.la

fsttopsort_SOURCES = fsttopsort.cc topsort-main.h
fsttopsort_LDADD = ../lib/libfst.la libfstmain.la

fstunion_SOURCES = fstunion.cc union-main.h
fstunion_LDADD = ../lib/libfst.la libfstmain.la
