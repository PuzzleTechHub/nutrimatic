// main.h

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: riley@google.com (Michael Riley)
//
// \file
// Classes and functions for registering and invoking Fst main functions that
// support multiple and extensible arc types.
//

#ifndef FST_MAIN_H__
#define FST_MAIN_H__

#include <map>
#include <string>
#include <utility>

#include <fst/fst.h>

extern "C" {
  typedef void (*ArcInitFunc)();
}

namespace fst {

// This class holds the mapping from arc and main name string pair to a
// templated main implementation function, e.g.,
// 'pair<"ComposeMain", "log"> -> ComposeMain<LogArc>(int ac, char **av, ...)'
class FstMainRegister {
 public:
  typedef int (*Main)(int argc, char **argv, istream &strm,
                      const FstReadOptions &opts);

  static FstMainRegister*GetRegister() {
    FstOnceInit(&register_init_, &FstMainRegister::Init);
    return register_;
  }

  const Main LookupMain(const string &mtype, const string &atype) const {
    MutexLock l(register_lock_);
    map<pair<string, string>, Main>::const_iterator it =
        main_table_.find(make_pair(mtype, atype));
    return it != main_table_.end() ? it->second : 0;
  }

  const Main GetMain(const string &mtype, const string &atype) const {
    Main main = LookupMain(mtype, atype);
    if (main)
      return main;
    string so_file = atype + "-arc.so";
    void *handle = dlopen(so_file.c_str(), RTLD_LAZY);
    if (handle == 0) {
      LOG(ERROR) << "FstMainRegister::GetMain: " << dlerror();
      return 0;
    }
    string init_name =  atype + "_arc_init";
    ArcInitFunc init_func =
        reinterpret_cast<ArcInitFunc>(dlsym(handle, init_name.c_str()));
    if (init_func == 0) {
      LOG(ERROR) << "FstMainRegister::GetMain: " << dlerror();
      return 0;
    }
    (*init_func)();
    return LookupMain(mtype, atype);
  }

  void SetMain(const string &mtype, const string &atype, Main mfunc) {
    MutexLock l(register_lock_);
    main_table_.insert(make_pair(make_pair(mtype, atype), mfunc));
  }

 private:
  static void Init() {
    register_lock_ = new Mutex;
    register_ = new FstMainRegister;
  }

  static FstOnceType register_init_;   // ensures only called once
  static Mutex* register_lock_;           // multithreading lock
  static FstMainRegister *register_;

  map<pair<string, string>, Main> main_table_;
};

// This class registers an Fst operation main for a given arc type.
template <class A>
class FstMainRegisterer {
 public:
  typedef typename FstMainRegister::Main Main;

  FstMainRegisterer(const string &mtype, Main mfunc) {
    FstMainRegister *registr = FstMainRegister::GetRegister();
    registr->SetMain(mtype, A::Type(), mfunc);
  }
};

// Convenience macro to generate static FstMainRegisterer instance.
#define REGISTER_FST_MAIN(M, A) \
static fst::FstMainRegisterer<A> M ## _ ## A ## _registerer(#M, M<A>)

// Invokes mfunc<Arc>. If atype == "", arc type is determined from Fst
// at argv[1].
int CallFstMain(const string &mtype, int argc, char **argv, string atype = "");

// Convenience macro to invoke CallFstMain.
#define CALL_FST_MAIN(M, ac, av) fst::CallFstMain(#M, ac, av)

}  // namespace fst;

#endif  // FST_MAIN_H__
