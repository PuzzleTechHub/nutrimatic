// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_FST_CLASS_H_
#define FST_SCRIPT_FST_CLASS_H_

#include <algorithm>
#include <istream>
#include <limits>
#include <string>
#include <type_traits>

#include <fst/expanded-fst.h>
#include <fst/fst.h>
#include <fst/generic-register.h>
#include <fst/mutable-fst.h>
#include <fst/vector-fst.h>
#include <fst/script/arc-class.h>
#include <fst/script/weight-class.h>

// Classes to support "boxing" all existing types of FST arcs in a single
// FstClass which hides the arc types. This allows clients to load
// and work with FSTs without knowing the arc type. These classes are only
// recommended for use in high-level scripting applications. Most users should
// use the lower-level templated versions corresponding to these classes.

namespace fst {
namespace script {

// Abstract base class defining the set of functionalities implemented in all
// impls and passed through by all bases. Below FstClassBase the class
// hierarchy bifurcates; FstClassImplBase serves as the base class for all
// implementations (of which FstClassImpl is currently the only one) and
// FstClass serves as the base class for all interfaces.

class FstClassBase {
 public:
  virtual const std::string &ArcType() const = 0;
  virtual WeightClass Final(int64) const = 0;
  virtual const std::string &FstType() const = 0;
  virtual const SymbolTable *InputSymbols() const = 0;
  virtual size_t NumArcs(int64) const = 0;
  virtual size_t NumInputEpsilons(int64) const = 0;
  virtual size_t NumOutputEpsilons(int64) const = 0;
  virtual const SymbolTable *OutputSymbols() const = 0;
  virtual uint64 Properties(uint64, bool) const = 0;
  virtual int64 Start() const = 0;
  virtual const std::string &WeightType() const = 0;
  virtual bool ValidStateId(int64) const = 0;
  virtual bool Write(const std::string &) const = 0;
  virtual bool Write(std::ostream &, const std::string &) const = 0;
  virtual ~FstClassBase() {}
};

// Adds all the MutableFst methods.
class FstClassImplBase : public FstClassBase {
 public:
  virtual bool AddArc(int64, const ArcClass &) = 0;
  virtual int64 AddState() = 0;
  virtual void AddStates(size_t) = 0;
  virtual FstClassImplBase *Copy() = 0;
  virtual bool DeleteArcs(int64, size_t) = 0;
  virtual bool DeleteArcs(int64) = 0;
  virtual bool DeleteStates(const std::vector<int64> &) = 0;
  virtual void DeleteStates() = 0;
  virtual SymbolTable *MutableInputSymbols() = 0;
  virtual SymbolTable *MutableOutputSymbols() = 0;
  virtual int64 NumStates() const = 0;
  virtual bool ReserveArcs(int64, size_t) = 0;
  virtual void ReserveStates(int64) = 0;
  virtual void SetInputSymbols(const SymbolTable *) = 0;
  virtual bool SetFinal(int64, const WeightClass &) = 0;
  virtual void SetOutputSymbols(const SymbolTable *) = 0;
  virtual void SetProperties(uint64, uint64) = 0;
  virtual bool SetStart(int64) = 0;
  ~FstClassImplBase() override {}
};

// Containiner class wrapping an Fst<Arc>, hiding its arc type. Whether this
// Fst<Arc> pointer refers to a special kind of FST (e.g. a MutableFst) is
// known by the type of interface class that owns the pointer to this
// container.

template <class Arc>
class FstClassImpl : public FstClassImplBase {
 public:
  explicit FstClassImpl(Fst<Arc> *impl, bool should_own = false)
      : impl_(should_own ? impl : impl->Copy()) {}

  explicit FstClassImpl(const Fst<Arc> &impl) : impl_(impl.Copy()) {}

  // Warning: calling this method casts the FST to a mutable FST.
  bool AddArc(int64 s, const ArcClass &ac) final {
    if (!ValidStateId(s)) return false;
    // Note that we do not check that the destination state is valid, so users
    // can add arcs before they add the corresponding states. Verify can be
    // used to determine whether any arc has a nonexisting destination.
    Arc arc(ac.ilabel, ac.olabel, *ac.weight.GetWeight<typename Arc::Weight>(),
            ac.nextstate);
    static_cast<MutableFst<Arc> *>(impl_.get())->AddArc(s, arc);
    return true;
  }

  // Warning: calling this method casts the FST to a mutable FST.
  int64 AddState() final {
    return static_cast<MutableFst<Arc> *>(impl_.get())->AddState();
  }

  // Warning: calling this method casts the FST to a mutable FST.
  void AddStates(size_t n) final {
    return static_cast<MutableFst<Arc> *>(impl_.get())->AddStates(n);
  }

  const std::string &ArcType() const final { return Arc::Type(); }

  FstClassImpl *Copy() final { return new FstClassImpl<Arc>(impl_.get()); }

  // Warning: calling this method casts the FST to a mutable FST.
  bool DeleteArcs(int64 s, size_t n) final {
    if (!ValidStateId(s)) return false;
    static_cast<MutableFst<Arc> *>(impl_.get())->DeleteArcs(s, n);
    return true;
  }

  // Warning: calling this method casts the FST to a mutable FST.
  bool DeleteArcs(int64 s) final {
    if (!ValidStateId(s)) return false;
    static_cast<MutableFst<Arc> *>(impl_.get())->DeleteArcs(s);
    return true;
  }

  // Warning: calling this method casts the FST to a mutable FST.
  bool DeleteStates(const std::vector<int64> &dstates) final {
    for (const auto &state : dstates)
      if (!ValidStateId(state)) return false;
    // Warning: calling this method with any integers beyond the precision of
    // the underlying FST will result in truncation.
    std::vector<typename Arc::StateId> typed_dstates(dstates.size());
    std::copy(dstates.begin(), dstates.end(), typed_dstates.begin());
    static_cast<MutableFst<Arc> *>(impl_.get())->DeleteStates(typed_dstates);
    return true;
  }

  // Warning: calling this method casts the FST to a mutable FST.
  void DeleteStates() final {
    static_cast<MutableFst<Arc> *>(impl_.get())->DeleteStates();
  }

  WeightClass Final(int64 s) const final {
    if (!ValidStateId(s)) return WeightClass::NoWeight(WeightType());
    WeightClass w(impl_->Final(s));
    return w;
  }

  const std::string &FstType() const final { return impl_->Type(); }

  const SymbolTable *InputSymbols() const final {
    return impl_->InputSymbols();
  }

  // Warning: calling this method casts the FST to a mutable FST.
  SymbolTable *MutableInputSymbols() final {
    return static_cast<MutableFst<Arc> *>(impl_.get())->MutableInputSymbols();
  }

  // Warning: calling this method casts the FST to a mutable FST.
  SymbolTable *MutableOutputSymbols() final {
    return static_cast<MutableFst<Arc> *>(impl_.get())->MutableOutputSymbols();
  }

  // Signals failure by returning size_t max.
  size_t NumArcs(int64 s) const final {
    return ValidStateId(s) ? impl_->NumArcs(s)
                           : std::numeric_limits<size_t>::max();
  }

  // Signals failure by returning size_t max.
  size_t NumInputEpsilons(int64 s) const final {
    return ValidStateId(s) ? impl_->NumInputEpsilons(s)
                           : std::numeric_limits<size_t>::max();
  }

  // Signals failure by returning size_t max.
  size_t NumOutputEpsilons(int64 s) const final {
    return ValidStateId(s) ? impl_->NumOutputEpsilons(s)
                           : std::numeric_limits<size_t>::max();
  }

  // Warning: calling this method casts the FST to a mutable FST.
  int64 NumStates() const final {
    return static_cast<MutableFst<Arc> *>(impl_.get())->NumStates();
  }

  uint64 Properties(uint64 mask, bool test) const final {
    return impl_->Properties(mask, test);
  }

  // Warning: calling this method casts the FST to a mutable FST.
  bool ReserveArcs(int64 s, size_t n) final {
    if (!ValidStateId(s)) return false;
    static_cast<MutableFst<Arc> *>(impl_.get())->ReserveArcs(s, n);
    return true;
  }

  // Warning: calling this method casts the FST to a mutable FST.
  void ReserveStates(int64 n) final {
    static_cast<MutableFst<Arc> *>(impl_.get())->ReserveStates(n);
  }

  const SymbolTable *OutputSymbols() const final {
    return impl_->OutputSymbols();
  }

  // Warning: calling this method casts the FST to a mutable FST.
  void SetInputSymbols(const SymbolTable *isyms) final {
    static_cast<MutableFst<Arc> *>(impl_.get())->SetInputSymbols(isyms);
  }

  // Warning: calling this method casts the FST to a mutable FST.
  bool SetFinal(int64 s, const WeightClass &weight) final {
    if (!ValidStateId(s)) return false;
    static_cast<MutableFst<Arc> *>(impl_.get())
        ->SetFinal(s, *weight.GetWeight<typename Arc::Weight>());
    return true;
  }

  // Warning: calling this method casts the FST to a mutable FST.
  void SetOutputSymbols(const SymbolTable *osyms) final {
    static_cast<MutableFst<Arc> *>(impl_.get())->SetOutputSymbols(osyms);
  }

  // Warning: calling this method casts the FST to a mutable FST.
  void SetProperties(uint64 props, uint64 mask) final {
    static_cast<MutableFst<Arc> *>(impl_.get())->SetProperties(props, mask);
  }

  // Warning: calling this method casts the FST to a mutable FST.
  bool SetStart(int64 s) final {
    if (!ValidStateId(s)) return false;
    static_cast<MutableFst<Arc> *>(impl_.get())->SetStart(s);
    return true;
  }

  int64 Start() const final { return impl_->Start(); }

  bool ValidStateId(int64 s) const final {
    // This cowardly refuses to count states if the FST is not yet expanded.
    if (!Properties(kExpanded, true)) {
      FSTERROR() << "Cannot get number of states for unexpanded FST";
      return false;
    }
    // If the FST is already expanded, CountStates calls NumStates.
    if (s < 0 || s >= CountStates(*impl_)) {
      FSTERROR() << "State ID " << s << " not valid";
      return false;
    }
    return true;
  }

  const std::string &WeightType() const final { return Arc::Weight::Type(); }

  bool Write(const std::string &source) const final {
    return impl_->Write(source);
  }

  bool Write(std::ostream &ostr, const std::string &source) const final {
    const FstWriteOptions opts(source);
    return impl_->Write(ostr, opts);
  }

  ~FstClassImpl() override {}

  Fst<Arc> *GetImpl() const { return impl_.get(); }

 private:
  std::unique_ptr<Fst<Arc>> impl_;
};

// BASE CLASS DEFINITIONS

class MutableFstClass;

class FstClass : public FstClassBase {
 public:
  FstClass() : impl_(nullptr) {}

  template <class Arc>
  explicit FstClass(const Fst<Arc> &fst) : impl_(new FstClassImpl<Arc>(fst)) {}

  FstClass(const FstClass &other)
      : impl_(other.impl_ == nullptr ? nullptr : other.impl_->Copy()) {}

  FstClass &operator=(const FstClass &other) {
    impl_.reset(other.impl_ == nullptr ? nullptr : other.impl_->Copy());
    return *this;
  }

  WeightClass Final(int64 s) const final { return impl_->Final(s); }

  const std::string &ArcType() const final { return impl_->ArcType(); }

  const std::string &FstType() const final { return impl_->FstType(); }

  const SymbolTable *InputSymbols() const final {
    return impl_->InputSymbols();
  }

  size_t NumArcs(int64 s) const final { return impl_->NumArcs(s); }

  size_t NumInputEpsilons(int64 s) const final {
    return impl_->NumInputEpsilons(s);
  }

  size_t NumOutputEpsilons(int64 s) const final {
    return impl_->NumOutputEpsilons(s);
  }

  const SymbolTable *OutputSymbols() const final {
    return impl_->OutputSymbols();
  }

  uint64 Properties(uint64 mask, bool test) const final {
    // Special handling for FSTs with a null impl.
    if (!impl_) return kError & mask;
    return impl_->Properties(mask, test);
  }

  static FstClass *Read(const std::string &source);

  static FstClass *Read(std::istream &istrm, const std::string &source);

  int64 Start() const final { return impl_->Start(); }

  bool ValidStateId(int64 s) const final { return impl_->ValidStateId(s); }

  const std::string &WeightType() const final { return impl_->WeightType(); }

  // Helper that logs an ERROR if the weight type of an FST and a WeightClass
  // don't match.

  bool WeightTypesMatch(const WeightClass &weight,
                        const std::string &op_name) const;

  bool Write(const std::string &source) const final {
    return impl_->Write(source);
  }

  bool Write(std::ostream &ostr, const std::string &source) const final {
    return impl_->Write(ostr, source);
  }

  ~FstClass() override {}

  // These methods are required by IO registration.

  template <class Arc>
  static FstClassImplBase *Convert(const FstClass &other) {
    FSTERROR() << "Doesn't make sense to convert any class to type FstClass";
    return nullptr;
  }

  template <class Arc>
  static FstClassImplBase *Create() {
    FSTERROR() << "Doesn't make sense to create an FstClass with a "
               << "particular arc type";
    return nullptr;
  }

  template <class Arc>
  const Fst<Arc> *GetFst() const {
    if (Arc::Type() != ArcType()) {
      return nullptr;
    } else {
      FstClassImpl<Arc> *typed_impl =
          static_cast<FstClassImpl<Arc> *>(impl_.get());
      return typed_impl->GetImpl();
    }
  }

  template <class Arc>
  static FstClass *Read(std::istream &stream, const FstReadOptions &opts) {
    if (!opts.header) {
      LOG(ERROR) << "FstClass::Read: Options header not specified";
      return nullptr;
    }
    const FstHeader &hdr = *opts.header;
    if (hdr.Properties() & kMutable) {
      return ReadTypedFst<MutableFstClass, MutableFst<Arc>>(stream, opts);
    } else {
      return ReadTypedFst<FstClass, Fst<Arc>>(stream, opts);
    }
  }

 protected:
  explicit FstClass(FstClassImplBase *impl) : impl_(impl) {}

  const FstClassImplBase *GetImpl() const { return impl_.get(); }

  FstClassImplBase *GetImpl() { return impl_.get(); }

  // Generic template method for reading an arc-templated FST of type
  // UnderlyingT, and returning it wrapped as FstClassT, with appropriat
  // error checking. Called from arc-templated Read() static methods.
  template <class FstClassT, class UnderlyingT>
  static FstClassT *ReadTypedFst(std::istream &stream,
                                 const FstReadOptions &opts) {
    std::unique_ptr<UnderlyingT> u(UnderlyingT::Read(stream, opts));
    return u ? new FstClassT(*u) : nullptr;
  }

 private:
  std::unique_ptr<FstClassImplBase> impl_;
};

// Specific types of FstClass with special properties

class MutableFstClass : public FstClass {
 public:
  bool AddArc(int64 s, const ArcClass &ac) {
    if (!WeightTypesMatch(ac.weight, "AddArc")) return false;
    return GetImpl()->AddArc(s, ac);
  }

  int64 AddState() { return GetImpl()->AddState(); }

  void AddStates(size_t n) { return GetImpl()->AddStates(n); }

  bool DeleteArcs(int64 s, size_t n) { return GetImpl()->DeleteArcs(s, n); }

  bool DeleteArcs(int64 s) { return GetImpl()->DeleteArcs(s); }

  bool DeleteStates(const std::vector<int64> &dstates) {
    return GetImpl()->DeleteStates(dstates);
  }

  void DeleteStates() { GetImpl()->DeleteStates(); }

  SymbolTable *MutableInputSymbols() {
    return GetImpl()->MutableInputSymbols();
  }

  SymbolTable *MutableOutputSymbols() {
    return GetImpl()->MutableOutputSymbols();
  }

  int64 NumStates() const { return GetImpl()->NumStates(); }

  bool ReserveArcs(int64 s, size_t n) { return GetImpl()->ReserveArcs(s, n); }

  void ReserveStates(int64 n) { GetImpl()->ReserveStates(n); }

  static MutableFstClass *Read(const std::string &source, bool convert = false);

  void SetInputSymbols(const SymbolTable *isyms) {
    GetImpl()->SetInputSymbols(isyms);
  }

  bool SetFinal(int64 s, const WeightClass &weight) {
    if (!WeightTypesMatch(weight, "SetFinal")) return false;
    return GetImpl()->SetFinal(s, weight);
  }

  void SetOutputSymbols(const SymbolTable *osyms) {
    GetImpl()->SetOutputSymbols(osyms);
  }

  void SetProperties(uint64 props, uint64 mask) {
    GetImpl()->SetProperties(props, mask);
  }

  bool SetStart(int64 s) { return GetImpl()->SetStart(s); }

  template <class Arc>
  explicit MutableFstClass(const MutableFst<Arc> &fst) : FstClass(fst) {}

  // These methods are required by IO registration.

  template <class Arc>
  static FstClassImplBase *Convert(const FstClass &other) {
    FSTERROR() << "Doesn't make sense to convert any class to type "
               << "MutableFstClass";
    return nullptr;
  }

  template <class Arc>
  static FstClassImplBase *Create() {
    FSTERROR() << "Doesn't make sense to create a MutableFstClass with a "
               << "particular arc type";
    return nullptr;
  }

  template <class Arc>
  MutableFst<Arc> *GetMutableFst() {
    Fst<Arc> *fst = const_cast<Fst<Arc> *>(this->GetFst<Arc>());
    MutableFst<Arc> *mfst = static_cast<MutableFst<Arc> *>(fst);
    return mfst;
  }

  template <class Arc>
  static MutableFstClass *Read(std::istream &stream,
                               const FstReadOptions &opts) {
    std::unique_ptr<MutableFst<Arc>> mfst(MutableFst<Arc>::Read(stream, opts));
    return mfst ? new MutableFstClass(*mfst) : nullptr;
  }

 protected:
  explicit MutableFstClass(FstClassImplBase *impl) : FstClass(impl) {}
};

class VectorFstClass : public MutableFstClass {
 public:
  explicit VectorFstClass(FstClassImplBase *impl) : MutableFstClass(impl) {}

  explicit VectorFstClass(const FstClass &other);

  explicit VectorFstClass(const std::string &arc_type);

  static VectorFstClass *Read(const std::string &source);

  template <class Arc>
  static VectorFstClass *Read(std::istream &stream,
                              const FstReadOptions &opts) {
    std::unique_ptr<VectorFst<Arc>> mfst(VectorFst<Arc>::Read(stream, opts));
    return mfst ? new VectorFstClass(*mfst) : nullptr;
  }

  template <class Arc>
  explicit VectorFstClass(const VectorFst<Arc> &fst) : MutableFstClass(fst) {}

  template <class Arc>
  static FstClassImplBase *Convert(const FstClass &other) {
    return new FstClassImpl<Arc>(new VectorFst<Arc>(*other.GetFst<Arc>()),
                                 true);
  }

  template <class Arc>
  static FstClassImplBase *Create() {
    return new FstClassImpl<Arc>(new VectorFst<Arc>(), true);
  }
};

// Registration stuff.

// This class definition is to avoid a nested class definition inside the
// FstClassIORegistration struct.
template <class Reader, class Creator, class Converter>
struct FstClassRegEntry {
  Reader reader;
  Creator creator;
  Converter converter;

  FstClassRegEntry(Reader r, Creator cr, Converter co)
      : reader(r), creator(cr), converter(co) {}

  FstClassRegEntry() : reader(nullptr), creator(nullptr), converter(nullptr) {}
};

// Actual FST IO method register.
template <class Reader, class Creator, class Converter>
class FstClassIORegister
    : public GenericRegister<std::string,
                             FstClassRegEntry<Reader, Creator, Converter>,
                             FstClassIORegister<Reader, Creator, Converter>> {
 public:
  Reader GetReader(const std::string &arc_type) const {
    return this->GetEntry(arc_type).reader;
  }

  Creator GetCreator(const std::string &arc_type) const {
    return this->GetEntry(arc_type).creator;
  }

  Converter GetConverter(const std::string &arc_type) const {
    return this->GetEntry(arc_type).converter;
  }

 protected:
  std::string ConvertKeyToSoFilename(const std::string &key) const final {
    std::string legal_type(key);
    ConvertToLegalCSymbol(&legal_type);
    return legal_type + "-arc.so";
  }
};

// Struct containing everything needed to register a particular type
// of FST class (e.g., a plain FstClass, or a MutableFstClass, etc.).
template <class FstClassType>
struct FstClassIORegistration {
  using Reader = FstClassType *(*)(std::istream &stream,
                                   const FstReadOptions &opts);

  using Creator = FstClassImplBase *(*)();

  using Converter = FstClassImplBase *(*)(const FstClass &other);

  using Entry = FstClassRegEntry<Reader, Creator, Converter>;

  // FST class Register.
  using Register = FstClassIORegister<Reader, Creator, Converter>;

  // FST class Register-er.
  using Registerer =
      GenericRegisterer<FstClassIORegister<Reader, Creator, Converter>>;
};

// Macros for registering other arc types.

#define REGISTER_FST_CLASS(Class, Arc)                                         \
  static FstClassIORegistration<Class>::Registerer Class##_##Arc##_registerer( \
      Arc::Type(),                                                             \
      FstClassIORegistration<Class>::Entry(                                    \
          Class::Read<Arc>, Class::Create<Arc>, Class::Convert<Arc>))

#define REGISTER_FST_CLASSES(Arc)           \
  REGISTER_FST_CLASS(FstClass, Arc);        \
  REGISTER_FST_CLASS(MutableFstClass, Arc); \
  REGISTER_FST_CLASS(VectorFstClass, Arc);

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_FST_CLASS_H_
