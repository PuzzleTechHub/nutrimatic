// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Creates a finite-state archive from component FSTs.

#ifndef FST_EXTENSIONS_FAR_CREATE_H_
#define FST_EXTENSIONS_FAR_CREATE_H_

#include <libgen.h>

#include <sstream>
#include <string>
#include <vector>

#include <fst/extensions/far/far.h>

namespace fst {

template <class Arc>
void FarCreate(const std::vector<std::string> &in_sources,
               const std::string &out_source, const int32 generate_keys,
               const FarType &far_type, const std::string &key_prefix,
               const std::string &key_suffix) {
  std::unique_ptr<FarWriter<Arc>> far_writer(
      FarWriter<Arc>::Create(out_source, far_type));
  if (!far_writer) return;
  for (size_t i = 0; i < in_sources.size(); ++i) {
    std::unique_ptr<Fst<Arc>> ifst(Fst<Arc>::Read(in_sources[i]));
    if (!ifst) return;
    std::string key;
    if (generate_keys > 0) {
      std::ostringstream keybuf;
      keybuf.width(generate_keys);
      keybuf.fill('0');
      keybuf << i + 1;
      key = keybuf.str();
    } else {
      auto *source = new char[in_sources[i].size() + 1];
      strcpy(source, in_sources[i].c_str());  // NOLINT
      key = basename(source);
      delete[] source;
    }
    far_writer->Add(key_prefix + key + key_suffix, *ifst);
  }
}

}  // namespace fst

#endif  // FST_EXTENSIONS_FAR_CREATE_H_
