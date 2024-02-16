# The source for [Nutrimatic](http://nutrimatic.org/)

### To build the source

1. Install [mise-en-place](https://mise.jdx.dev/) as a bootstrap tool

2. You'll need a working C++ build system (debian/ubuntu: `sudo apt install build-essential`)

3. Run `./dev_setup.py` which will install various dependencies locally

4. After that, run `conan build .` which will leave binaries in `build/`

### To build an index

To actually use Nutrimatic, you will need to build an index from Wikipedia.

1. Download the latest Wikipedia database dump (this is a ~20GB file!):

     ```
     wget https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2
     ```

     (You can also look for a
     [mirror closer to you](https://dumps.wikimedia.org/mirrors.html).)

2. Extract the text from the articles using
   [Wikipedia Extractor](http://medialab.di.unipi.it/wiki/Wikipedia_Extractor)
   (this generates ~12GB, and takes hours!):

     ```
     # TODO: There are probably better tools these days!
     pip install wikiextractor  # installs into the local virtualenv
     wikiextractor enwiki-latest-pages-articles.xml.bz2
     ```

   (You may need [this fix](https://github.com/attardi/wikiextractor/pull/182).
   There are probably better extractors these days!)

   This will write many files named `text/??/wiki_??`.

3. Index the text (this generates ~100GB of data, and also takes hours!):

     ```
     find text -type f | xargs cat | build/make-index wikipedia
     ```

   This will write many files named `wikipedia.?????.index`.
   (You can break this up by running `make-index` with different chunks of
   input data, replacing "wikipedia" with unique names each time.)

4. Merge the indexes; I normally do this in two stages:

     ```
     for x in 0 1 2 3 4 5 6 7 8 9
     do build/merge-indexes 2 wikipedia.????$x.index wiki-merged.$x.index
     done
     ```

     followed by

     ```
     build/merge-indexes 5 wiki-merged.*.index wiki-merged.index
     ```

   There's nothing magical about this 10-batch approach, you can use
   any strategy you like. The 2 and 5 numbers are phrase frequency cutoffs
   (how many times a string must occur to be included).

5. Enjoy your new index:

     ```
     build/find-expr wiki-merged.index '<aciimnrttu>'
     ```

### Serving the web interface

If you want to set up the web interface, write a shell wrapper that runs
cgi-search.py with arguments pointing it at your binaries and data files, e.g.:

```
#!/bin/sh
export NUTRIMATIC_FIND_EXPR=/path/to/nutrimatic/build/find-expr
export NUTRIMATIC_INDEX=/path/to/nutrimatic/data/wiki-merged.index
exec /path/to/nutrimatic/cgi-search.py
```

Then arrange for your web server to invoke that shell wrapper as a CGI script.

Have fun,

-- egnor@ofb.net
