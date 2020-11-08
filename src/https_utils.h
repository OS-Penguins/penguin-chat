
#ifndef HTTPS_UTILS_H
#define HTTPS_UTILS_H

#include <openssl/bio.h>

#include <string>

namespace utils {

std::string receive_http_message(BIO *);
void send_http_response(BIO*, const std::string &);
}
#endif
