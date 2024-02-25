#!/usr/bin/python3
#
# Web interface to Nutrimatic find-expr.  (As seen on http://nutrimatic.org/)
#
# Expects to be run with $NUTRIMATIC_FIND_EXPR and $NUTRIMATIC_INDEX set to the
# pathnames of the find-expr binary and the merged .index file, respectively.

import cgi
import cgitb; cgitb.enable()
import html
import math
import os
import resource
import signal
import subprocess
import sys
import urllib

# When find-expr reports searching this many nodes, give up and
# print the "computation limit exceeded" message
MAX_COMPUTATION = 1000000

# Number of results to print per page
PER_PAGE = 100

#####
# HTML output templates

HOME_PAGE_BEGIN = """
<html lang="en"><head>
  <title>Nutrimatic</title>
  <link rel="icon" type="image/vnd.microsoft.icon" href="/favicon.ico">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="google-site-verification" content="HYukS48AhdgGIgHndvQBdN5aoJJHHWnvMq_OJfcpVYg" />
</head><body>
<p><em>Almost, but not quite, entirely unlike tea.</em></p>
<form action="" method=get>
<input type=search name=q size=45 autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false">
<input type=submit name=go value="Go">
</form>
<p>Matches patterns against a dictionary of words and phrases
mined from Wikipedia.  Text is normalized to lowercase letters,
numbers and spaces.  More common results are returned first.</p>
"""

HOME_PAGE_LIST_BEGIN = """
<h3>%(title)s</h3>
<ul>
"""

HOME_PAGE_SYNTAX_ROW = """
<li><em>%(syntax)s</em> - %(text)s
"""

HOME_PAGE_EXAMPLE_ROW = """
<li><a style="text-decoration: none" href="?q=%(param)s">%(query)s</a> - %(text)s
"""

HOME_PAGE_EDITION_ROW = """
<li><a href="%(url)s">%(link)s</a> - %(text)s
"""

HOME_PAGE_LIST_END = """
</ul>
"""

HOME_PAGE_END = """
<h3>More</h3>
<ul>
<li><a href="usage.html">Usage guide</a>: usage tips,
worked examples, why it's slow.
<li><a href="https://github.com/PuzzleTechHub/nutrimatic">Source code</a>:
not completely documented, but it's there!
</ul>

</body></html>
"""

RESULT_PAGE_BEGIN = """
<html lang="en"><head>
  <title>%(query)s - Nutrimatic</title>
  <link rel="icon" type="image/vnd.microsoft.icon" href="/favicon.ico">
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
<form action="" method=get>
<input type=search name=q value="%(query)s" size=45 autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false">
<input type=submit name=go value="Go">
</form>
"""

RESULT_ITEM = "<span style='font-size: %(size)fem'>%(text)s</span><br>"

RESULT_ERROR = "<p><b><font color=red>%(text)s</font></b></p>"

RESULT_TIMEOUT = """
<p><b>Computation limit reached.</b>
<a href='?q=%(query)s&comp=%(next_max)d'>Try %(even)s harder &raquo;</a></p>
"""

RESULT_DONE = "<p><b>No more results found.</b></p>"

RESULT_NONE = "<p><b>No results found, sorry.</b></p>"

RESULT_PAGE = "<p><b>Page %(page)d:</b></p>"

RESULT_NEXT = "<p><a href='?q=%(query)s&start=%(start)d&num=%(num)d'>page %(page)d &raquo;</a></p>"

RESULT_PAGE_END = """
</body></html>
"""

#####
# List of syntax descriptions and examples for the search page

SYNTAX = [
  ("a-z, 0-9, space", "literal match"),
  ("[], (), {}, |, ., ?, *, +", "same as regexp"),
  ("\"expr\"", "forbid word breaks without a space or hyphen"),
  ("expr&expr", "both expressions must match"),
  ("<aaagmnr>, <(gram)(ana)>",
   "anagram of contents (<a href=usage.html#syntax_anagram>note warnings</a>)"),
  ("_ (underscore)", "alphanumeric, not space: [a-z0-9]"),
  ("# (number sign)", "digit: [0-9]"),
  ("- (hyphen)", "optional space: ( ?)"),
  ("A", "alphabetic: [a-z]"),
  ("C", "consonant (including y)"),
  ("V", "vowel ([aeiou], not y)"),
]

EXAMPLES = [
  ("\"C*aC*eC*iC*oC*uC*yC*\"", "facetiously"),
  ("867-####", "for a good time call"),
  ("\"_ ___ ___ _*burger\"", "lol"),
]

EDITIONS = [
  ("https://nutrimatic.org/2016/", "'classic' original"),
  ("https://nutrimatic.org/2024/", "current edition"),
]

binary = os.environ["NUTRIMATIC_FIND_EXPR"]
index = os.environ["NUTRIMATIC_INDEX"]

print('Content-type: text/html')
print()

fs = cgi.FieldStorage()
if 'q' not in fs:  # No query, emit the home page
  print(HOME_PAGE_BEGIN)
  print(HOME_PAGE_LIST_BEGIN % {"title": "Syntax"})
  for syntax, raw in SYNTAX:
    print(HOME_PAGE_SYNTAX_ROW % {
        "syntax": html.escape(syntax).replace(" ", "&nbsp;"),
        "text": raw,
      })
  print(HOME_PAGE_LIST_END)

  print(HOME_PAGE_LIST_BEGIN % {"title": "Examples"})
  for query, text in EXAMPLES:
    print(HOME_PAGE_EXAMPLE_ROW % {
        "param": urllib.parse.quote(query),
        "query": html.escape(query).replace(" ", "&nbsp;"),
        "text": html.escape(text),
      })
  print(HOME_PAGE_LIST_END)

  print(HOME_PAGE_LIST_BEGIN % {"title": "Editions"})
  for url, text in EDITIONS:
    parts = urllib.parse.urlparse(url)._replace(scheme="")
    print(HOME_PAGE_EDITION_ROW % {
        "url": url,
        "link": html.escape(parts.geturl().strip("/")),
        "text": html.escape(text),
      })
  print(HOME_PAGE_LIST_END)
  print(HOME_PAGE_END)
  sys.exit(0)

query = fs.getvalue("q", "").strip()
start = int(fs.getvalue("start", 0))
num = int(fs.getvalue("num", PER_PAGE))
max_computation = int(fs.getvalue("comp", MAX_COMPUTATION))

# Shell out to the find-exec binary to get results
soft, hard = resource.getrlimit(resource.RLIMIT_CPU)
if soft == -1 or soft > 30: soft = 30
resource.setrlimit(resource.RLIMIT_CPU, (soft, hard))

soft, hard = resource.getrlimit(resource.RLIMIT_AS)
if hard == -1 or hard > 2048 * 1024 * 1024: hard = 2048 * 1024 * 1024
resource.setrlimit(resource.RLIMIT_AS, (hard, hard))

proc = subprocess.Popen([binary, index, query],
    preexec_fn=lambda: signal.signal(signal.SIGPIPE, signal.SIG_DFL),
    stdout=subprocess.PIPE, stderr=subprocess.PIPE)

print(RESULT_PAGE_BEGIN % {"query": html.escape(query)})

rn = 0
while 1:
  line = proc.stdout.readline().decode()
  if not line:
    error = proc.stderr.read().decode().strip()
    if error:
      print(RESULT_ERROR % {"text": html.escape(error).replace("\n", "<br>")})
    elif proc.poll():
      if proc.returncode == -signal.SIGXCPU:
        print(RESULT_ERROR % {"text": "find-expr killed: Too much CPU time"})
      elif proc.returncode < 0:
        print(RESULT_ERROR % {"text": "find-expr killed: Signal %d" % -proc.returncode})
      else:
        print(RESULT_ERROR % {"text": "find-expr died: Return code %d" % proc.returncode})
    elif rn > 0:
      print(RESULT_DONE)
    else:
      print(RESULT_NONE)
    break

  score, text = line.strip().split(" ", 1)
  if score == "#" and int(text) >= max_computation:
    print(RESULT_TIMEOUT % {
        "query": urllib.parse.quote(query),
        "even": "even" if max_computation > MAX_COMPUTATION else "",
        "next_max": 2 * max_computation,
    })
    break

  if score == "#":
    continue

  if start > 0 and rn == start:
    print(RESULT_PAGE % {"page": rn // num + 1})

  if rn >= start + num:
    print(RESULT_NEXT % {
        "query": urllib.parse.quote(query),
        "start": start + num,
        "num": num,
        "page": start // num + 2,
      })
    break

  if rn >= start:
    score = float(score)
    if score >= 1.0:
      size = 1.5 + math.log(score) / 5.0
    elif score > 0.0:
      size = 1.5 + math.log(score) / 50.0
    else:
      size = 0
    print(RESULT_ITEM % {"size": max(size, 0.4), "text": html.escape(text)})

  rn += 1

print(RESULT_PAGE_END)
