#include "https_utils.h"

#include "ssl_wrappers.h"
#include <openssl/err.h>

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace utils {

using std::string;

namespace {

[[noreturn]] void print_errors_and_throw(const char * message) {

    class StringBIO final {
        std::string str_;
        bio_method_ptr methods_;
        bio_ptr bio_;

      public:
        explicit StringBIO() : methods_(BIO_meth_new(BIO_TYPE_SOURCE_SINK, "StringBIO")) {
            if (methods_ == nullptr) throw std::runtime_error("StringBIO: error in BIO_meth_new");

            BIO_meth_set_write(methods_.get(), [](BIO * bio, const char * data, int len) {
                auto * str = reinterpret_cast<std::string *>(BIO_get_data(bio));
                str->append(data, len);
                return len;
            });

            bio_.reset(BIO_new(methods_.get()));
            if (bio_ == nullptr) throw std::runtime_error("StringBIO: error in BIO_new");
            BIO_set_data(bio_.get(), &str_);
            BIO_set_init(bio_.get(), 1);
        }

        StringBIO(const StringBIO &) = delete;
        StringBIO & operator=(const StringBIO &) = delete;

        StringBIO(StringBIO &&) noexcept = delete;
        StringBIO & operator=(StringBIO &&) noexcept = delete;

        ~StringBIO() noexcept = default;

        BIO * bio() noexcept { return bio_.get(); }
        std::string str() && noexcept { return std::move(str_); }
    } bio;

    ERR_print_errors(bio.bio());
    throw std::runtime_error{std::string(message) + '\n' + std::move(bio).str()};
}

string receive_some_data(BIO * bio) {
    char buffer[1024];
    if (const auto len = BIO_read(bio, buffer, sizeof(buffer)); len < 0)
        print_errors_and_throw("error in BIO_read");
    else if (len > 0)
        return string(buffer, len);
    else if (BIO_should_retry(bio))
        return receive_some_data(bio);
    else
        print_errors_and_throw("empty BIO_read");
}

static constexpr const auto * eol_http = "\r\n";

std::vector<std::string> split_headers(const std::string & text) {
    std::vector<std::string> lines;
    const auto * start = text.c_str();
    const auto * end = strstr(start, eol_http);
    while (end != nullptr) {
        lines.emplace_back(start, end);
        start = end + 2;
        end = strstr(start, eol_http);
    }
    return lines;
}

static constexpr const auto * http_section_separator = "\r\n\r\n";

} // namespace

string receive_http_message(BIO * bio) {

    // Collect all headers at the top of an HTTP message.
    string headers{};
    char * end_of_headers = nullptr;
    do {
        headers += receive_some_data(bio);
        end_of_headers = strstr(headers.data(), http_section_separator);
    } while (end_of_headers == nullptr);

    // Take all after the section separator and make that the body.
    auto body
        = std::string(end_of_headers + strlen(http_section_separator), &headers[headers.size()]);

    // Remove the data after the section separator, keeping the first EOL cluster.
    headers.resize(end_of_headers + strlen(eol_http) - headers.c_str());

    // Determine the length of the body.
    // This is done by reading the 'Content-Length' header attribute.
    // TO FIX: If multiple such headers appear,
    //         RFC 7230 Section 3.3.2 demands that the message be marked invalid.
    //         This is currently not done.
    size_t content_length = 0;
    for (const auto & line : split_headers(headers)) {
        if (const auto * colon = strchr(line.c_str(), ':'); colon != nullptr)
            if (const auto header_name = std::string(line.c_str(), colon);
                header_name == "Content-Length")
                content_length = std::stoul(colon + 1);
    }

    while (body.size() < content_length) body += receive_some_data(bio);

    return headers + eol_http + body;
}

void send_http_response(BIO * bio, const std::string & body) {
    {
        std::stringstream response_builder{"HTTP/1.1 200 OK\r\n"};
        response_builder << "Content-Length: " << body.size() << http_section_separator;
        const auto response = response_builder.str();
        BIO_write(bio, response.data(), response.size());
    }

    BIO_write(bio, body.data(), body.size());
    BIO_flush(bio);
}
} // namespace utils
