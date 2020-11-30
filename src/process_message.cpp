#include "process_message.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

struct message_t {
    std::string sender;
    std::string body;
};

struct user_data_t {
    std::string password;
    std::vector<message_t> mailbox;
};

static std::map<std::string, user_data_t> storage;

std::string process_message(const std::string & packet) {

    static constexpr auto sender_length = 8;
    static constexpr auto pass_length = 10;
    std::string request = packet;

    unsigned delimit_send = request.find("sender");
    unsigned second_delimit = request.find_first_of("\r\n", delimit_send);
    unsigned delimit_pass = request.find("password");
    unsigned fourth_delimit = request.find_first_of("\r\n", delimit_pass);
    unsigned body_delimit = request.find("\r\n\r\n");

    std::string username = request.substr(delimit_send + sender_length,
                                          second_delimit - delimit_send - sender_length);
    std::string password
        = request.substr(delimit_pass + pass_length, fourth_delimit - delimit_pass - pass_length);

    std::string body = request.substr(body_delimit);

    message_t message;
    message.sender = username;
    message.body = body;
    user_data_t user;
    user.password = password;
    user.mailbox.push_back(message);

    auto it = storage.find(username);
    if (it != storage.end()) {
        it->second = user;
    } else
        storage.insert({username, user});

    return message.body;
}

std::string get(const std::string & user, const std::string & password) {

    user_data_t returnvalue;

    std::map<std::string, user_data_t>::iterator it = storage.find(user);
    if (it != storage.end()) {
        returnvalue = storage.at(user);
        if (password == returnvalue.password) {
            for (int i = 0; i < returnvalue.mailbox.size(); ++i) {
                std::cout << returnvalue.mailbox[i].sender << ": " << returnvalue.mailbox[i].body;
            }
            return "End of message.";
        }
    }

    return "badinput";
}
