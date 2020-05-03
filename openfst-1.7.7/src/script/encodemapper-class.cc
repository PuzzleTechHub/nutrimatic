// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/encodemapper-class.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {
namespace {

// Helper methods.

EncodeMapperClass *ReadEncodeMapper(std::istream &istrm,
                                    const std::string &source) {
  if (!istrm) {
    LOG(ERROR) << "ReadEncodeMapperClass: Can't open file: " << source;
    return nullptr;
  }
  EncodeTableHeader hdr;
  if (!hdr.Read(istrm, source)) return nullptr;
  const auto &arc_type = hdr.ArcType();
  // TODO(b/141172858): deprecated, remove by 2020-01-01.
  if (arc_type.empty()) {
    LOG(ERROR) << "Old-style EncodeMapper cannot be used with script interface";
    return nullptr;
  }
  // The actual reader also consumes the header, so to be kind we rewind.
  istrm.seekg(0, istrm.beg);
  static const auto *reg =
      EncodeMapperClassIORegistration::Register::GetRegister();
  const auto reader = reg->GetReader(arc_type);
  if (!reader) {
    LOG(ERROR) << "EncodeMapperClass::Read: Unknown arc type: " << arc_type;
    return nullptr;
  }
  return reader(istrm, source);
}

EncodeMapperImplBase *CreateEncodeMapper(const std::string &arc_type,
                                         uint8 flags, EncodeType type) {
  static const auto *reg =
      EncodeMapperClassIORegistration::Register::GetRegister();
  auto creator = reg->GetCreator(arc_type);
  if (!creator) {
    FSTERROR() << "EncodeMapperClass: Unknown arc type: " << arc_type;
    return nullptr;
  }
  return creator(flags, type);
}

}  // namespace

EncodeMapperClass::EncodeMapperClass(const std::string &arc_type, uint8 flags,
                                     EncodeType type)
    : impl_(CreateEncodeMapper(arc_type, flags, type)) {}

EncodeMapperClass *EncodeMapperClass::Read(const std::string &source) {
  if (!source.empty()) {
    std::ifstream strm(source, std::ios_base::in | std::ios_base::binary);
    return ReadEncodeMapper(strm, source);
  } else {
    return ReadEncodeMapper(std::cin, "standard input");
  }
}

EncodeMapperClass *EncodeMapperClass::Read(std::istream &strm,
                                           const std::string &source) {
  return ReadEncodeMapper(strm, source);
}

// Registration.

REGISTER_ENCODEMAPPER_CLASS(StdArc);
REGISTER_ENCODEMAPPER_CLASS(LogArc);
REGISTER_ENCODEMAPPER_CLASS(Log64Arc);

}  // namespace script
}  // namespace fst
