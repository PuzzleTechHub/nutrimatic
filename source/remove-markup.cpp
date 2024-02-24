#include <string>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libxml/xmlreader.h>
#include <tre/regex.h>

using namespace std;

#define DEBUG 0

static regex_t make_regex(const std::string &str) {
  regex_t r;

#if DEBUG
  fprintf(stderr, "Compiling: #%s#\n", str.c_str());
#endif

  static const int opt = REG_EXTENDED | REG_ICASE | REG_UNGREEDY;
  if (int errcode = regcomp(&r, str.c_str(), opt)) {
    char errmsg[256];
    regerror(errcode, &r, errmsg, sizeof(errmsg));
    fprintf(stderr, "regcomp(\"%s\"): %s\n", str.c_str(), errmsg);
    exit(1);
  }
  return r;
}

static bool replace_regex(std::string* in, regex_t const& rx,
                          const char *repl) {
  std::string out;
  int e, pos = 0;
  regmatch_t sub[rx.re_nsub + 1];

  while ((e = regexec(&rx, in->c_str() + pos, rx.re_nsub + 1, sub, 0)) == 0) {
    out.reserve(out.size() + in->size() - pos);
    out.append(*in, pos, sub[0].rm_so);

#if DEBUG
    size_t oldsize = out.size();
#endif

    for (int i = 0; repl[i] != '\0'; ++i) {
      if (repl[i] == '\\' && repl[i+1] >= '0' && repl[i+1] <= '9') {
	if (repl[i+1] - '0' > int(rx.re_nsub)) {
	  fprintf(stderr, "\"%s\": only %zd subexpressions\n", repl, rx.re_nsub);
	  exit(1);
	}

        regmatch_t const& match = sub[repl[i+1] - '0'];
	if (match.rm_so >= 0) {
          out.append(*in, pos + match.rm_so, match.rm_eo - match.rm_so);
        }
	++i;
      } else {
        out.append(1, repl[i]);
      }
    }

#if DEBUG
    fprintf(stderr, "Replaced \"%s\" with \"%s\"\n",
        in->substr(pos + sub[0].rm_so, sub[0].rm_eo - sub[0].rm_so).c_str(),
        out.substr(oldsize).c_str());
#endif

    pos += sub[0].rm_eo ? sub[0].rm_eo : 1;
  }

  if (e != REG_NOMATCH) {
    char errmsg[256];
    regerror(e, &rx, errmsg, sizeof(errmsg));
    fprintf(stderr, "regexec: %s\n", errmsg);
    exit(1);
  }

  if (pos != 0) {
    out.append(*in, pos, in->size() - pos);
    in->swap(out);
    return true;
  } else {
    return false;
  }
}

static void do_page(std::string& title, std::string& text) {
  if (text.empty() || title.empty()) return;

  static const regex_t redirect = make_regex("^#REDIRECT");
  if (replace_regex(&text, redirect, "")) return;

  static const std::string NB = "(?:[][]?[^][])*";

  static const regex_t remove = make_regex(
      "<!--.*-->|"
      "<ref([^>]*[^/>])?>.*</ref>|"
      "<gallery([^>]*[^/>])?>.*</gallery>|"
      "<imagemap([^>]*[^/>])?>.*</imagemap>|"
      "\\[\\[[a-z-]*:" + NB + "\\]\\]|"
      "\\{\\|([^{|]|\\{[^|]|\\|[^}])*\\|+\\}|"
      "\\{\\{[^{}]*\\}\\}");

  static const regex_t markup = make_regex(
      "</?[a-z][a-z0-9]*( [^>]*)?/?>");

  static const regex_t entity = make_regex(
      "&[a-z]+;");

  static const regex_t urllink = make_regex(
      "\\[(?:http|https|ftp)://[^] ]*( [^]]*)?\\]");

  static const regex_t wikilink = make_regex("\\[\\["
      "(" + NB + "\\|)?"
      "(" + NB + ")"
      "(\\|" + NB + ")?\\]\\]");

  printf("BEGIN ARTICLE: %s\n", title.c_str());

  while (replace_regex(&text, remove, "") ||
         replace_regex(&text, markup, " ") ||
         replace_regex(&text, entity, " ") ||
	 replace_regex(&text, wikilink, "\\2") ||
	 replace_regex(&text, urllink, "\\1")) ;

  static const regex_t marker = make_regex("(BEGIN|END) ARTICLE");
  replace_regex(&text, marker, ">\\0");

  printf("%s\nEND ARTICLE: %s\n", text.c_str(), title.c_str());
}

int main(int argc, char* argv[]) {
  if (argc > 1 || isatty(0)) {
    fprintf(stderr, "usage: bzcat pages-articles.xml.bz2 | %s\n", argv[0]);
    return 2;
  }

  xmlTextReaderPtr reader = xmlReaderForFd(0, "stdin", NULL,
      XML_PARSE_NONET | XML_PARSE_NOCDATA | XML_PARSE_COMPACT);
  if (reader == NULL) return 1;

  int ret;
  std::string title, text, *current = NULL;
  while ((ret = xmlTextReaderRead(reader)) == 1) {
    char const* name = (char const*) xmlTextReaderConstName(reader);
    if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
      if (current != NULL)
        current->append((char const*) xmlTextReaderConstValue(reader));
    } else if (!strcmp(name, "page")) {
      do_page(title, text);
      title.clear();
      text.clear();
    } else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
      current = NULL;
    } else if (!strcmp(name, "title")) {
      current = &title;
    } else if (!strcmp(name, "text")) {
      current = &text;
    }
  }

  xmlFreeTextReader(reader);
  return (ret != 0);
}
