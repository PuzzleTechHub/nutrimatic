#!/usr/bin/env python
#
# Build Nutrimatic binaries.
#
# Uses "memoize", not make: http://www.eecs.berkeley.edu/~billm/memoize.html
# We do invoke configure and make to build the bundled openfst library.
#
# Re-run this script to do an incremental rebuild; edit the compile() directives
# at the end if you add new programs or source files.

import os
import sys
from glob import glob
from memoize import memoize

CFLAGS = "-g -O6 -Wall -Werror"
LIBS = ""

def run(cmd):
  """ Run a shell command with memoization, exit if it fails """
  status = memoize(cmd)
  if status: sys.exit(1)

def escape(s):
  """ Escape a string for the shell """
  return "'%s'" % s.replace("'", "'\\''")

def getoutput(cmd):
  """ Capture and return the output from a command """
  tmp = "tmp/%s" % cmd.replace("/", "_")
  run("%s >%s" % (cmd, escape(tmp)))
  return file(tmp).read().strip()

def compile(main, others=[], cflags=CFLAGS, libs=LIBS):
  """ Build a C++ binary from source """
  objs = []
  for source in [main] + others:
    objs.append(escape("tmp/%s.o" % source))
    run("g++ -o %s %s -c %s" % (objs[-1], cflags, escape(source)))

  bin = "bin/%s" % main.replace(".cpp", "")
  run("g++ -o %s %s %s" % (escape(bin), " ".join(objs), libs))

#####
# Build the bundled openfst library in tmp/openfst, install it in bin/openfst
# So openfst binaries are bin/openfst/bin, libraries are bin/openfst/lib, etc.
# Note that memoize will remember *all* the inputs and outputs of the build,
# and re-invoke configure/make only if any of them change.

run("mkdir -p bin/openfst tmp/openfst")
run("cd tmp/openfst && ../../openfst-1.0/configure --prefix=%s/bin/openfst" % os.getcwd())
run("make -C tmp/openfst install")
run("cp tmp/openfst/config.h bin/openfst/include/config.h")
fst_cflags = "-Ibin/openfst/include -Wno-sign-compare"
fst_libs = "bin/openfst/lib/libfst.a -lpthread"

#####
# Build the Nutrimatic binaries.

compile("remove-markup.cpp", [],
    cflags=CFLAGS + " " + getoutput("xml2-config --cflags"),
    libs=LIBS + " " + getoutput("xml2-config --libs") + " -ltre")

compile("make-index.cpp", glob("index-*.cpp"))

compile("merge-indexes.cpp", glob("index-*.cpp"))

compile("dump-index.cpp", glob("index-*.cpp"))

compile("explore-index.cpp", glob("index-*.cpp"))

compile("find-anagrams.cpp", glob("search-*.cpp") + glob("index-*.cpp"))

compile("find-phone-words.cpp", glob("search-*.cpp") + glob("index-*.cpp"))

compile("find-expr.cpp",
    glob("expr-*.cpp") + glob("search-*.cpp") + glob("index-*.cpp"),
    cflags=CFLAGS + " " + fst_cflags,
    libs=LIBS + " " + fst_libs)

compile("test-expr.cpp",
    glob("expr-*.cpp") + glob("search-*.cpp") + glob("index-*.cpp"),
    cflags=CFLAGS + " " + fst_cflags,
    libs=LIBS + " " + fst_libs)
