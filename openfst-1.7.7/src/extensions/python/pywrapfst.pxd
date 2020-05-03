#cython: language_level=3
# See www.openfst.org for extensive documentation on this weighted
# finite-state transducer library.


from libc.time cimport time
from libc.time cimport time_t

from libcpp cimport bool
from libcpp.memory cimport shared_ptr
from libcpp.memory cimport unique_ptr
from libcpp.string cimport string
from libcpp.utility cimport pair
from libcpp.vector cimport vector

from cintegral_types cimport *

cimport cpywrapfst as fst

from cios cimport ostream
from cios cimport ofstream
from cios cimport stringstream


# Exportable helper functions.


cdef string tostring(data) except *

cdef string weight_tostring(data) except *

cdef fst.ComposeFilter _get_compose_filter(
    const string &compose_filter) except *

cdef fst.DeterminizeType _get_determinize_type(const string &det_type) except *

cdef fst.QueueType _get_queue_type(const string &queue_type) except *

cdef fst.RandArcSelection _get_rand_arc_selection(
    const string &replace_label_type) except *

cdef fst.ReplaceLabelType _get_replace_label_type(
    const string &replace_label_type,
    bool epsilon_on_replace) except *


# Weight.


cdef fst.WeightClass _get_WeightClass_or_One(const string &weight_type,
                                             weight_string) except *

cdef fst.WeightClass _get_WeightClass_or_Zero(const string &weight_type,
                                              weight_string) except *


cdef class Weight(object):

  cdef unique_ptr[fst.WeightClass] _weight

  cdef void _check_weight(self) except *

  cpdef Weight copy(self)

  cpdef string to_string(self)

  cpdef string type(self)

  cpdef bool member(self)


cdef Weight _Zero(weight_type)

cdef Weight _One(weight_type)

cdef Weight _NoWeight(weight_type)

cdef Weight _plus(Weight lhs, Weight rhs)

cdef Weight _times(Weight lhs, Weight rhs)

cdef Weight _divide(Weight lhs, Weight rhs)

cdef Weight _power(Weight lhs, size_t n)


# SymbolTable.

ctypedef fst.SymbolTable * SymbolTable_ptr
ctypedef const fst.SymbolTable * const_SymbolTable_ptr


cdef class _SymbolTable(object):

  cdef const fst.SymbolTable *_raw(self)

  cdef void _raise_nonexistent(self) except *

  cdef const fst.SymbolTable *_raw_ptr_or_raise(self) except *

  cpdef int64 available_key(self) except *

  cpdef bytes checksum(self)

  cpdef SymbolTable copy(self)

  cpdef int64 get_nth_key(self, ssize_t pos) except *

  cpdef bytes labeled_checksum(self)

  cpdef bool member(self, key) except *

  cpdef string name(self) except *

  cpdef size_t num_symbols(self) except *

  cpdef void write(self, source) except *

  cpdef void write_text(self, source) except *

  cpdef bytes write_to_string(self)


cdef class _EncodeMapperSymbolTableView(_SymbolTable):

  # Indicates whether this view is of an input or output SymbolTable
  cdef bool _input_side

  cdef shared_ptr[fst.EncodeMapperClass] _mapper


cdef class _FstSymbolTableView(_SymbolTable):

  # Indicates whether this view is of an input or output SymbolTable
  cdef bool _input_side

  cdef shared_ptr[fst.FstClass] _fst


cdef class _MutableSymbolTable(_SymbolTable):

  cdef fst.SymbolTable *_mutable_raw(self)

  cdef fst.SymbolTable *_mutable_raw_ptr_or_raise(self) except *

  cpdef int64 add_symbol(self, symbol, int64 key=?) except *

  cpdef void add_table(self, _SymbolTable syms) except *

  cpdef void set_name(self, new_name) except *


cdef class _MutableFstSymbolTableView(_MutableSymbolTable):

  # Indicates whether this view is of an input or output SymbolTable
  cdef bool _input_side

  cdef shared_ptr[fst.MutableFstClass] _mfst


cdef class SymbolTable(_MutableSymbolTable):

  cdef unique_ptr[fst.SymbolTable] _smart_table


cdef _EncodeMapperSymbolTableView _init_EncodeMapperSymbolTableView(
    shared_ptr[fst.EncodeMapperClass] encoder, bool input_side)


cdef _FstSymbolTableView _init_FstSymbolTableView(shared_ptr[fst.FstClass] ifst,
                                                  bool input_side)


cdef _MutableFstSymbolTableView _init_MutableFstSymbolTableView(
    shared_ptr[fst.MutableFstClass] ifst, bool input_side)


cdef SymbolTable _init_SymbolTable(unique_ptr[fst.SymbolTable] table)


cpdef _SymbolTable _read_SymbolTable_from_string(state)


cdef class _SymbolTableIterator(object):

  cdef _SymbolTable _table
  cdef unique_ptr[fst.SymbolTableIterator] _siter


# EncodeMapper.


ctypedef fst.EncodeMapperClass * EncodeMapperClass_ptr


cdef class EncodeMapper(object):

  cdef shared_ptr[fst.EncodeMapperClass] _mapper

  cpdef string arc_type(self)

  cpdef string weight_type(self)

  cpdef uint8 flags(self)

  cpdef uint64 properties(self, uint64 mask)

  cpdef void write(self, source) except *

  cpdef bytes write_to_string(self)

  cpdef _EncodeMapperSymbolTableView input_symbols(self)

  cpdef _EncodeMapperSymbolTableView output_symbols(self)

  cdef void _set_input_symbols(self, _SymbolTable syms) except *

  cdef void _set_output_symbols(self, _SymbolTable syms) except *


cdef EncodeMapper _init_EncodeMapper(EncodeMapperClass_ptr mapper)

cpdef EncodeMapper _read_EncodeMapper_from_string(state)


# Fst.


ctypedef fst.FstClass * FstClass_ptr
ctypedef const fst.FstClass * const_FstClass_ptr
ctypedef fst.MutableFstClass * MutableFstClass_ptr
ctypedef fst.VectorFstClass * VectorFstClass_ptr


cdef class Fst(object):

  cdef shared_ptr[fst.FstClass] _fst

  # Google-only...
  @staticmethod
  cdef string _server_render_svg(const string &)
  # ...Google-only.

  @staticmethod
  cdef string _local_render_svg(const string &)

  cpdef string arc_type(self)

  cpdef ArcIterator arcs(self, int64 state)

  cpdef Fst copy(self)

  cpdef void draw(self,
                  source,
                  _SymbolTable isymbols=?,
                  _SymbolTable osymbols=?,
                  _SymbolTable ssymbols=?,
                  bool acceptor=?,
                  title=?,
                  double width=?,
                  double height=?,
                  bool portrait=?,
                  bool vertical=?,
                  double ranksep=?,
                  double nodesep=?,
                  int32 fontsize=?,
                  int32 precision=?,
                  float_format=?,
                  bool show_weight_one=?) except *

  cpdef Weight final(self, int64 state)

  cpdef string fst_type(self)

  cpdef _FstSymbolTableView input_symbols(self)

  cpdef size_t num_arcs(self, int64 state) except *

  cpdef size_t num_input_epsilons(self, int64 state) except *

  cpdef size_t num_output_epsilons(self, int64 state) except *

  cpdef _FstSymbolTableView output_symbols(self)

  cpdef string print(self,
                    _SymbolTable isymbols=?,
                    _SymbolTable osymbols=?,
                    _SymbolTable ssymbols=?,
                    bool acceptor=?,
                    bool show_weight_one=?,
                    missing_sym=?) except *

  cpdef uint64 properties(self, uint64 mask, bool test)

  cpdef int64 start(self)

  cpdef StateIterator states(self)

  cpdef string text(self,
                    _SymbolTable isymbols=?,
                    _SymbolTable osymbols=?,
                    _SymbolTable ssymbols=?,
                    bool acceptor=?,
                    bool show_weight_one=?,
                    missing_sym=?) except *

  cpdef bool verify(self)

  cpdef string weight_type(self)

  cpdef void write(self, source) except *

  cpdef bytes write_to_string(self)


cdef class MutableFst(Fst):

  cdef shared_ptr[fst.MutableFstClass] _mfst

  cdef void _check_mutating_imethod(self) except *

  cdef void _add_arc(self, int64 state, Arc arc) except *

  cpdef int64 add_state(self)

  cpdef void add_states(self, size_t)

  cdef void _arcsort(self, sort_type=?) except *

  cdef void _closure(self, bool closure_plus=?)

  cdef void _concat(self, Fst fst2) except *

  cdef void _connect(self)

  cdef void _decode(self, EncodeMapper) except *

  cdef void _delete_arcs(self, int64 state, size_t n=?) except *

  cdef void _delete_states(self, states=?) except *

  cdef void _encode(self, EncodeMapper) except *

  cdef void _invert(self)

  cdef void _minimize(self, float delta=?, bool allow_nondet=?) except *

  cpdef MutableArcIterator mutable_arcs(self, int64 state)

  cpdef int64 num_states(self)

  cdef void _project(self, bool project_output=?) except *

  cdef void _prune(self, float delta=?, int64 nstate=?, weight=?) except *

  cdef void _push(self,
                  float delta=?,
                  bool remove_total_weight=?,
                  bool to_final=?)

  cdef void _relabel_pairs(self, ipairs=?, opairs=?) except *

  cdef void _relabel_tables(self,
                            _SymbolTable old_isymbols=?,
                            _SymbolTable new_isymbols=?,
                            unknown_isymbol=?,
                            bool attach_new_isymbols=?,
                            _SymbolTable old_osymbols=?,
                            _SymbolTable new_osymbols=?,
                            unknown_osymbol=?,
                            bool attach_new_osymbols=?) except *

  cdef void _reserve_arcs(self, int64 state, size_t n) except *

  cdef void _reserve_states(self, int64 n)

  cdef void _reweight(self, potentials, bool to_final=?) except *

  cdef void _rmepsilon(self,
                       queue_type=?,
                       bool connect=?,
                       weight=?,
                       int64 nstate=?,
                       float delta=?) except *

  cdef void _set_final(self, int64 state, weight=?) except *

  cdef void _set_properties(self, uint64 props, uint64 mask)

  cdef void _set_start(self, int64 state) except *

  cdef void _set_input_symbols(self, _SymbolTable syms) except *

  cdef void _set_output_symbols(self, _SymbolTable syms) except *

  cdef void _topsort(self)


cdef class VectorFst(MutableFst):

    pass


# Construction helpers.


cdef Fst _init_Fst(FstClass_ptr tfst)

cdef MutableFst _init_MutableFst(MutableFstClass_ptr tfst)

cdef Fst _init_XFst(FstClass_ptr tfst)

cpdef Fst _read_Fst(source)

cpdef Fst _read_Fst_from_string(state)


# Iterators.


cdef class Arc(object):

  cdef unique_ptr[fst.ArcClass] _arc

  cpdef Arc copy(self)


cdef Arc _init_Arc(const fst.ArcClass &arc)


cdef class ArcIterator(object):

  cdef shared_ptr[fst.FstClass] _fst
  cdef unique_ptr[fst.ArcIteratorClass] _aiter

  cpdef bool done(self)

  cpdef uint8 flags(self)

  cpdef void next(self)

  cpdef size_t position(self)

  cpdef void reset(self)

  cpdef void seek(self, size_t a)

  cpdef void set_flags(self, uint8 flags, uint8 mask)

  cpdef object value(self)


cdef class MutableArcIterator(object):

  cdef shared_ptr[fst.MutableFstClass] _mfst
  cdef unique_ptr[fst.MutableArcIteratorClass] _aiter

  cpdef bool done(self)

  cpdef uint8 flags(self)

  cpdef void next(self)

  cpdef size_t position(self)

  cpdef void reset(self)

  cpdef void seek(self, size_t a)

  cpdef void set_flags(self, uint8 flags, uint8 mask)

  cpdef void set_value(self, Arc arc)

  cpdef object value(self)


cdef class StateIterator(object):

  cdef shared_ptr[fst.FstClass] _fst
  cdef unique_ptr[fst.StateIteratorClass] _siter

  cpdef bool done(self)

  cpdef void next(self)

  cpdef void reset(self)

  cpdef int64 value(self)


# Constructive operations on Fst.


cdef Fst _map(Fst ifst, float delta=?, map_type=?, double power=?, weight=?)

cpdef Fst arcmap(Fst ifst, float delta=?, map_type=?, double power=?, weight=?)

cpdef MutableFst compose(Fst ifst1,
                         Fst ifst2,
                         compose_filter=?,
                         bool connect=?)

cpdef Fst convert(Fst ifst, fst_type=?)

cpdef MutableFst determinize(Fst ifst,
                             float delta=?,
                             det_type=?,
                             int64 nstate=?,
                             int64 subsequential_label=?,
                             weight=?,
                             bool increment_subsequential_label=?)

cpdef MutableFst difference(Fst ifst1,
                            Fst ifst2,
                            compose_filter=?,
                            bool connect=?)

cpdef MutableFst disambiguate(Fst ifst,
                              float delta=?,
                              int64 nstate=?,
                              int64 subsequential_label=?,
                              weight=?)

cpdef MutableFst epsnormalize(Fst ifst, bool eps_norm_output=?)

cpdef bool equal(Fst ifst1, Fst ifst2, float delta=?)

cpdef bool equivalent(Fst ifst1, Fst ifst2, float delta=?) except *

cpdef MutableFst intersect(Fst ifst1,
                           Fst ifst2,
                           compose_filter=?,
                           bool connect=?)

cpdef bool isomorphic(Fst ifst1, Fst ifst2, float delta=?)

cpdef MutableFst prune(Fst ifst,
                       float delta=?,
                       int64 nstate=?,
                       weight=?)

cpdef MutableFst push(Fst ifst,
                      float delta=?,
                      bool push_weights=?,
                      bool push_labels=?,
                      bool remove_common_affix=?,
                      bool remove_total_weight=?,
                      bool to_final=?)

cpdef bool randequivalent(Fst ifst1,
                          Fst ifst2,
                          int32 npath=?,
                          float delta=?,
                          select=?,
                          int32 max_length=?,
                          uint64 seed=?) except *

cpdef MutableFst randgen(Fst ifst,
                         int32 npath=?,
                         select=?,
                         int32 max_length=?,
                         bool remove_total_weight=?,
                         bool weighted=?,
                         uint64 seed=?)

cpdef MutableFst replace(pairs,
                         call_arc_labeling=?,
                         return_arc_labeling=?,
                         bool epsilon_on_replace=?,
                         int64 return_label=?)

cpdef MutableFst reverse(Fst ifst, bool require_superinitial=?)

cdef vector[fst.WeightClass] *_shortestdistance(Fst ifst,
                                                float delta=?,
                                                int64 nstate=?,
                                                queue_type=?,
                                                bool reverse=?) except *

cpdef MutableFst shortestpath(Fst ifst,
                              float delta=?,
                              int32 nshortest=?,
                              int64 nstate=?,
                              queue_type=?,
                              bool unique=?,
                              weight=?)

cpdef Fst statemap(Fst ifst, map_type)

cpdef MutableFst synchronize(Fst ifst)


# Compiler.


cdef class Compiler(object):

  cdef unique_ptr[stringstream] _sstrm
  cdef string _fst_type
  cdef string _arc_type
  cdef const fst.SymbolTable *_isymbols
  cdef const fst.SymbolTable *_osymbols
  cdef const fst.SymbolTable *_ssymbols
  cdef bool _acceptor
  cdef bool _keep_isymbols
  cdef bool _keep_osymbols
  cdef bool _keep_state_numbering
  cdef bool _allow_negative_labels

  cpdef Fst compile(self)

  cpdef void write(self, expression)


# FarReader.

cdef class FarReader(object):

  cdef unique_ptr[fst.FarReaderClass] _reader

  cpdef string arc_type(self)

  cpdef bool done(self)

  cpdef bool error(self)

  cpdef string far_type(self)

  cpdef bool find(self, key)

  cpdef Fst get_fst(self)

  cpdef string get_key(self)

  cpdef void next(self)

  cpdef void reset(self)


# FarWriter.

cdef class FarWriter(object):

  cdef unique_ptr[fst.FarWriterClass] _writer

  cpdef string arc_type(self)

  cdef void close(self)

  cpdef void add(self, key, Fst ifst) except *

  cpdef bool error(self)

  cpdef string far_type(self)
