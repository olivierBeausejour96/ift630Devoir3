#include <iostream>
#include <Client/Client.h>
#include <string>
#include <cstring>

int main() {

    Network::TCP::Client qwe;


    if (qwe.connect("127.0.0.1", 23232));
    {
        char asd[] = "caliss";
        char* jkl = asd;

        bool zxc = qwe.send((const unsigned char*)jkl, strlen(jkl));
        while (true) {
            while (auto msg = qwe.poll()) {
                if (msg->is<Network::Messages::Connection>()) {
                    std::cout << "Received Connection!\n";
                    //std::cout << "Connexion de [" << Network::GetAddress(msg->from) << ":" << Network::GetPort(msg->from) << "]" << std::endl;
                } else if (msg->is<Network::Messages::Disconnection>()) {
                    std::cout << "Received Disconnection!\n";
                    //std::cout << "Deconnexion de [" << Network::GetAddress(msg->from) << ":" << Network::GetPort(msg->from) << "]" << std::endl;
                } else if (msg->is<Network::Messages::UserData>()) {
                    auto userdata = msg->as<Network::Messages::UserData>();
                    std::cout << "Reveived Data!\n";
                    //server.sendToAll(userdata->data.data(), static_cast<unsigned int>(userdata->data.size()));
                } else {
                    std::cout << "Reveived Something!\n";
                }
            }
        }
    }
    std::cout << "ERPPPRROROEROERO\n";
}