#include <iostream>
#include <Client/Client.h>
#include <string>
#include <fstream>
#include <cstring>

int getFileSize(const std::string &fileName)
{
    std::ifstream file(fileName.c_str(), std::ifstream::in | std::ifstream::binary);

    if(!file.is_open())
    {
        return -1;
    }

    file.seekg(0, std::ios::end);
    int fileSize = file.tellg();
    file.close();

    return fileSize;
}

int main(int argc, char** argv)
{
    /*if (!Network::Start())
    {
        //std::cout << "Erreur initialisation WinSock : " << Network::Errors::Get();
        return -1;
    }*/

    auto pid = fork();
    if( pid < 0 )
        std::cout << "Une erreur est subvenue.\nVeuillez contacter votre administrateur réseau pour obtenir du support\n";
        // error out here!
    if( !pid && execl( "./Server", "./Server", "23232", NULL));

    sleep(1);

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
