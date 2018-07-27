#include <iostream>
#include <csignal>
#include <memory.h>
#include "Server.h"


Network::TCP::Server server;
namespace
{
    volatile std::sig_atomic_t gSignalStatus;
}

void signal_handler(int signal)
{
    server.stop();
    exit(signal);
}

int main(int argc, char** argv)
{

    std::signal(SIGABRT, signal_handler);
    std::signal(SIGFPE, signal_handler);
    std::signal(SIGILL, signal_handler);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGSEGV, signal_handler);
    std::signal(SIGTERM, signal_handler);

    unsigned short port;
    if (argc == 1) {
        std::cout << "Port ? ";
        std::cin >> port;
    }
    else {
        std::cout << argv[1] << std::endl;
        port = std::stol(argv[1], 0, 10);
        //port = 23232;
    }


    if (!server.start(port))
    {
        return -2;
    }



    while(1)
    {
        server.update();
    }
}