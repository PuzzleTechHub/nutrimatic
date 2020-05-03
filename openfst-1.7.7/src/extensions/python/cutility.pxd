# See www.openfst.org for extensive documentation on this weighted
# finite-state transducer library.


#TODO(kbg): When/if PR https://github.com/cython/cython/pull/3358 is merged
# and we update third-party Cython up to or beyond a version that includes
# this, delete this file and instead use libcpp.utility.move.


cdef extern from * namespace "fst":
    """
    #include <type_traits>
    #include <utility>

    namespace fst {

    template <typename T>
    inline typename std::remove_reference<T>::type &&move(T &t) {
        return std::move(t);
    }

    template <typename T>
    inline typename std::remove_reference<T>::type &&move(T &&t) {
        return std::move(t);
    }

    }  // namespace fst
    """
    cdef T move[T](T)
