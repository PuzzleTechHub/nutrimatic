#!/usr/bin/python
#
# Slack interface (via Web request) to Nutrimatic find-expr.
#
# Expects to be run $NUTRIMATIC_FIND_EXPR and $NUTRIMATIC_INDEX set to the
# pathnames of the find-expr binary and the merged .index file, respectively.
# Normally this is done with a shell script wrapper which the web server runs.

import cgi
import cgitb; cgitb.enable()
import daemon
import json
import math
import os
import resource
import signal
import subprocess
import sys
import time
import urllib
import urllib2

# When find-expr reports searching this many nodes, give up and
# print the "computation limit exceeded" message
MAX_COMPUTATION = 1000000

# Maximum number of results to print
MAX_RESULTS = 30

binary = os.environ["NUTRIMATIC_FIND_EXPR"]
index = os.environ["NUTRIMATIC_INDEX"]
fs = cgi.FieldStorage()


def Finalize(json_data):
  for a in json_data.get('attachments', []):
    a['text'] = '\n'.join(filter(bool, [a.get('text')] + a.pop('lines', [])))
    a.setdefault('fallback', a['text'])


def OutputImmediate(json_data):
  Finalize(json_data)
  print 'Content-type: application/json\n'
  json.dump(json_data, sys.stdout)
  print


def OutputDelayed(json_data):
  Finalize(json_data)
  response_url = fs.getvalue('response_url', '')
  if response_url:
    req = urllib2.Request(response_url)
    req.add_header('Content-Type', 'application/json')
    urllib2.urlopen(req, json.dumps(json_data))
  else:
    print
    json.dump(json_data, sys.stdout)  # For debugging.
    print
    sys.stdout.flush()


query = fs.getvalue('text', '').strip()
if not query:
  OutputImmediate({
    'response_type': 'ephemeral',
    'attachments': [{
      'color': 'warning',
      'text': 'Empty Nutrimatic query! Try: */nut &lt;ehllo&gt;*',
      'mrkdwn_in': ['text'],
    }],
  })
  sys.exit(0)

command_name = fs.getvalue('command', '')
response_type = 'ephemeral' if command_name.endswith('q') else 'in_channel'

if fs.has_key('response_url'):
  if response_type == 'in_channel':
    OutputImmediate({'response_type': response_type})
  else:
    print 'Content-type: text/plain'
    print

  sys.stdout.flush()
  dc = daemon.DaemonContext()
  dc.open()

# Shell out to the find-exec binary to get results
soft, hard = resource.getrlimit(resource.RLIMIT_CPU)
if soft == -1 or soft > 30: soft = 30
resource.setrlimit(resource.RLIMIT_CPU, (soft, hard))

soft, hard = resource.getrlimit(resource.RLIMIT_AS)
if hard == -1 or hard > 2048 * 1024 * 1024: hard = 2048 * 1024 * 1024
resource.setrlimit(resource.RLIMIT_AS, (hard, hard))

start_time = time.time()
proc = subprocess.Popen([binary, index, query],
    preexec_fn=lambda: signal.signal(signal.SIGPIPE, signal.SIG_DFL),
    stdout=subprocess.PIPE, stderr=subprocess.PIPE)

lines = []

nutrimatic_url = 'https://nutrimatic.org/?q=%s' % urllib.quote_plus(query)

attachment = {
  'title': 'Query: %s' % query,
  'title_link': nutrimatic_url,
  'lines': lines,
  'mrkdwn_in': ['text'],
}

output = {
  'response_type': response_type,
  'attachments': [attachment],
}

rn = 0
while 1:
  line = proc.stdout.readline()
  if not line:
    error = proc.stderr.read().strip()
    if error:
      attachment['color'] = 'warning' if rn else 'danger'
      lines.append('_*%s*_' % cgi.escape(error))
    elif proc.poll():
      if proc.returncode == -signal.SIGXCPU:
        message = '_*Search killed:* Too much CPU time._'
      elif proc.returncode < 0:
        message = '_*Search killed:* Signal %d._' % -proc.returncode
      else:
        message = '_*Search died:* Return code %d._' % proc.returncode
      attachment['color'] = 'warning' if rn else 'danger'
      lines.append(cgi.escape(message))
    elif rn == 0:
      attachment['color'] = 'warning'
      lines.append('_*No results found, sorry.*_')
    break

  score, text = line.strip().split(" ", 1)
  if score == "#" and int(text) >= MAX_COMPUTATION:
    attachment['color'] = 'warning' if rn else 'danger'
    lines.append('_*Computation limit reached.*_')
    break

  if score == "#":
    now = time.time()
    if now > start_time + 2.0:
      lines.append('_Still working..._')
      OutputDelayed(output)
      attachment.clear()
      attachment.update({
        'title': 'Continued: %s' % query,
        'title_link': nutrimatic_url,
        'lines': lines,
        'mrkdwn_in': ['text'],
      })
      lines[:] = []
      start_time = now
    continue

  if rn >= MAX_RESULTS:
    lines.append('<%s|More results on nutrimatic.org...>' % nutrimatic_url)
    break

  score = float(score)
  attachment['color'] = 'good'
  lines.append(cgi.escape(('*%s*' if score >= 2.0 else '%s') % text))
  rn += 1

OutputDelayed(output)
