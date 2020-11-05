
#include "https_utils.h"
#include "ssl_wrappers.h"
#include <openssl/err.h>

#include <csignal>
#include <cstdio>
#include <thread>
#include <unistd.h> //close
#include <vector>

using utils::wrap_c_ptr;

namespace {

// Initializes the SSL context.
utils::ssl_ctx_ptr init_ssl_context() {

    if constexpr (OPENSSL_VERSION_NUMBER < 0x10100000L) {
        SSL_library_init();
        SSL_load_error_strings();
        return wrap_c_ptr(SSL_CTX_new(SSLv23_method()));
    } else {
        utils::ssl_ctx_ptr ctx{SSL_CTX_new(TLS_method())};
        SSL_CTX_set_min_proto_version(ctx.get(), TLS1_2_VERSION);
        return ctx;
    }
}

utils::bio_ptr accept_new_tcp_connection(BIO * accept_bio) {
    return wrap_c_ptr(BIO_do_accept(accept_bio) <= 0 ? nullptr : BIO_pop(accept_bio));
}

} // namespace

static constexpr const auto * port = "8080";

int main() {
    auto ctx = init_ssl_context();

    // Load the certificate and key for SSL.
    // If one fails, print error messages and exit.
    if (SSL_CTX_use_certificate_file(ctx.get(), "localhost.crt", SSL_FILETYPE_PEM) <= 0) {
        fputs("Error loading server certificate", stderr);
        ERR_print_errors_fp(stderr);
        return -1;
    } else if (SSL_CTX_use_PrivateKey_file(ctx.get(), "localhost.key", SSL_FILETYPE_PEM) <= 0) {
        fputs("Error loading server private key", stderr);
        ERR_print_errors_fp(stderr);
        return -1;
    }

    // Setup the Basic Input/Output for SSL.
    auto accept_bio = wrap_c_ptr(BIO_new_accept(port));
    if (BIO_do_accept(accept_bio.get()) <= 0) {
        fprintf(stderr, "Error in BIO_do_accept (binding to port %s)\n", port);
        ERR_print_errors_fp(stderr);
        return -1;
    } else
        printf("listening on port %s\n", port);

    std::vector<std::thread> current_requests{};

    // Read in a connection.
    auto bio = accept_new_tcp_connection(accept_bio.get());
    while (bio != nullptr) {
        bio = std::move(bio) | wrap_c_ptr(BIO_new_ssl(ctx.get(), 0));
        // Read incoming message.
        // NOTE: This is still synchronous.
        const auto request = utils::receive_http_message(bio.get());
        // Process request
        current_requests.emplace_back([&bio, message = std::move(request)] {
            send_http_response(bio.get(), process_message(message));
        });
    }

    // Clean up all requests.
    for (auto & request : current_requests) request.join();

    puts("\nClean exit.");
}
