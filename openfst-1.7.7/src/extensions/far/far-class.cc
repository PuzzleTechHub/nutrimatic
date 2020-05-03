// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/extensions/far/far-class.h>

#include <fst/extensions/far/script-impl.h>
#include <fst/script/script-impl.h>

namespace fst {
namespace script {


// FarReaderClass.

FarReaderClass *FarReaderClass::Open(const std::string &source) {
  const std::vector<std::string> sources{source};
  return FarReaderClass::Open(sources);
}

FarReaderClass *FarReaderClass::Open(const std::vector<std::string> &sources) {
  if (sources.empty()) {
    LOG(ERROR) << "FarReaderClass::Open: No files specified";
    return nullptr;
  }
  const auto arc_type = LoadArcTypeFromFar(sources.front());
  if (arc_type.empty()) return nullptr;
  OpenFarReaderClassArgs args(sources);
  args.retval = nullptr;
  Apply<Operation<OpenFarReaderClassArgs>>("OpenFarReaderClass", arc_type,
                                            &args);
  return args.retval;
}

REGISTER_FST_OPERATION(OpenFarReaderClass, StdArc, OpenFarReaderClassArgs);
REGISTER_FST_OPERATION(OpenFarReaderClass, LogArc, OpenFarReaderClassArgs);
REGISTER_FST_OPERATION(OpenFarReaderClass, Log64Arc, OpenFarReaderClassArgs);

// FarWriterClass.

FarWriterClass *FarWriterClass::Create(const std::string &source,
                                       const std::string &arc_type,
                                       FarType type) {
  CreateFarWriterClassInnerArgs iargs(source, type);
  CreateFarWriterClassArgs args(iargs);
  args.retval = nullptr;
  Apply<Operation<CreateFarWriterClassArgs>>("CreateFarWriterClass", arc_type,
                                             &args);
  return args.retval;
}

REGISTER_FST_OPERATION(CreateFarWriterClass, StdArc, CreateFarWriterClassArgs);
REGISTER_FST_OPERATION(CreateFarWriterClass, LogArc, CreateFarWriterClassArgs);
REGISTER_FST_OPERATION(CreateFarWriterClass, Log64Arc,
                       CreateFarWriterClassArgs);

}  // namespace script
}  // namespace fst
