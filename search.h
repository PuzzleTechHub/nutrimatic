#include <deque>
#include <queue>
#include <set>
#include <string>
#include <vector>

struct SearchFilter {
  typedef int State;
  virtual bool is_accepting(State state) const = 0;
  virtual bool has_transition(State from, char ch, State* to) const = 0;
  virtual ~SearchFilter() { }
};

class SearchDriver {
 public:
  const char* text;
  double score;

  SearchDriver(const IndexReader*,
               const SearchFilter*,
               SearchFilter::State start,
               double restart);

  bool step();
  void next() { while (!step()) ; }

 private:
  struct Next {
    int crumb;
    double scale;
    IndexReader::Choice choice;
    SearchFilter::State state;
    bool operator<(Next const& n) const {
      return choice.count * scale < n.choice.count * n.scale;
    }
  };

  struct Crumb {
    int parent;
    char ch;
  };

  std::priority_queue<Next> nexts;
  std::deque<Crumb> crumbs;
  std::vector<IndexReader::Choice> tmp;
  std::set<std::string> seen;
  const IndexReader* const reader;
  const SearchFilter* const filter;
  const double restart;
};

void PrintAll(SearchDriver*);
