#include "process_message.h"

#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
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

    auto it = storage.find(user);
    if (it != storage.end()) {
        auto returnvalue = it->second;
        if (password == returnvalue.password) {
            std::stringstream result;
            for (const auto & mail : returnvalue.mailbox) {
                result << mail.sender << ": " << mail.body << '\n';
            }
            return result.str();
        }
    }

    storage[user] = {password, {}};
    std::cout << "user _" << user << "_ has a mailbox." << std::endl;
    return "no mail found";
}

std::string process_message(const std::string & packet) {

    static constexpr auto sender_length = 8;
    static constexpr auto recipient_length = 9;
    static constexpr auto pass_length = 10;
    std::string request = packet;
    std::string action = packet.substr(0, packet.find(" "));

    unsigned delimit_send = request.find("sender");
    unsigned second_delimit = request.find("\r\n", delimit_send);
    unsigned delimit_pass = request.find("password");
    unsigned fourth_delimit = request.find("\r\n", delimit_pass);
    unsigned body_delimit = request.find("\r\n\r\n");

    std::cout << "Request\n" << request << std::endl;

    std::string username = request.substr(delimit_send + sender_length,
                                          second_delimit - delimit_send - sender_length);
    std::string password
        = request.substr(delimit_pass + pass_length, fourth_delimit - (delimit_pass + pass_length));

    std::cout << "username: " << username << " | password: " << password << std::endl;
    for (char c : password) std::cout << std::hex << std::setw(2) << (int)c << ' ';
    std::cout << std::dec << std::endl;

    if (action == "GET") { return get(username, password); }

    std::string body = request.substr(body_delimit + 4);

    if (action == "POST") {
        unsigned delimiter1 = packet.find("message/");
        unsigned delimiter2 = packet.find(' ', delimiter1);
        std::string recipient = packet.substr(delimiter1 + sender_length,
                                              delimiter2 + 1 - (delimiter1 + recipient_length));
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
            std::cout << "sender _" << message.sender << "_\n";
            return "Error: Incorrect username/password.";
        }
        return "Message Posted";
    }

    if (action == "OPTION") {

        return "Available Options: \nPOST /message/username\n GET /mailbox\n";
    }

    return " ";
}
