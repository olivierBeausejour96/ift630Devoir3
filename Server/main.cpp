#include <iostream>
#include "Server.h"

int main(int argc, char** argv)
{
    /*if (!Network::Start())
    {
        //std::cout << "Erreur initialisation WinSock : " << Network::Errors::Get();
        return -1;
    }*/

    unsigned short port;
    if (argc == 1) {
        std::cout << "Port ? ";
        std::cin >> port;
    }
    else {
        port = std::stol(argv[1], 0, 10);
    }



    Network::TCP::Server server;
    if (!server.start(port))
    {
        //std::cout << "Erreur initialisation serveur : " << Network::Errors::Get();
        return -2;
    }



    while(1)
    {
        server.update();
    }
}