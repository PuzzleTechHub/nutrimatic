// fst.cc

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
// FST definitions.

#include <fst/fst.h>

// Include these so they are registered
#include <fst/const-fst.h>
#include <fst/vector-fst.h>
#include <fst/compact-fst.h>

// FST flag definitions

DEFINE_bool(fst_verify_properties, false,
            "Verify fst properties queried by TestProperties");

DEFINE_string(fst_pair_separator, ",",
              "Character separator between printed pair weights; "
              "must be a single character");

DEFINE_string(fst_pair_parentheses, "",
              "Characters enclosing the first weight of a printed pair "
              "weight (and derived classes) to ensure proper I/O of nested "
              "pair weights; "
              "must have size 0 (none) or 2 (open and close parenthesis)");

DEFINE_bool(fst_default_cache_gc, true, "Enable garbage collection of cache");

DEFINE_int64(fst_default_cache_gc_limit, 1<<20LL,
             "Cache byte size that triggers garbage collection");

namespace fst {

// Register VectorFst and ConstFst for common arcs types
REGISTER_FST(VectorFst, StdArc);
REGISTER_FST(VectorFst, LogArc);
REGISTER_FST(ConstFst, StdArc);
REGISTER_FST(ConstFst, LogArc);

// Register ConstFst for common arcs types with uint8, uint16 and
// uint64 size types
static fst::FstRegisterer< ConstFst<StdArc, uint8> >
        ConstFst_StdArc_uint8_registerer;
static fst::FstRegisterer< ConstFst<LogArc, uint8> >
        ConstFst_LogArc_uint8_registerer;
static fst::FstRegisterer< ConstFst<StdArc, uint16> >
        ConstFst_StdArc_uint16_registerer;
static fst::FstRegisterer< ConstFst<LogArc, uint16> >
        ConstFst_LogArc_uint16_registerer;
static fst::FstRegisterer< ConstFst<StdArc, uint64> >
        ConstFst_StdArc_uint64_registerer;
static fst::FstRegisterer< ConstFst<LogArc, uint64> >
        ConstFst_LogArc_uint64_registerer;

// Register CompactFst for common arcs with the default (uint32) size type
static fst::FstRegisterer<
  CompactFst<StdArc, StringCompactor<StdArc> > >
CompactFst_StdArc_StringCompactor_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, StringCompactor<LogArc> > >
CompactFst_LogArc_StringCompactor_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, WeightedStringCompactor<StdArc> > >
CompactFst_StdArc_WeightedStringCompactor_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, WeightedStringCompactor<LogArc> > >
CompactFst_LogArc_WeightedStringCompactor_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, AcceptorCompactor<StdArc> > >
CompactFst_StdArc_AcceptorCompactor_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, AcceptorCompactor<LogArc> > >
CompactFst_LogArc_AcceptorCompactor_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, UnweightedCompactor<StdArc> > >
CompactFst_StdArc_UnweightedCompactor_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, UnweightedCompactor<LogArc> > >
CompactFst_LogArc_UnweightedCompactor_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, UnweightedAcceptorCompactor<StdArc> > >
CompactFst_StdArc_UnweightedAcceptorCompactor_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, UnweightedAcceptorCompactor<LogArc> > >
CompactFst_LogArc_UnweightedAcceptorCompactor_registerer;

// Register CompactFst for common arcs with uint8 size type
static fst::FstRegisterer<
  CompactFst<StdArc, StringCompactor<StdArc>, uint8> >
CompactFst_StdArc_StringCompactor_uint8_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, StringCompactor<LogArc>, uint8> >
CompactFst_LogArc_StringCompactor_uint8_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, WeightedStringCompactor<StdArc>, uint8> >
CompactFst_StdArc_WeightedStringCompactor_uint8_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, WeightedStringCompactor<LogArc>, uint8> >
CompactFst_LogArc_WeightedStringCompactor_uint8_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, AcceptorCompactor<StdArc>, uint8> >
CompactFst_StdArc_AcceptorCompactor_uint8_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, AcceptorCompactor<LogArc>, uint8> >
CompactFst_LogArc_AcceptorCompactor_uint8_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, UnweightedCompactor<StdArc>, uint8> >
CompactFst_StdArc_UnweightedCompactor_uint8_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, UnweightedCompactor<LogArc>, uint8> >
CompactFst_LogArc_UnweightedCompactor_uint8_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, UnweightedAcceptorCompactor<StdArc>, uint8> >
CompactFst_StdArc_UnweightedAcceptorCompactor_uint8_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, UnweightedAcceptorCompactor<LogArc>, uint8> >
CompactFst_LogArc_UnweightedAcceptorCompactor_uint8_registerer;

// Register CompactFst for common arcs with uint16 size type
static fst::FstRegisterer<
  CompactFst<StdArc, StringCompactor<StdArc>, uint16> >
CompactFst_StdArc_StringCompactor_uint16_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, StringCompactor<LogArc>, uint16> >
CompactFst_LogArc_StringCompactor_uint16_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, WeightedStringCompactor<StdArc>, uint16> >
CompactFst_StdArc_WeightedStringCompactor_uint16_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, WeightedStringCompactor<LogArc>, uint16> >
CompactFst_LogArc_WeightedStringCompactor_uint16_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, AcceptorCompactor<StdArc>, uint16> >
CompactFst_StdArc_AcceptorCompactor_uint16_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, AcceptorCompactor<LogArc>, uint16> >
CompactFst_LogArc_AcceptorCompactor_uint16_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, UnweightedCompactor<StdArc>, uint16> >
CompactFst_StdArc_UnweightedCompactor_uint16_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, UnweightedCompactor<LogArc>, uint16> >
CompactFst_LogArc_UnweightedCompactor_uint16_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, UnweightedAcceptorCompactor<StdArc>, uint16> >
CompactFst_StdArc_UnweightedAcceptorCompactor_uint16_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, UnweightedAcceptorCompactor<LogArc>, uint16> >
CompactFst_LogArc_UnweightedAcceptorCompactor_uint16_registerer;

// Register CompactFst for common arcs with uint64 size type
static fst::FstRegisterer<
  CompactFst<StdArc, StringCompactor<StdArc>, uint64> >
CompactFst_StdArc_StringCompactor_uint64_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, StringCompactor<LogArc>, uint64> >
CompactFst_LogArc_StringCompactor_uint64_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, WeightedStringCompactor<StdArc>, uint64> >
CompactFst_StdArc_WeightedStringCompactor_uint64_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, WeightedStringCompactor<LogArc>, uint64> >
CompactFst_LogArc_WeightedStringCompactor_uint64_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, AcceptorCompactor<StdArc>, uint64> >
CompactFst_StdArc_AcceptorCompactor_uint64_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, AcceptorCompactor<LogArc>, uint64> >
CompactFst_LogArc_AcceptorCompactor_uint64_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, UnweightedCompactor<StdArc>, uint64> >
CompactFst_StdArc_UnweightedCompactor_uint64_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, UnweightedCompactor<LogArc>, uint64> >
CompactFst_LogArc_UnweightedCompactor_uint64_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, UnweightedAcceptorCompactor<StdArc>, uint64> >
CompactFst_StdArc_UnweightedAcceptorCompactor_uint64_registerer;
static fst::FstRegisterer<
  CompactFst<LogArc, UnweightedAcceptorCompactor<LogArc>, uint64> >
CompactFst_LogArc_UnweightedAcceptorCompactor_uint64_registerer;



// Identifies stream data as an FST (and its endianity)
static const int32 kFstMagicNumber = 2125659606;

// Check for Fst magic number in stream, to indicate
// caller function that the stream content is an Fst header;
bool IsFstHeader(istream &strm, const string &source) {
  int64 pos = strm.tellg();
  bool match = true;
  int32 magic_number = 0;
  ReadType(strm, &magic_number);
  if (magic_number != kFstMagicNumber
      ) {
    match = false;
  }
  strm.seekg(pos);
  return match;
}

// Check Fst magic number and read in Fst header.
// If rewind = true, reposition stream to before call (if possible).
bool FstHeader::Read(istream &strm, const string &source, bool rewind) {
  int64 pos = 0;
  if (rewind) pos = strm.tellg();
  int32 magic_number = 0;
  ReadType(strm, &magic_number);
  if (magic_number != kFstMagicNumber
      ) {
    LOG(ERROR) << "FstHeader::Read: Bad FST header: " << source;
    if (rewind) strm.seekg(pos);
    return false;
  }

  ReadType(strm, &fsttype_);
  ReadType(strm, &arctype_);
  ReadType(strm, &version_);
  ReadType(strm, &flags_);
  ReadType(strm, &properties_);
  ReadType(strm, &start_);
  ReadType(strm, &numstates_);
  ReadType(strm, &numarcs_);
  if (!strm) {
    LOG(ERROR) << "FstHeader::Read: read failed: " << source;
    return false;
  }
  if (rewind) strm.seekg(pos);
  return true;
}

// Write Fst magic number and Fst header.
bool FstHeader::Write(ostream &strm, const string &source) const {
  WriteType(strm, kFstMagicNumber);
  WriteType(strm, fsttype_);
  WriteType(strm, arctype_);
  WriteType(strm, version_);
  WriteType(strm, flags_);
  WriteType(strm, properties_);
  WriteType(strm, start_);
  WriteType(strm, numstates_);
  WriteType(strm, numarcs_);
  return true;
}

}
