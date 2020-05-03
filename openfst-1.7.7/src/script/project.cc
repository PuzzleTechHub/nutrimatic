// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/project.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {


void Project(MutableFstClass *ofst, ProjectType project_type) {
  ProjectArgs args(ofst, project_type);
  Apply<Operation<ProjectArgs>>("Project", ofst->ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Project, ProjectArgs);

}  // namespace script
}  // namespace fst
