
#include "Server.h"
#include <thread>
#include <map>
#include <list>
#include <cassert>
#include <Client/Client.h>
#include <cstring>
#include <fstream>
#include <csignal>

namespace Network
{
    namespace TCP
    {
        //https://bousk.developpez.com/cours/reseau-c++/TCP/05-envoi-reception-serveur/
        //https://fr.wikipedia.org/wiki/Signal_(informatique)
        //https://forgetcode.com/c/1201-file-transfer-using-tcp
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
                        if (msg->is<Network::Messages::Connection>())
                        {
                            std::cout << "Received Connection!\n";
                            //std::cout << "Connexion de [" << Network::GetAddress(msg->from) << ":" << Network::GetPort(msg->from) << "]" << std::endl;
                        } else if (msg->is<Network::Messages::Disconnection>())
                        {
                            std::cout << "Received Disconnection!\n";
                            //std::cout << "Deconnexion de [" << Network::GetAddress(msg->from) << ":" << Network::GetPort(msg->from) << "]" << std::endl;
                        } else if (msg->is<Network::Messages::UserData>())
                        {
                            static std::map<uint64_t , std::map<int, std::vector<char>>> fileTransferData;



                            auto userdata = msg->as<Network::Messages::UserData>();
                            if (fileTransferData.find(userdata->idFrom) == fileTransferData.end())
                            {
                                //new client
                                fileTransferData.insert(std::make_pair(userdata->idFrom, std::map<int, std::vector<char>>()));
                            }
                            std::cout << "Reveived Data!\n";
                            std::string qwe;
                            int ind = 0;
                            char m[6];
                            memset(m, 0 , 6);
                            memcpy(m, userdata->data.data() + ind++, 1);
                            //printf("isFile: %d \n", m[0]);

                            bool l;
                            memset(m, 0 , 6);
                            memcpy(&l, userdata->data.data() + ind++, 1);
                            //printf("morePacket: %s \n", l ? "yes" : "no");

                            int q = 0;
                            memset(m, 0 , 6);
                            memcpy(&q, userdata->data.data() + ind++, 4);
                            //printf("fileId: %d \n", q);

                            if (fileTransferData[userdata->idFrom].find(q) == fileTransferData[userdata->idFrom].end())
                            {
                                //new file from client x
                                fileTransferData[userdata->idFrom].insert(std::make_pair(q, std::vector<char>()));
                            }

                            auto it = userdata->data.begin() + 6;
                            while (it != userdata->data.end())
                            {
                                fileTransferData[userdata->idFrom][q].emplace_back(std::move(*it));
                                it++;
                            }

                            if (!l)
                            {
                                char outputPath[256];
                                memset(outputPath, 0, 256);
                                sprintf(outputPath, "./testu%dfid%d.test", userdata->idFrom, q);
                                //data to file
                                std::ofstream output(outputPath, std::ios::binary);
                                output.write(fileTransferData[userdata->idFrom][q].data(), fileTransferData[userdata->idFrom][q].size());
                            }

                            //printf("%s\n",
                            //userdata->data.data()+6);//the \n is really important; without it, the string wont print
                            //this->sendToAll(userdata->data.data(), static_cast<unsigned int>(userdata->data.size()));
                        } else
                        {
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
                            mClients[id].disconnect();
                            mClients.erase(id);
                            mMessages.push_back(std::move(msg));
                            break;
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
        //////////////////////////Server////////////////////////////////////////////////////
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
