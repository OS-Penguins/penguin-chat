#include "process_message.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

struct message_t {
    std::string sender;
    std::string body;

    message_t(std::string send, std::string bod) {
        sender = send;
        body = bod;
    }
};

struct user_data_t {
    std::string password;
    std::vector<message_t> mailbox;
};

static std::map<std::string, user_data_t> storage;

std::string get(const std::string & user, const std::string & password) {

    user_data_t returnvalue;

    auto it = storage.find(user);
    if (it != storage.end()) {
        returnvalue = it->second;
        if (password == returnvalue.password) {
            for (int i = 0; i < returnvalue.mailbox.size(); ++i) {
                std::cout << returnvalue.mailbox[i].sender << ": " << returnvalue.mailbox[i].body;
            }
            return "End of message.";
        }
    }

    return "badinput";
}

std::string process_message(const std::string & packet) {

    static constexpr auto sender_length = 8;
    static constexpr auto recipient_length = 9;
    static constexpr auto pass_length = 10;
    std::string request = packet;
    std::string action = packet.substr(0, packet.find(" "));

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

    if (action == "GET") { get(username, password); }

    if (action == "POST") {
        unsigned delimiter1 = packet.find("message/");
        unsigned delimiter2 = packet.find("/r/n");
        std::string recipient = packet.substr(delimiter1 + sender_length,
                                              (delimiter2 - delimiter1) - recipient_length);
        message_t message(username, body);

        if (username.empty() && password.empty()) { message.sender = "Anonymous"; }

        if (username.empty() ^ password.empty()) { return "Message not posted"; }

        auto it = storage.find(message.sender);
        if (it != storage.end() && password == it->second.password) {
            auto it = storage.find(recipient);
            if (it != storage.end()) {
                it->second.mailbox.push_back(message);
            } else {
                return "Error: Recipient does not exist.";
            }
        } else {
            return "Error: Incorrect username/password.";
        }
        return "Message Posted";
    }

    if (action == "OPTION") {

        return "Available Options: \nPOST /message/username\n GET /mailbox\n";
    }

    return " ";
}
