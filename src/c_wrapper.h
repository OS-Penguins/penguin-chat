#ifndef C_WRAPPER_H
#define C_WRAPPER_H

#include <cstdlib>
#include <memory>

namespace utils {

// The default delete for C is the `free` function.
template <typename T> struct c_deleter {
    void operator()(T * ptr) { free(ptr); }
};

// A conveniance typedef for creating smart wrappers around C-created pointers.
template <typename T> using c_wrapper = std::unique_ptr<T, c_deleter<T>>;

// A conveniance function for creating smart wrappers around C-created pointers.
template <typename T> c_wrapper<T> wrap_c_ptr(T * ptr) { return c_wrapper<T>(ptr); }

} // namespace utils

#endif
