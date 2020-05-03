// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Represents a generic weight in an FST; that is, represents a specific type
// of weight underneath while hiding that type from a client.

#ifndef FST_SCRIPT_WEIGHT_CLASS_H_
#define FST_SCRIPT_WEIGHT_CLASS_H_

#include <memory>
#include <ostream>
#include <string>

#include <fst/arc.h>
#include <fst/generic-register.h>
#include <fst/util.h>
#include <fst/weight.h>

namespace fst {
namespace script {

class WeightImplBase {
 public:
  virtual WeightImplBase *Copy() const = 0;
  virtual void Print(std::ostream *o) const = 0;
  virtual const std::string &Type() const = 0;
  virtual std::string ToString() const = 0;
  virtual bool Member() const = 0;
  virtual bool operator==(const WeightImplBase &other) const = 0;
  virtual bool operator!=(const WeightImplBase &other) const = 0;
  virtual WeightImplBase &PlusEq(const WeightImplBase &other) = 0;
  virtual WeightImplBase &TimesEq(const WeightImplBase &other) = 0;
  virtual WeightImplBase &DivideEq(const WeightImplBase &other) = 0;
  virtual WeightImplBase &PowerEq(size_t n) = 0;
  virtual ~WeightImplBase() {}
};

template <class W>
class WeightClassImpl : public WeightImplBase {
 public:
  explicit WeightClassImpl(const W &weight) : weight_(weight) {}

  WeightClassImpl<W> *Copy() const final {
    return new WeightClassImpl<W>(weight_);
  }

  const std::string &Type() const final { return W::Type(); }

  void Print(std::ostream *ostrm) const final { *ostrm << weight_; }

  std::string ToString() const final {
    std::string str;
    WeightToStr(weight_, &str);
    return str;
  }

  bool Member() const final { return weight_.Member(); }

  bool operator==(const WeightImplBase &other) const final {
    const auto *typed_other = static_cast<const WeightClassImpl<W> *>(&other);
    return weight_ == typed_other->weight_;
  }

  bool operator!=(const WeightImplBase &other) const final {
    return !(*this == other);
  }

  WeightClassImpl<W> &PlusEq(const WeightImplBase &other) final {
    const auto *typed_other = static_cast<const WeightClassImpl<W> *>(&other);
    weight_ = Plus(weight_, typed_other->weight_);
    return *this;
  }

  WeightClassImpl<W> &TimesEq(const WeightImplBase &other) final {
    const auto *typed_other = static_cast<const WeightClassImpl<W> *>(&other);
    weight_ = Times(weight_, typed_other->weight_);
    return *this;
  }

  WeightClassImpl<W> &DivideEq(const WeightImplBase &other) final {
    const auto *typed_other = static_cast<const WeightClassImpl<W> *>(&other);
    weight_ = Divide(weight_, typed_other->weight_);
    return *this;
  }

  WeightClassImpl<W> &PowerEq(size_t n) final {
    weight_ = Power<W>(weight_, n);
    return *this;
  }

  W *GetImpl() { return &weight_; }

 private:
  W weight_;
};


class WeightClass {
 public:
  WeightClass() = default;

  template <class W>
  explicit WeightClass(const W &weight)
      : impl_(new WeightClassImpl<W>(weight)) {}

  template <class W>
  explicit WeightClass(const WeightClassImpl<W> &impl)
      : impl_(new WeightClassImpl<W>(impl)) {}

  WeightClass(const std::string &weight_type, const std::string &weight_str);

  WeightClass(const WeightClass &other)
      : impl_(other.impl_ ? other.impl_->Copy() : nullptr) {}

  WeightClass &operator=(const WeightClass &other) {
    impl_.reset(other.impl_ ? other.impl_->Copy() : nullptr);
    return *this;
  }

  static constexpr char __ZERO__[] = "__ZERO__";  // NOLINT
  static constexpr char __ONE__[] = "__ONE__";    // NOLINT
  static constexpr char __NOWEIGHT__[] = "__NOWEIGHT__";  // NOLINT

  static WeightClass Zero(const std::string &weight_type);

  static WeightClass One(const std::string &weight_type);

  static WeightClass NoWeight(const std::string &weight_type);

  template <class W>
  const W *GetWeight() const {
    if (W::Type() != impl_->Type()) {
       return nullptr;
    } else {
      auto *typed_impl = static_cast<WeightClassImpl<W> *>(impl_.get());
      return typed_impl->GetImpl();
    }
  }

  std::string ToString() const { return (impl_) ? impl_->ToString() : "none"; }

  const std::string &Type() const {
    if (impl_) return impl_->Type();
    static const std::string *const no_type = new std::string("none");
    return *no_type;
  }

  bool Member() const { return impl_ && impl_->Member(); }

  static bool WeightTypesMatch(const WeightClass &lhs, const WeightClass &rhs,
                               const std::string &op_name);

  friend bool operator==(const WeightClass &lhs, const WeightClass &rhs);

  friend WeightClass Plus(const WeightClass &lhs, const WeightClass &rhs);

  friend WeightClass Times(const WeightClass &lhs, const WeightClass &rhs);

  friend WeightClass Divide(const WeightClass &lhs, const WeightClass &rhs);

  friend WeightClass Power(const WeightClass &w, size_t n);

 private:
  const WeightImplBase *GetImpl() const { return impl_.get(); }

  WeightImplBase *GetImpl() { return impl_.get(); }

  std::unique_ptr<WeightImplBase> impl_;

  friend std::ostream &operator<<(std::ostream &o, const WeightClass &c);
};

bool operator==(const WeightClass &lhs, const WeightClass &rhs);

bool operator!=(const WeightClass &lhs, const WeightClass &rhs);

WeightClass Plus(const WeightClass &lhs, const WeightClass &rhs);

WeightClass Times(const WeightClass &lhs, const WeightClass &rhs);

WeightClass Divide(const WeightClass &lhs, const WeightClass &rhs);

WeightClass Power(const WeightClass &w, size_t n);

std::ostream &operator<<(std::ostream &o, const WeightClass &c);

// Registration for generic weight types.

using StrToWeightImplBaseT = WeightImplBase *(*)(const std::string &str);

template <class W>
WeightImplBase *StrToWeightImplBase(const std::string &str) {
  if (str == WeightClass::__ZERO__) {
    return new WeightClassImpl<W>(W::Zero());
  } else if (str == WeightClass::__ONE__) {
    return new WeightClassImpl<W>(W::One());
  } else if (str == WeightClass::__NOWEIGHT__) {
    return new WeightClassImpl<W>(W::NoWeight());
  }
  return new WeightClassImpl<W>(StrToWeight<W>(str));
}

class WeightClassRegister
    : public GenericRegister<std::string, StrToWeightImplBaseT,
                             WeightClassRegister> {
 protected:
  std::string ConvertKeyToSoFilename(const std::string &key) const final {
    std::string legal_type(key);
    ConvertToLegalCSymbol(&legal_type);
    return legal_type + ".so";
  }
};

using WeightClassRegisterer = GenericRegisterer<WeightClassRegister>;

// Internal version; needs to be called by wrapper in order for macro args to
// expand.
#define REGISTER_FST_WEIGHT__(Weight, line)                \
  static WeightClassRegisterer weight_registerer##_##line( \
      Weight::Type(), StrToWeightImplBase<Weight>)

// This layer is where __FILE__ and __LINE__ are expanded.
#define REGISTER_FST_WEIGHT_EXPANDER(Weight, line) \
  REGISTER_FST_WEIGHT__(Weight, line)

// Macro for registering new weight types; clients call this.
#define REGISTER_FST_WEIGHT(Weight) \
  REGISTER_FST_WEIGHT_EXPANDER(Weight, __LINE__)

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_WEIGHT_CLASS_H_
