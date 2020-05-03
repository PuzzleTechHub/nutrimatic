// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Definitions for encode table header.

#include <fst/encode.h>

namespace fst {

bool EncodeTableHeader::Read(std::istream &strm, const std::string &source) {
  int32 magic_number;
  ReadType(strm, &magic_number);
  if (magic_number == internal::kEncodeMagicNumber) {
    ReadType(strm, &arctype_);
    ReadType(strm, &flags_);
    ReadType(strm, &size_);
  } else if (magic_number == internal::kEncodeDeprecatedMagicNumber) {
    // TODO(b/141172858): deprecated, remove by 2020-01-01.
    uint32 flags;
    ReadType(strm, &flags);
    flags_ = flags;
    int64 size;
    ReadType(strm, &size);
    size_ = size;
  } else {
    LOG(ERROR) << "EncodeTableHeader::Read: Bad encode table header: "
               << source;
    return false;
  }
  if (!strm) {
    LOG(ERROR) << "EncodeTableHeader::Read: Read failed: " << source;
    return false;
  }
  return true;
}

bool EncodeTableHeader::Write(std::ostream &strm,
                              const std::string &source) const {
  WriteType(strm, internal::kEncodeMagicNumber);
  WriteType(strm, arctype_);
  WriteType(strm, flags_);
  WriteType(strm, size_);
  strm.flush();
  if (!strm) {
    LOG(ERROR) << "EncodeTableHeader::Write: Write failed: " << source;
    return false;
  }
  return true;
}

}  // namespace fst
