#ifndef FST_EXTENSIONS_FAR_ISOMORPHIC_H_
#define FST_EXTENSIONS_FAR_ISOMORPHIC_H_

#include <string>

#include <fst/extensions/far/far.h>
#include <fst/isomorphic.h>

namespace fst {

template <class Arc>
bool FarIsomorphic(const string &filename1, const string &filename2,
                   float delta = kDelta, const string &begin_key = string(),
                   const string &end_key = string()) {
  std::unique_ptr<FarReader<Arc>> reader1(FarReader<Arc>::Open(filename1));
  std::unique_ptr<FarReader<Arc>> reader2(FarReader<Arc>::Open(filename2));
  if (!reader1 || !reader2) {
    VLOG(1) << "FarIsomorphic: cannot open input Far file(s)";
    return false;
  }

  if (!begin_key.empty()) {
    bool find_begin1 = reader1->Find(begin_key);
    bool find_begin2 = reader2->Find(begin_key);
    if (!find_begin1 || !find_begin2) {
      bool ret = !find_begin1 && !find_begin2;
      if (!ret) {
        VLOG(1) << "FarIsomorphic: key \"" << begin_key << "\" missing from "
                << (find_begin1 ? "second" : "first") << " archive.";
      }
      return ret;
    }
  }

  for (; !reader1->Done() && !reader2->Done();
      reader1->Next(), reader2->Next()) {
    const string key1 = reader1->GetKey();
    const string key2 = reader2->GetKey();
    if (!end_key.empty() && end_key < key1 && end_key < key2) return true;
    if (key1 != key2) {
      VLOG(1) << "FarIsomorphic: mismatched keys \""
              << key1 << "\" <> \"" << key2 << "\".";
      return false;
    }
    if (!Isomorphic(reader1->GetFst(), reader2->GetFst(), delta)) {
      VLOG(1) << "FarIsomorphic: Fsts for key \"" << key1
              << "\" are not isomorphic.";
      return false;
    }
  }

  if (!reader1->Done() || !reader2->Done()) {
    VLOG(1) << "FarIsomorphic: key \""
            << (reader1->Done() ? reader2->GetKey() : reader1->GetKey())
            << "\" missing form " << (reader2->Done() ? "first" : "second")
            << " archive.";
    return false;
  }

  return true;
}

}  // namespace fst

#endif  // FST_EXTENSIONS_FAR_ISOMORPHIC_H_
