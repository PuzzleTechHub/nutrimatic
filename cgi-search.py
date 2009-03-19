#!/usr/bin/python
#
# Web interface to Nutrimatic find-expr.  (As seen on http://nutrimatic.org/)
#
# Expects to be run with the pathnames to the find-expr binary and the index.
# Normally this is done with a shell script wrapper which the web server runs.

import cgi
import cgitb; cgitb.enable()
import math
import signal
import subprocess
import sys
import urllib

# When find-expr reports searching this many nodes, give up and
# print the "computation limit exceeded" message
MAX_COMPUTATION = 500000

# Number of results to print per page
PER_PAGE = 100

#####
# HTML output templates

HOME_PAGE_BEGIN = """
<html><head>
<title>Nutrimatic</title>
<style>
  .query { text-decoration: none; }
</style>
</head><body>
<p><em>Almost, but not quite, entirely unlike tea.</em></p>
<form action="" method=get>
<input type=text name=q size=45>
<input type=submit name=go value="Go">
</form>
<p>Searches words and phrases found in Wikipedia,
normalized to lowercase letters and numbers.
More common results are returned first.</p>
"""

HOME_PAGE_TABLE_BEGIN = """
<h3>%(title)s</h3>
<table cellpadding=0 cellspacing=0 border=0 margin=0>
"""

HOME_PAGE_SYNTAX_ROW = """
<tr>
<td class=query>%(syntax)s</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td>%(text)s</td></tr>
"""

HOME_PAGE_EXAMPLE_ROW = """
<tr>
<td><a class=query href="?q=%(link)s">%(query)s</a></td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td>%(text)s</td></tr>
"""

HOME_PAGE_TABLE_END = """
</table>
"""

HOME_PAGE_END = """
</body></html>
"""

RESULT_PAGE_BEGIN = """
<html><head><title>%(query)s - Nutrimatic</title></head>
<body>
<form action="" method=get>
<input type=text name=q value="%(query)s" size=45>
<input type=submit name=go value="Go">
</form>
"""

RESULT_ITEM = "<span style='font-size: %(size)fem'>%(text)s</span><br>"

RESULT_ERROR = "<p><b><font color=red>%(text)s</font></b></p>"

RESULT_TIMEOUT = "<p><b>Computation limit reached.</b></p>"

RESULT_DONE = "<p><b>No more results found.</b></p>"

RESULT_NONE = "<p><b>No results found, sorry.</b></p>"

RESULT_PAGE = "<p><b>Page %(page)d:</b></p>"

RESULT_NEXT = "<p><a href='?q=%(query)s&start=%(num)d'>page %(page)d &raquo;</a></p>"

RESULT_PAGE_END = """
</body></html>
"""

#####
# List of syntax descriptions and examples for the search page

SYNTAX = [
  ("a-z, 0-9, space", "literal match"),
  ("[], (), {}, |, ., ?, *, +", "same as regexp"),
  ("\"expr\"", "don't allow word breaks without explicit space or hyphen"),
  ("expr&expr", "both expressions must match"),
  ("<aaagmnr>, <(gram)(ana)>", "anagram of contents"),
  ("_", "[a-z0-9]: alphanumeric, not space"),
  ("#", "[0-9]: digit"),
  ("-", "( ?): optional space"),
  ("A", "[a-z]: alphabetic"),
  ("C", "consonant (including y)"),
  ("V", "vowel ([aeiou], not y)"),
]

EXAMPLES = [
  ("\"C*aC*eC*iC*oC*uC*yC*\"", "facetiously"),
  ("867-####", "for a good time call"),
  ("\"_ ___ ___ _*burger\"", "lol"),
  ("\"(((((m?o)?c)?h)?i)t?)_(h(a(t(o(ry?)?)?)?)?)?&_{5,}\"",
   "MSPH 12 \"Camouflage\""),
  ("(c?h?a?r?m?&____)(e?l?t?o?n?&____)(c?h?e?s?t?&____)(o?n?e?&__)",
   "MSPH 12 \"Hollywood Walk of Fame\""),
]

if len(sys.argv) != 3:
  print >>sys.stderr, "usage: cgi-search.py /path/to/find-expr /path/to/index"
  sys.exit(1)

me, binary, index = sys.argv

print 'Content-type: text/html'
print

fs = cgi.FieldStorage()
if not fs.has_key("q"):  # No query, emit the home page
  print HOME_PAGE_BEGIN
  print HOME_PAGE_TABLE_BEGIN % {"title": "Syntax"}
  for syntax, text in SYNTAX:
    print HOME_PAGE_SYNTAX_ROW % {
        "syntax": cgi.escape(syntax).replace(" ", "&nbsp;"),
        "text": cgi.escape(text),
      }
  print HOME_PAGE_TABLE_END
  print HOME_PAGE_TABLE_BEGIN % {"title": "Examples"}
  for query, text in EXAMPLES:
    print HOME_PAGE_EXAMPLE_ROW % {
        "link": urllib.quote(query),
        "query": cgi.escape(query).replace(" ", "&nbsp;"),
        "text": cgi.escape(text),
      }
  print HOME_PAGE_TABLE_END
  print HOME_PAGE_END
  sys.exit(0)

query = fs["q"].value
start = fs.has_key("start") and int(fs["start"].value) or 0

# Shell out to the find-exec binary to get results
proc = subprocess.Popen([binary, index, query],
    preexec_fn=lambda: signal.signal(signal.SIGPIPE, signal.SIG_DFL),
    stdout=subprocess.PIPE, stderr=subprocess.PIPE)

print RESULT_PAGE_BEGIN % {"query": cgi.escape(query, quote=True)}

num = 0
while 1:
  line = proc.stdout.readline()
  if not line:
    error = proc.stderr.read().strip()
    if error:
      print RESULT_ERROR % {"text": cgi.escape(error).replace("\n", "<br>")}
    elif num > 0:
      print RESULT_DONE
    else:
      print RESULT_NONE
    break

  score, text = line.strip().split(" ", 1)
  if score == "#" and int(text) >= MAX_COMPUTATION:
    print RESULT_TIMEOUT
    break

  if score == "#":
    continue

  if start > 0 and num == start:
    print RESULT_PAGE % {"page": num // PER_PAGE + 1}

  if num >= start + PER_PAGE:
    print RESULT_NEXT % {
        "query": urllib.quote(query),
        "num": num,
        "page": num // PER_PAGE + 1,
      }
    break

  if num >= start:
    score = float(score)
    if score >= 1.0:
      size = 1.5 + math.log(score) / 5.0
    elif score > 0.0:
      size = 1.5 + math.log(score) / 40.0
    else:
      size = 0.5
    print RESULT_ITEM % {"size": size, "text": cgi.escape(text)}

  num += 1

print RESULT_PAGE_END
