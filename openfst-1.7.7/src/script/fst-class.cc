// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// These classes are only recommended for use in high-level scripting
// applications. Most users should use the lower-level templated versions
// corresponding to these classes.

#include <fst/script/fst-class.h>

#include <istream>

#include <fst/log.h>
#include <fst/equal.h>
#include <fst/fst-decl.h>
#include <fst/reverse.h>
#include <fst/union.h>

namespace fst {
namespace script {
namespace {

// Helper functions.

template <class F>
F *ReadFstClass(std::istream &istrm, const std::string &source) {
  if (!istrm) {
    LOG(ERROR) << "ReadFstClass: Can't open file: " << source;
    return nullptr;
  }
  FstHeader hdr;
  if (!hdr.Read(istrm, source)) return nullptr;
  const FstReadOptions read_options(source, &hdr);
  const auto &arc_type = hdr.ArcType();
  static const auto *reg = FstClassIORegistration<F>::Register::GetRegister();
  const auto reader = reg->GetReader(arc_type);
  if (!reader) {
    LOG(ERROR) << "ReadFstClass: Unknown arc type: " << arc_type;
    return nullptr;
  }
  return reader(istrm, read_options);
}

template <class F>
FstClassImplBase *CreateFstClass(const std::string &arc_type) {
  static const auto *reg = FstClassIORegistration<F>::Register::GetRegister();
  auto creator = reg->GetCreator(arc_type);
  if (!creator) {
    FSTERROR() << "CreateFstClass: Unknown arc type: " << arc_type;
    return nullptr;
  }
  return creator();
}

template <class F>
FstClassImplBase *ConvertFstClass(const FstClass &other) {
  static const auto *reg = FstClassIORegistration<F>::Register::GetRegister();
  auto converter = reg->GetConverter(other.ArcType());
  if (!converter) {
    FSTERROR() << "ConvertFstClass: Unknown arc type: " << other.ArcType();
    return nullptr;
  }
  return converter(other);
}

}  // namespace

// FstClass methods.

FstClass *FstClass::Read(const std::string &source) {
  if (!source.empty()) {
    std::ifstream istrm(source, std::ios_base::in | std::ios_base::binary);
    return ReadFstClass<FstClass>(istrm, source);
  } else {
    return ReadFstClass<FstClass>(std::cin, "standard input");
  }
}

FstClass *FstClass::Read(std::istream &istrm, const std::string &source) {
  return ReadFstClass<FstClass>(istrm, source);
}

bool FstClass::WeightTypesMatch(const WeightClass &weight,
                                const std::string &op_name) const {
  if (WeightType() != weight.Type()) {
    FSTERROR() << op_name << ": FST and weight with non-matching weight types: "
               << WeightType() << " and " << weight.Type();
    return false;
  }
  return true;
}

// MutableFstClass methods.

MutableFstClass *MutableFstClass::Read(const std::string &source,
                                       bool convert) {
  if (convert == false) {
    if (!source.empty()) {
      std::ifstream in(source, std::ios_base::in | std::ios_base::binary);
      return ReadFstClass<MutableFstClass>(in, source);
    } else {
      return ReadFstClass<MutableFstClass>(std::cin, "standard input");
    }
  } else {  // Converts to VectorFstClass if not mutable.
    std::unique_ptr<FstClass> ifst(FstClass::Read(source));
    if (!ifst) return nullptr;
    if (ifst->Properties(kMutable, false) == kMutable) {
      return static_cast<MutableFstClass *>(ifst.release());
    } else {
      return new VectorFstClass(*ifst.release());
    }
  }
}

// VectorFstClass methods.

VectorFstClass *VectorFstClass::Read(const std::string &source) {
  if (!source.empty()) {
    std::ifstream in(source, std::ios_base::in | std::ios_base::binary);
    return ReadFstClass<VectorFstClass>(in, source);
  } else {
    return ReadFstClass<VectorFstClass>(std::cin, "standard input");
  }
}

VectorFstClass::VectorFstClass(const std::string &arc_type)
    : MutableFstClass(CreateFstClass<VectorFstClass>(arc_type)) {}

VectorFstClass::VectorFstClass(const FstClass &other)
    : MutableFstClass(ConvertFstClass<VectorFstClass>(other)) {}

// Registration.

REGISTER_FST_CLASSES(StdArc);
REGISTER_FST_CLASSES(LogArc);
REGISTER_FST_CLASSES(Log64Arc);

}  // namespace script
}  // namespace fst
