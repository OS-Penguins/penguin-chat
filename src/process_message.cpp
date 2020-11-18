#include "process_message.h"

#include <iostream>
#include <string>
#include <vector>

struct user_data_t {
    std::string password;
    std::vector<message_t> mailbox;
};

std::string process_message(const std::string packet &) {

    static constexpr auto sender_length = 8;
    static constexpr auto pass_length = 10;
    std::string request = packet;

    unsigned delimit_send = request.find("sender");
    unsigned second_delimit = request.find_first_of("\r\n", delimit_send);
    unsigned delimit_pass = request.find("password");
    unsigned fourth_delimit = request.find_first_of("\r\n", delimit_pass);

    std::string username = request.substr(delimit_send + sender_length,
                                          second_delimit - delimit_send - sender_length);
    std::string password
        = request.substr(delimit_pass + pass_length, fourth_delimit - delimit_pass - pass_length);

    std::cout << username << '\n' << password << std::endl;

    return '';
}
