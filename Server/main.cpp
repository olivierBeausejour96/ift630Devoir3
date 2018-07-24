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
        while (auto msg = server.poll())
        {
            if (msg->is<Network::Messages::Connection>())
            {
                std::cout << "Received Connection!\n";
                //std::cout << "Connexion de [" << Network::GetAddress(msg->from) << ":" << Network::GetPort(msg->from) << "]" << std::endl;
            }
            else if (msg->is<Network::Messages::Disconnection>())
            {
                std::cout << "Received Disconnection!\n";
                //std::cout << "Deconnexion de [" << Network::GetAddress(msg->from) << ":" << Network::GetPort(msg->from) << "]" << std::endl;
            }
            else if (msg->is<Network::Messages::UserData>())
            {
                auto userdata = msg->as<Network::Messages::UserData>();
                std::cout << "Reveived Data!\n";
                std::cout << "Data follows: " << userdata->data.size() << "\n";
                const unsigned char* qwe = userdata->data.data();
                printf("%s\n", qwe);//the \n is really important; without it, the string wont print
                //server.sendToAll(userdata->data.data(), static_cast<unsigned int>(userdata->data.size()));
            }
            else
            {
                std::cout << "Reveived Something!\n";
            }
        }
    }
}