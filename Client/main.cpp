#include <iostream>
#include <Client/Client.h>
#include <string>
#include <fstream>
#include <cstring>
#include <csignal>


Network::TCP::Client clientTCP;

namespace
{
    volatile std::sig_atomic_t gSignalStatus;
}

void signal_handler(int signal)
{
    clientTCP.disconnect();
    std::cout << strsignal(signal) << std::endl;
    exit(signal);
}

int main(int argc, char** argv)
{
    std::signal(SIGTERM, signal_handler);

    auto pid = fork();
    if( pid < 0 )
        std::cout << "Une erreur est subvenue.\nVeuillez contacter votre administrateur réseau pour obtenir du support\n";
        // error out here!
    if( !pid && execl( "./Server", "./Server", argv[1], NULL));

    sleep(1);

    pid = fork();

    unsigned short port;
    if (argc == 1) {
        std::cout << "Port ? ";
        std::cin >> port;
    }
    else {
        port = std::stol(argv[1], 0, 10);
    }


    if (clientTCP.connect("127.0.0.1", port));
    {
        char* pathFile = "../tp3.zip";

        //char message[] = "LOLIPOP";
        //char *msg = message;

        bool isSend = clientTCP.sendFile(pathFile);
        //bool isSend = clientTCP.send((const unsigned char*)msg, strlen(msg));
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
