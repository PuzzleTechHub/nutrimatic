AM_CPPFLAGS = -I$(srcdir)/../include
AM_LDFLAGS = -lm -ldl

check_PROGRAMS = fst_test weight_test algo_test

fst_test_SOURCES = fst_test.cc
fst_test_LDADD = ../lib/libfst.la

weight_test_SOURCES = weight_test.cc weight-tester.h
weight_test_LDADD = ../lib/libfst.la

algo_test_SOURCES = algo_test.cc
algo_test_LDADD = ../lib/libfst.la

TESTS = fst_test weight_test algo_test
