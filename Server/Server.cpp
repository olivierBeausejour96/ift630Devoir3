
#include "Server.h"
#include <thread>
#include <map>
#include <list>
#include <cassert>
#include <Client/Client.h>
#include <cstring>
#include <iostream>

namespace Network
{
    namespace TCP
    {
        //https://bousk.developpez.com/cours/reseau-c++/TCP/05-envoi-reception-serveur/
        //https://fr.wikipedia.org/wiki/Signal_(informatique)
        class ServerImpl
        {
        public:
            ServerImpl() = default;
            ~ServerImpl();

            bool start(unsigned short _port);
            void stop();
            void update();
            bool sendTo(uint64_t clientid, const unsigned char* data, unsigned int len);
            bool sendToAll(const unsigned char* data, unsigned int len);
            std::unique_ptr<Messages::Base> poll();

        private:
            std::map<uint64_t, Client> mClients;
            std::list<std::unique_ptr<Messages::Base>> mMessages;

            SOCKET mSocket{ INVALID_SOCKET };
        };
        ServerImpl::~ServerImpl()
        {
            stop();
        }

        bool ServerImpl::start(unsigned short _port)
        {
            assert(mSocket == INVALID_SOCKET);
            if (mSocket != INVALID_SOCKET)
                stop();
            mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (mSocket == INVALID_SOCKET)
                return false;

            if (!SetReuseAddr(mSocket) )//|| !SetNonBlocking(mSocket))
            {
                stop();
                return false;
            }

            sockaddr_in addr;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(_port);
            addr.sin_family = AF_INET;
            if (bind(mSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
            {
                stop();
                return false;
            }
            if (listen(mSocket, SOMAXCONN) != 0)
            {
                stop();
                return false;
            }

            std::thread([this](){
                while(true) {
                    if (auto msg = this->poll()) {
                        if (msg->is<Network::Messages::Connection>()) {
                            std::cout << "Received Connection!\n";
                            //std::cout << "Connexion de [" << Network::GetAddress(msg->from) << ":" << Network::GetPort(msg->from) << "]" << std::endl;
                        } else if (msg->is<Network::Messages::Disconnection>()) {
                            std::cout << "Received Disconnection!\n";
                            //std::cout << "Deconnexion de [" << Network::GetAddress(msg->from) << ":" << Network::GetPort(msg->from) << "]" << std::endl;
                        } else if (msg->is<Network::Messages::UserData>()) {
                            auto userdata = msg->as<Network::Messages::UserData>();
                            std::cout << "Reveived Data!\n";
                            std::cout << "Data follows: " << userdata->data.size() << "\n";
                            const unsigned char *msgFromClient = userdata->data.data();
                            printf("%s\n",
                                   msgFromClient);//the \n is really important; without it, the string wont print
                            this->sendToAll(userdata->data.data(), static_cast<unsigned int>(userdata->data.size()));
                        } else {
                            std::cout << "Reveived Something!\n";
                        }
                    }
                }
            }).detach();
            return true;
        }
        void ServerImpl::stop()
        {
            for (auto& client : mClients)
                client.second.disconnect();
            mClients.clear();
            if (mSocket != INVALID_SOCKET)
                CloseSocket(mSocket);
            mSocket = INVALID_SOCKET;
        }



        void ServerImpl::update()
        {
            if (mSocket == INVALID_SOCKET)
                return;

                sockaddr_in addr = { 0 };
                socklen_t addrlen = sizeof(addr);
                SOCKET newClientSocket = accept(mSocket, reinterpret_cast<sockaddr*>(&addr), (socklen_t*)&addrlen);
                if (newClientSocket == INVALID_SOCKET)
                    printf("c pas correct sa \n");
                Client newClient;
                uint64_t id;
                if (newClient.init(std::move(newClientSocket), addr))
                {
                    auto message = std::make_unique<Messages::Connection>(Messages::Connection::Result::Success);
                    id = newClient.id();
                    message->idFrom = newClient.id();
                    message->from = newClient.destinationAddress();
                    mMessages.push_back(std::move(message));
                    mClients[newClient.id()] = std::move(newClient);
                }



            std::thread([this, id](){
                while(true){
                    Client &client = this->mClients[id];
                    auto msg = client.poll();
                    if (msg)
                    {
                        msg->from = client.destinationAddress();
                        msg->idFrom = client.id();
                        if (msg->is<Messages::Disconnection>())
                        {
                            //TODO: FAUT LE FAIRE SERIEUX LA
                            //newClient = mClients.erase(itClient);
                        }
                        mMessages.push_back(std::move(msg));
                    }
                }
            }).detach();
        }

        bool ServerImpl::sendTo(uint64_t clientid, const unsigned char* data, unsigned int len)
        {
            auto itClient = mClients.find(clientid);
            return itClient != mClients.end() && itClient->second.send(data, len);
        }

        bool ServerImpl::sendToAll(const unsigned char* data, unsigned int len)
        {
            bool ret = true;
            for (auto& client : mClients)
                ret &= client.second.send(data, len);
            return ret;
        }

        std::unique_ptr<Messages::Base> ServerImpl::poll()
        {
            if (mMessages.empty())
                return nullptr;

            auto msg = std::move(mMessages.front());
            mMessages.pop_front();
            return msg;
        }
        ////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        Server::Server() {}
        Server::~Server() {}
        Server::Server(Server&& other)
                : mImpl(std::move(other.mImpl))
        {}
        Server& Server::operator=(Server&& other)
        {
            mImpl = std::move(other.mImpl);
            return *this;
        }
        bool Server::start(unsigned short _port)
        {
            if (!mImpl)
                mImpl = std::make_unique<ServerImpl>();
            return mImpl && mImpl->start(_port);
        }
        void Server::stop() { if (mImpl) mImpl->stop(); }
        void Server::update() { if (mImpl) mImpl->update(); }
        bool Server::sendTo(uint64_t clientid, const unsigned char* data, unsigned int len) { return mImpl && mImpl->sendTo(clientid, data, len); }
        bool Server::sendToAll(const unsigned char* data, unsigned int len) { return mImpl && mImpl->sendToAll(data, len); }
        std::unique_ptr<Messages::Base> Server::poll() { return mImpl ? mImpl->poll() : nullptr; }
    }
}
