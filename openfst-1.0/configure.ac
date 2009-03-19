AC_INIT([OpenFst], [1.0], [help@www.openfst.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/lib/fst.cc])
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/include/Makefile
  src/lib/Makefile
  src/bin/Makefile
  src/test/Makefile
])
AC_LANG([C++])
AC_CHECK_TYPES([std::tr1::hash<long long unsigned>], [], [], 
               [[#include <tr1/unordered_map>]])
AC_CHECK_TYPES([__gnu_cxx::slist<int>], [], [], 
               [[#include <ext/slist>]])

AC_OUTPUT
