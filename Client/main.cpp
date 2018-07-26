#include <iostream>
#include <Client/Client.h>
#include <string>
#include <cstring>

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

    Network::TCP::Client clientTCP;


    if (clientTCP.connect("127.0.0.1", port));
    {
        char messsage[] = "LOLIPOP";
        char* msg = messsage;

        bool isSend = clientTCP.send((const unsigned char*)msg, strlen(msg));
        while (true) {
            while (auto msg = clientTCP.poll()) {
                if (msg->is<Network::Messages::Connection>()) {
                    std::cout << "Received Connection!\n";
                    //std::cout << "Connexion de [" << Network::GetAddress(msg->from) << ":" << Network::GetPort(msg->from) << "]" << std::endl;
                } else if (msg->is<Network::Messages::Disconnection>()) {
                    std::cout << "Received Disconnection!\n";
                    //std::cout << "Deconnexion de [" << Network::GetAddress(msg->from) << ":" << Network::GetPort(msg->from) << "]" << std::endl;
                } else if (msg->is<Network::Messages::UserData>()) {
                    auto userdata = msg->as<Network::Messages::UserData>();
                    std::cout << "Reveived Data from server!\n";
                    const unsigned char* msgFromServer = userdata->data.data();
                    printf("%s\n", msgFromServer);
                    clientTCP.disconnect();
                    //server.sendToAll(userdata->data.data(), static_cast<unsigned int>(userdata->data.size()));
                } else {
                    std::cout << "Reveived Something!\n";
                }
            }
        }
    }
}