// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_ENCODEMAPPER_CLASS_H_
#define FST_SCRIPT_ENCODEMAPPER_CLASS_H_

#include <iostream>
#include <memory>
#include <string>

#include <fst/types.h>
#include <fst/encode.h>
#include <fst/generic-register.h>
#include <fst/script/arc-class.h>
#include <fst/script/fst-class.h>

// Scripting API support for EncodeMapper.

namespace fst {
namespace script {

// Virtual interface implemented by each concrete EncodeMapperClassImpl<Arc>.
class EncodeMapperImplBase {
 public:
  // Returns an encoded ArcClass.
  virtual ArcClass operator()(const ArcClass &) = 0;
  virtual const std::string &ArcType() const = 0;
  virtual const std::string &WeightType() const = 0;
  virtual EncodeMapperImplBase *Copy() const = 0;
  virtual uint8 Flags() const = 0;
  virtual uint64 Properties(uint64) = 0;
  virtual EncodeType Type() const = 0;
  virtual bool Write(const std::string &) const = 0;
  virtual bool Write(std::ostream &, const std::string &) const = 0;
  virtual const SymbolTable *InputSymbols() const = 0;
  virtual const SymbolTable *OutputSymbols() const = 0;
  virtual void SetInputSymbols(const SymbolTable *) = 0;
  virtual void SetOutputSymbols(const SymbolTable *) = 0;
  virtual ~EncodeMapperImplBase() {}
};

// Templated implementation.
template <class Arc>
class EncodeMapperClassImpl : public EncodeMapperImplBase {
 public:
  explicit EncodeMapperClassImpl(const EncodeMapper<Arc> &mapper)
      : mapper_(mapper) {}

  ArcClass operator()(const ArcClass &a) final;

  const std::string &ArcType() const final { return Arc::Type(); }

  const std::string &WeightType() const final { return Arc::Weight::Type(); }

  EncodeMapperClassImpl<Arc> *Copy() const final {
    return new EncodeMapperClassImpl<Arc>(mapper_);
  }

  uint8 Flags() const final { return mapper_.Flags(); }

  uint64 Properties(uint64 inprops) final {
    return mapper_.Properties(inprops);
  }

  EncodeType Type() const final { return mapper_.Type(); }

  bool Write(const std::string &source) const final {
    return mapper_.Write(source);
  }

  bool Write(std::ostream &strm, const std::string &source) const final {
    return mapper_.Write(strm, source);
  }

  const SymbolTable *InputSymbols() const final {
    return mapper_.InputSymbols();
  }

  const SymbolTable *OutputSymbols() const final {
    return mapper_.OutputSymbols();
  }

  void SetInputSymbols(const SymbolTable *syms) final {
    mapper_.SetInputSymbols(syms);
  }

  void SetOutputSymbols(const SymbolTable *syms) final {
    mapper_.SetOutputSymbols(syms);
  }

  ~EncodeMapperClassImpl() override {}

  const EncodeMapper<Arc> *GetImpl() const { return &mapper_; }

  EncodeMapper<Arc> *GetImpl() { return &mapper_; }

 private:
  EncodeMapper<Arc> mapper_;
};

template <class Arc>
inline ArcClass EncodeMapperClassImpl<Arc>::operator()(const ArcClass &a) {
  const Arc arc(a.ilabel, a.olabel,
                *(a.weight.GetWeight<typename Arc::Weight>()), a.nextstate);
  return ArcClass(mapper_(arc));
}

class EncodeMapperClass {
 public:
  EncodeMapperClass() : impl_(nullptr) {}

  EncodeMapperClass(const std::string &arc_type, uint8 flags,
                    EncodeType type = ENCODE);

  template <class Arc>
  explicit EncodeMapperClass(const EncodeMapper<Arc> &mapper)
      : impl_(new EncodeMapperClassImpl<Arc>(mapper)) {}

  EncodeMapperClass(const EncodeMapperClass &other)
      : impl_(other.impl_ == nullptr ? nullptr : other.impl_->Copy()) {}

  EncodeMapperClass &operator=(const EncodeMapperClass &other) {
    impl_.reset(other.impl_ == nullptr ? nullptr : other.impl_->Copy());
    return *this;
  }

  ArcClass operator()(const ArcClass &arc) { return (*impl_)(arc); }

  const std::string &ArcType() const { return impl_->ArcType(); }

  const std::string &WeightType() const { return impl_->WeightType(); }

  uint8 Flags() const { return impl_->Flags(); }

  uint64 Properties(uint64 inprops) { return impl_->Properties(inprops); }

  EncodeType Type() const { return impl_->Type(); }

  static EncodeMapperClass *Read(const std::string &source);

  static EncodeMapperClass *Read(std::istream &strm, const std::string &source);

  bool Write(const std::string &source) const { return impl_->Write(source); }

  bool Write(std::ostream &strm, const std::string &source) const {
    return impl_->Write(strm, source);
  }

  const SymbolTable *InputSymbols() const { return impl_->InputSymbols(); }

  const SymbolTable *OutputSymbols() const { return impl_->OutputSymbols(); }

  void SetInputSymbols(const SymbolTable *syms) {
    impl_->SetInputSymbols(syms);
  }

  void SetOutputSymbols(const SymbolTable *syms) {
    impl_->SetOutputSymbols(syms);
  }

  // Implementation stuff.

  template <class Arc>
  EncodeMapper<Arc> *GetEncodeMapper() {
    if (Arc::Type() != ArcType()) {
      return nullptr;
    } else {
      auto *typed_impl = static_cast<EncodeMapperClassImpl<Arc> *>(impl_.get());
      return typed_impl->GetImpl();
    }
  }

  template <class Arc>
  const EncodeMapper<Arc> *GetEncodeMapper() const {
    if (Arc::Type() != ArcType()) {
      return nullptr;
    } else {
      auto *typed_impl = static_cast<EncodeMapperClassImpl<Arc> *>(impl_.get());
      return typed_impl->GetImpl();
    }
  }

  // Required for registration.

  template <class Arc>
  static EncodeMapperClass *Read(std::istream &strm,
                                 const std::string &source) {
    std::unique_ptr<EncodeMapper<Arc>> mapper(
        EncodeMapper<Arc>::Read(strm, source));
    return mapper ? new EncodeMapperClass(*mapper) : nullptr;
  }

  template <class Arc>
  static EncodeMapperImplBase *Create(uint8 flags, EncodeType type = ENCODE) {
    return new EncodeMapperClassImpl<Arc>(EncodeMapper<Arc>(flags, type));
  }

 private:
  explicit EncodeMapperClass(EncodeMapperImplBase *impl) : impl_(impl) {}

  const EncodeMapperImplBase *GetImpl() const { return impl_.get(); }

  EncodeMapperImplBase *GetImpl() { return impl_.get(); }

  std::unique_ptr<EncodeMapperImplBase> impl_;
};

// Registration for EncodeMapper types.

// This class definition is to avoid a nested class definition inside the
// EncodeMapperIORegistration struct.

template <class Reader, class Creator>
struct EncodeMapperClassRegEntry {
  Reader reader;
  Creator creator;

  EncodeMapperClassRegEntry(Reader reader, Creator creator)
      : reader(reader), creator(creator) {}

  EncodeMapperClassRegEntry() : reader(nullptr), creator(nullptr) {}
};

template <class Reader, class Creator>
class EncodeMapperClassIORegister
    : public GenericRegister<std::string,
                             EncodeMapperClassRegEntry<Reader, Creator>,
                             EncodeMapperClassIORegister<Reader, Creator>> {
 public:
  Reader GetReader(const std::string &arc_type) const {
    return this->GetEntry(arc_type).reader;
  }

  Creator GetCreator(const std::string &arc_type) const {
    return this->GetEntry(arc_type).creator;
  }

 protected:
  std::string ConvertKeyToSoFilename(const std::string &key) const final {
    std::string legal_type(key);
    ConvertToLegalCSymbol(&legal_type);
    return legal_type + "-arc.so";
  }
};

// Struct containing everything needed to register a particular type
struct EncodeMapperClassIORegistration {
  using Reader = EncodeMapperClass *(*)(std::istream &stream,
                                        const std::string &source);

  using Creator = EncodeMapperImplBase *(*)(uint8 flags, EncodeType type);

  using Entry = EncodeMapperClassRegEntry<Reader, Creator>;

  // EncodeMapper register.
  using Register = EncodeMapperClassIORegister<Reader, Creator>;

  // EncodeMapper register-er.
  using Registerer =
      GenericRegisterer<EncodeMapperClassIORegister<Reader, Creator>>;
};

#define REGISTER_ENCODEMAPPER_CLASS(Arc)             \
  static EncodeMapperClassIORegistration::Registerer \
      EncodeMapperClass_##Arc##_registerer(          \
          Arc::Type(),                               \
          EncodeMapperClassIORegistration::Entry(    \
              EncodeMapperClass::Read<Arc>, EncodeMapperClass::Create<Arc>));

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_ENCODEMAPPER_CLASS_H_
