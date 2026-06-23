#include "include/08chat/chat.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "include/08chat/mutiple_chat.h"
#include "include/08chat/signle.h"

static int getPort(int argc, char* argv[]) {
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "-p") {
            return std::stoi(argv[i + 1]);
        }
    }
    return 8080;
}

void RunChat(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "usage:\n";
        std::cout << "./chat server -p 8080\n";
        std::cout << "./chat client -p 8080\n";
        return;
    }

    std::string mode = argv[1];
    int port = getPort(argc, argv);
    auto chat = std::make_shared<SignleChat>();
    // auto chat = std::make_shared<MutipleChat>();

    if (mode == "server") {
        chat->server(port);
    } else if (mode == "client") {
        chat->client(port);
    } else {
        std::cout << "unknown mode: " << mode << "\n";
    }
}
