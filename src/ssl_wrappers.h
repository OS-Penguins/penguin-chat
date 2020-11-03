#ifndef SSL_WRAPPERS_H
#define SSL_WRAPPERS_H

#include <openssl/bio.h>
#include <openssl/ssl.h>

#include "c_wrapper.h"

namespace utils {
// Typical C style is to have special delete functions for some important types.
template <> struct c_deleter<SSL_CTX> {
    void operator()(SSL_CTX * ptr) { SSL_CTX_free(ptr); }
};
template <> struct c_deleter<BIO> {
    void operator()(BIO * ptr) { BIO_free_all(ptr); }
};
template <> struct c_deleter<BIO_METHOD> {
    void operator()(BIO_METHOD * ptr) { BIO_meth_free(ptr); }
};

// A few more wrappers, here specifically for SSL.
using ssl_ctx_ptr = c_wrapper<SSL_CTX>;
using bio_ptr = c_wrapper<BIO>;
using bio_method_ptr = c_wrapper<BIO_METHOD>;
} // namespace utils

utils::bio_ptr operator|(utils::bio_ptr lower, utils::bio_ptr upper) {
    BIO_push(upper.get(), lower.release());
    return upper;
}

#endif
