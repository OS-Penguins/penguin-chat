#include "https_utils.h"

#include "ssl_wrappers.h"
#include <openssl/err.h>

#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace utils {

using std::string, std::pair;

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

// `true` if connection ended
pair<string, bool> receive_some_data(BIO * bio) {
    char buffer[1024];
    static std::mutex mut{};
    while (true) {
        std::lock_guard lock{mut};
        const auto len = BIO_read(bio, buffer, sizeof(buffer));
        std::cout << len << std::endl;
        if (len == -2) print_errors_and_throw("error in BIO_read");
        else if (len > 0)
            return {string(buffer, len), false};
        else if (BIO_should_retry(bio))
            continue;
        else if (len == 0)
            return {"", true};
        else if (BIO_should_write(bio))
            print_errors_and_throw("should write");
        else
            print_errors_and_throw("empty BIO_read");
    }
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
    size_t end_of_thingy = 0;
    try {
        do {
            auto [data, ended] = receive_some_data(bio);
            if (ended) return "";
            headers += data;
            std::cout << "end of headers at: ";
            end_of_thingy = headers.find(http_section_separator);
            std::cout << end_of_thingy << std::endl;
            std::cout << "is endpos: " << std::boolalpha << (end_of_thingy == std::string::npos)
                      << std::endl;
        } while (end_of_thingy == std::string::npos and not headers.empty());
    } catch (std::runtime_error & e) {
        std::cout << "Error occured: " << e.what() << std::endl;
        return "";
    }

    std::cout << "headers: " << headers << std::endl;
    // Take all after the section separator and make that the body.
    auto body = headers.substr(end_of_thingy);

    // Remove the data after the section separator, keeping the first EOL cluster.
    headers.resize(end_of_thingy);

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
    std::cout << "content length: " << content_length << std::endl;
    std::cout << "body size: " << body.size() << std::endl;
    while (body.size() < content_length) body += std::get<0>(receive_some_data(bio));

    return headers + eol_http + body;
}

void send_http_response(BIO * bio, const std::string & body) {

    static std::mutex mut{};
    std::lock_guard lock{mut};

    {
        std::stringstream response_builder{};
        response_builder << "HTTP/1.1 200 OK\r\n";
        response_builder << "Content-Type: text/plain\r\n";
        response_builder << "Content-Length: " << body.size() << http_section_separator;
        const auto response = response_builder.str();
        std::cout << response;
        BIO_puts(bio, response.c_str());
    }

    std::cout << body << std::endl;

    // BIO_write(bio, body.data(), body.size());
    if (BIO_puts(bio, body.c_str()) == -2) std::cout << "unsupported puts" << std::endl;
    if (BIO_flush(bio) <= 0) std::cout << "did not flush" << std::endl;
}
} // namespace utils
