
#include "https_utils.h"
#include "process_message.h"
#include "ssl_wrappers.h"
#include <openssl/err.h>

#include <cstdio>
#include <iostream>
#include <queue>
#include <thread>

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

static pthread_mutex_t * lockarray;

void lock_callback(int mode, int type, char * file, int line) {
    (void)file;
    (void)line;
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&(lockarray[type]));
    } else {
        pthread_mutex_unlock(&(lockarray[type]));
    }
}

static void init_locks(void) {
    int i;

    lockarray = (pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
    for (i = 0; i < CRYPTO_num_locks(); i++) { pthread_mutex_init(&(lockarray[i]), NULL); }

    CRYPTO_set_id_callback((unsigned long (*)())thread_id);
    CRYPTO_set_locking_callback(lock_callback);
}

static void kill_locks(void) {
    int i;

    CRYPTO_set_locking_callback(NULL);
    for (i = 0; i < CRYPTO_num_locks(); i++) pthread_mutex_destroy(&(lockarray[i]));

    OPENSSL_free(lockarray);
}

static constexpr auto num_simultaneous_connections = 2;

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

    init_locks();

    std::queue<std::thread> current_requests{};

    while (true) {
        while (current_requests.size() < num_simultaneous_connections) {
            current_requests.emplace([&accept_bio, &ctx] {
                // Read in a connection.
                // Read incoming message.
                // NOTE: This is still synchronous.

                auto bio = accept_new_tcp_connection(accept_bio.get());
                while (bio != nullptr) {
                    bio = std::move(bio) | wrap_c_ptr(BIO_new_ssl(ctx.get(), 0));
                    // Process request
                    std::cout << "reading new message" << std::endl;
                    const auto message = utils::receive_http_message(bio.get());
                    if (message.empty()) return;
                    std::cout << "processing message" << std::endl;
                    const auto responce = process_message(message);
                    std::cout << "Responding with " << responce << std::endl;
                    utils::send_http_response(bio.get(), responce);
                }
            });
        }

        try {
            while (current_requests.front().joinable()) {
                current_requests.front().join();
                current_requests.pop();
            }
        } catch (...) {}

        std::this_thread::yield();
    }

    kill_locks();

    puts("\nClean exit.");
}
