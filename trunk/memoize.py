#!/usr/bin/python

# Copyright (c) 2007-2008,
#   Bill McCloskey    <bill.mccloskey@gmail.com>
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:

# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.

# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.

# 3. The names of the contributors may not be used to endorse or promote
# products derived from this software without specific prior written
# permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import sys
import os
import os.path
import re
import md5
import tempfile
import cPickle
from getopt import getopt

opt_use_modtime = False
opt_dirs = ['.']
opt_show_up_to_date = True
opt_force_build = False
opt_no_deps = False
opt_show_deps = False
opt_verbose = False

def set_use_modtime(use):
    """Use modification time of file, rather than md5sum"""
    global opt_use_modtime
    opt_use_modtime = use

def set_force_build(force):
    """Force things to build."""
    global opt_force_build
    opt_force_build = force

def set_no_deps(nodeps):
    """Force to not use strace"""
    global opt_no_deps
    opt_no_deps = nodeps
    
def set_show_up_to_date(show):
    """Display when files are up to date"""
    global opt_show_up_to_date
    opt_show_up_to_date = show

def set_show_deps(force):
    """Show dependencies for a command after it is run."""
    global opt_show_deps
    opt_show_deps = force

def set_verbose(verbose):
    global opt_verbose
    opt_verbose = verbose

def add_relevant_dir(d):
    opt_dirs.append(d)

def md5sum(fname):
    """Return the md5sum of a given filename, or None if there
    is a problem."""
    if os.path.isdir(fname):
        return fname
    try:
        data = file(fname).read()
    except:
        data = None

    if data == None:
        return None
    else:
        return md5.new(data).hexdigest()

def modtime(fname):
    """Return the modification time of a given filename, or None
    if there is a problem. (i.e: file doesn't exist.)
    """
    try:
        if os.path.isdir(fname):
            return 1
        else:
            return os.path.getmtime(fname)
    except:
        return None

def files_up_to_date(files):
    """Return true if the specified files are up-to-date. Return false
    if files have been modified."""
    for (fname, md5, mtime) in files:
        if opt_use_modtime:
            if modtime(fname) != mtime:
                if opt_verbose:
                    print '(File %s has changed (%s != %s))' % (fname, modtime(fname), mtime)
                return False
        else:
            if md5sum(fname) != md5:
                if opt_verbose:
                    print '(File %s has changed (%s != %s))' % (fname, md5sum(fname), md5)
                return False
    return True

def is_relevant(fname):
    """Returns true if the filename is 'relevant'. That is if we care about
    tracking its up-to-dateness. We want to avoid having to scan system directories
    all the time.
    """
    path1 = os.path.abspath(fname)
    for d in opt_dirs:
        path2 = os.path.abspath(d)
        if path1.startswith(path2) and path1 != path2:
            return True
    return False

# NOTE: One possible problem is that the program output depends
# on whether a directory exists or not, or on the contents
# of a directory.

def generate_deps(cmd, display=None):
    """Generate the dependencies for running a given command."""
    ecmd = cmd
    ecmd = ecmd.replace('\\', '\\\\')
    ecmd = ecmd.replace('"', '\\"')

    outfile = tempfile.mktemp()
    calls = 'open,stat64,execve,exit_group,chdir,mkdir'
    r = os.system('strace -f -o %s -e trace=%s /bin/sh -c "%s"'
                  % (outfile, calls, ecmd))
    if r != 0: return (r, [])

    output = file(outfile).readlines()
    os.remove(outfile)

    cwd = '.'

    status = 0
    files = []
    files_dict = {}
    for line in output:
        match1 = re.match(r'.*open\("([^"]*)", .*', line)
        match2 = re.match(r'.*stat64\("([^"]*)", .*', line)
        match3 = re.match(r'.*execve\("([^"]*)", .*', line)
        match4 = re.match(r'.*mkdir\("([^"]*)", .*', line)
        kill_match = re.match(r'.*killed by.*', line)

        if kill_match:
            return (2, [])

        if match1: match = match1
        elif match2: match = match2
        elif match3: match = match3
        else: match = match4
        if match:
            fname = os.path.normpath(match.group(1))
            name = os.path.normpath(os.path.join(cwd, match.group(1)))
            if (is_relevant(name)
                and (os.path.isfile(name) or os.path.isdir(name) or not os.path.lexists(name))
                and not files_dict.has_key(name)):
                files.append((name, md5sum(name), modtime(name)))
                files_dict[name] = True

        match = re.match(r'.*chdir\("([^"]*)"\)', line)
        if match: cwd = os.path.normpath(os.path.join(cwd, match.group(1)))

        match = re.match(r'.*exit_group\((.*)\).*', line)
        if match: status = int(match.group(1))
    if opt_show_deps:
        print "  depends on: "
        for x in files:
            print "     ", x
    return (status, files)

def read_deps(depsname):
    """Read in the dependencies from the given file."""
    try:
        f = file(depsname, 'rb')
    except: 
        # FIXME: Better error handling required
        f = None

    if f:
        try:
            deps = cPickle.load(f)
        except:
            # If the deps file was corrupt (may occur when the user CTRL+C's
            # mid-save), just throw it away.
            deps = {}
        f.close()
        return deps
    else:
        return {}

def write_deps(depsname, deps):
    """Write the dependencies out to a given file."""
    f = file(depsname, 'wb')
    cPickle.dump(deps, f)
    f.close()

def memoize_with_deps(depsname, deps, cmd, display=None):
    """Run the given command."""
    if display is None:
        display = cmd
    
    files = deps.get(cmd, None)
    if opt_force_build or files is None or not files_up_to_date(files):
        print "*", display
        if opt_no_deps:
            return os.system(cmd)
        else:
            (status, files) = generate_deps(cmd, display)
            if status == 0:
                deps[cmd] = files
            elif deps.has_key(cmd):
                del deps[cmd]
            # FIXME: We don't want to write out the deps every single time!
        write_deps(depsname, deps)
        return status
    else:
        if opt_show_up_to_date:
            print ".", display
        return 0

default_depsname = '.deps'
default_deps = read_deps(default_depsname)

def memoize(cmd, display=None):
    return memoize_with_deps(default_depsname, default_deps, cmd, display)

if __name__ == '__main__':
    (opts, cmd) = getopt(sys.argv[1:], 'td:')
    cmd = ' '.join(cmd)
    for (opt, value) in opts:
        if opt == '-t': opt_use_modtime = True
        elif opt == '-d': opt_dirs.append(value)

    status = memoize(cmd)
    sys.exit(status)
