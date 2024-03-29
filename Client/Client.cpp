   #include <vector>
#include <list>
#include <cassert>
#include <numeric>
#include <bits/unique_ptr.h>
#include <netinet/in.h>
#include <poll.h>
#include <arpa/inet.h>
#include <system_error>
#include <cstring>
#include <limits>
#include <sys/socket.h>
#include "Client.h"
#include <fstream>

namespace Network
{
    namespace TCP
    {
        class ConnectionHandler
        {
            public:
                ConnectionHandler() = default;
                bool connect(SOCKET &&sckt, const std::string& address, unsigned short port);
                std::unique_ptr<Messages::Connection> poll();
                const sockaddr_in& connectedAddress() const { return mConnectedAddress; }

            private:
                pollfd mFd{ 0 };
                struct sockaddr_in mConnectedAddress {0};
                std::string mAddress;
                unsigned short mPort;
        };

        bool ConnectionHandler::connect(SOCKET &&sckt, const std::string& address, unsigned short port)
        {
            assert(sckt == INVALID_SOCKET);
            mAddress = address;
            mPort = port;
            mFd.events = POLLOUT;
            memset(&mConnectedAddress, '0', sizeof(mConnectedAddress));
            mConnectedAddress.sin_family = AF_INET;
            mConnectedAddress.sin_port = htons(mPort);

            int sock;
            struct sockaddr_in serv_addr;

            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                printf("\n Socket creation error \n");
                return false;
            }

            memset(&serv_addr, '0', sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(mPort);

            if(inet_pton(AF_INET, address.c_str(), &mConnectedAddress.sin_addr)<=0)
            {
                printf("\nInvalid address/ Address not supported \n");
                return false;
            }
            sckt = sock;
            mFd.fd = sckt;
            if (::connect(sock, (struct sockaddr *)&mConnectedAddress, sizeof(mConnectedAddress)) != 0)
            {
                printf("\nConnection Failed \n");
                return false;
            }

            return true;
        }

        std::unique_ptr<Messages::Connection> ConnectionHandler::poll()
        {
            int res = ::poll(&mFd, 1, 0);

            if (res < 0)
                return std::make_unique<Messages::Connection>(Messages::Connection::Result::Failed);
            else if (res > 0)
            {
                if (mFd.revents & POLLOUT)
                    return std::make_unique<Messages::Connection>(Messages::Connection::Result::Success);
                else if (mFd.revents & (POLLHUP | POLLNVAL))
                    return std::make_unique<Messages::Connection>(Messages::Connection::Result::Failed);
                else if(mFd.revents & POLLERR)
                    return std::make_unique<Messages::Connection>(Messages::Connection::Result::Failed);
            }
            return nullptr;
        }

        ////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        //////////////////////RECEPTION HANDLER/////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        class ReceptionHandler
        {
            enum class State {
                Header,
                Data,
            };
            public:
                ReceptionHandler() = default;
                void init(SOCKET sckt);
                std::unique_ptr<Messages::Base> recv();

            private:
                inline char* missingDataStartBuffer() { return reinterpret_cast<char*>(mBuffer.data() + mReceived); }
                inline int missingDataLength() const { return static_cast<int>(mBuffer.size() - mReceived); }
                void startHeaderReception();
                void startDataReception();
                void startReception(unsigned int expectedDataLength, State newState);

            private:
                std::vector<unsigned char> mBuffer;
                unsigned int mReceived;
                SOCKET mSckt{ INVALID_SOCKET };
                State mState;
        };

        void ReceptionHandler::init(SOCKET sckt)
        {
            assert(sckt != INVALID_SOCKET);
            mSckt = sckt;
            startHeaderReception();
        }

        void ReceptionHandler::startHeaderReception()
        {
            startReception(HeaderSize, State::Header);
        }

        void ReceptionHandler::startDataReception()
        {
            assert(mBuffer.size() == sizeof(HeaderType));
            HeaderType networkExpectedDataLength;
            memcpy(&networkExpectedDataLength, mBuffer.data(), sizeof(networkExpectedDataLength));
            const auto expectedDataLength = ntohs(networkExpectedDataLength);
            startReception(expectedDataLength, State::Data);
        }

        void ReceptionHandler::startReception(unsigned int expectedDataLength, State newState)
        {
            mReceived = 0;
            mBuffer.clear();
            mBuffer.resize(expectedDataLength, 0);
            mState = newState;
        }

        std::unique_ptr<Messages::Base> ReceptionHandler::recv()
        {
            assert(mSckt != INVALID_SOCKET);
            int ret = ::recv(mSckt, missingDataStartBuffer(), missingDataLength(), 0);

            if (ret > 0)
            {
                mReceived += ret;
                if (mReceived == mBuffer.size())
                {
                    if (mState == State::Data)
                    {
                        std::unique_ptr<Messages::Base> msg = std::make_unique<Messages::UserData>(std::move(mBuffer));
                        startHeaderReception();
                        return msg;
                    }
                    else
                    {
                        startDataReception();
                        //!< si jamais les données sont déjà disponibles elles seront ainsi retournées directement
                        return recv();
                    }
                }
                return nullptr;
            }

            else if (ret == 0)
            {
                return std::make_unique<Messages::Disconnection>(Messages::Disconnection::Reason::Disconnected);
            }
            else
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                    return nullptr;
                else
                    return std::make_unique<Messages::Disconnection>(Messages::Disconnection::Reason::Lost);
            }
        }


        ////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////Sending Handler///////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        class SendingHandler
        {
            enum class State {
                Idle,
                Header,
                Data,
            };
            public:
                SendingHandler() = default;
                void init(SOCKET sckt);
                bool send(const unsigned char* data, unsigned int len, bool isFile = false, bool morePacket = false, int fileId = 0);
                bool sendFile(std::string data);
                void update();
                size_t queueSize() const;

            private:
                bool sendPendingBuffer();
                void prepareNextHeader();
                void prepareNextData();

            private:
                std::list<std::vector<unsigned char>> mQueueingBuffers;
                std::vector<unsigned char> mSendingBuffer;
                SOCKET mSocket{ INVALID_SOCKET };
                State mState{ State::Idle } ;
        };

        void SendingHandler::init(SOCKET sckt)
        {
            mSocket = sckt;
            if (mState == State::Header || mState == State::Data)
                mSendingBuffer.clear();

            mState = State::Idle;
        }

        bool SendingHandler::send(const unsigned char* data, unsigned int datalen, bool isFile, bool morePacket, int fileId)
        {
            if (datalen > std::numeric_limits<HeaderType>::max())
                return false;
            char qwe[datalen + 6];
            memset(qwe, 0, datalen+6);
            memcpy(qwe, &isFile, 1);
            memcpy(qwe+1, &morePacket, 1);
            memcpy(qwe+2, &fileId, 4);
            memcpy(qwe+6, data, datalen);
            char * asd = qwe;
            mQueueingBuffers.emplace_back(asd, asd + datalen + 6);
            return true;
        }

        bool SendingHandler::sendFile(std::string path)
        {
            std::ifstream input(path, std::ios::binary );
            // copies all data into buffer
            std::vector<char> buffer((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));
            static int fgh = 0;
            int ind = 0;
            const int packetSize = 1024;
            while(ind <= buffer.size() - packetSize){
                send((const unsigned char*)(buffer.data() + ind), packetSize, 1, 1, fgh);
                ind += packetSize;
            }

            send((const unsigned char*)(buffer.data() + ind), buffer.size() - ind, 1, 0, fgh++);
            return true;
        }

        void SendingHandler::update()
        {
            assert(mSocket != INVALID_SOCKET);
            if (mState == State::Idle && !mQueueingBuffers.empty())
                prepareNextHeader();

            while (mState != State::Idle && sendPendingBuffer())
            {
                if (mState == State::Header)
                    prepareNextData();
                else
                {
                    if (!mQueueingBuffers.empty())
                        prepareNextHeader();
                    else
                        mState = State::Idle;
                }
            }
        }

        bool SendingHandler::sendPendingBuffer()
        {
            if (mSendingBuffer.empty())
                return true;

            int sent = ::send(mSocket, reinterpret_cast<char*>(mSendingBuffer.data()), static_cast<int>(mSendingBuffer.size()), 0);
            if (sent > 0)
            {
                if (sent == mSendingBuffer.size())
                {
                    //!< toutes les données ont été envoyées
                    mSendingBuffer.clear();
                    return true;
                }
                else
                {
                    //!< envoi partiel
                    memmove(mSendingBuffer.data() + sent, mSendingBuffer.data(), sent);
                    mSendingBuffer.erase(mSendingBuffer.cbegin() + sent, mSendingBuffer.cend());
                }
            }
            return false;
        }

        void SendingHandler::prepareNextHeader()
        {
            assert(!mQueueingBuffers.empty());
            const auto header = static_cast<HeaderType>(mQueueingBuffers.front().size());
            const auto networkHeader = htons(header);
            mSendingBuffer.clear();
            mSendingBuffer.resize(HeaderSize);
            memcpy(mSendingBuffer.data(), &networkHeader, sizeof(HeaderType));
            mState = State::Header;
        }

        void SendingHandler::prepareNextData()
        {
            assert(!mQueueingBuffers.empty());
            mSendingBuffer.swap(mQueueingBuffers.front());
            mQueueingBuffers.pop_front();
            mState = State::Data;
        }

        size_t SendingHandler::queueSize() const
        {
            size_t s = std::accumulate(mQueueingBuffers.cbegin(), mQueueingBuffers.cend(), static_cast<size_t>(0), [](size_t n, const std::vector<unsigned char>& queuedItem) {
                return n + queuedItem.size() + HeaderSize;
            });
            if (mState == State::Data)
                s += mSendingBuffer.size();
            return s;
        }

        ////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////ClientImpl////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        class ClientImpl
        {
            enum class State {
                Connecting,
                Connected,
                Disconnected,
            };

            public:
                ClientImpl() = default;
                ~ClientImpl();

                bool init(SOCKET&& sckt, const sockaddr_in& addr);
                bool connect(const std::string& ipaddress, unsigned short port);
                void disconnect();
                bool send(const unsigned char* data, unsigned int len);
                bool sendFile(std::string data);
                std::unique_ptr<Messages::Base> poll();

                uint64_t id() const { return static_cast<uint64_t>(mSocket); }
                const sockaddr_in& destinationAddress() const { return mAddress; }

            private:
                void onConnected(const sockaddr_in& addr);

            private:
                ConnectionHandler mConnectionHandler;
                SendingHandler mSendingHandler;
                ReceptionHandler mReceivingHandler;
                sockaddr_in mAddress{ 0 };
                SOCKET mSocket{ INVALID_SOCKET };
                State mState{ State::Disconnected };
        };

        ClientImpl::~ClientImpl()
        {
            disconnect();
        }

        bool ClientImpl::init(SOCKET&& sckt, const sockaddr_in& addr)
        {
            assert(sckt != INVALID_SOCKET);
            if (sckt == INVALID_SOCKET)
                return false;

            assert(mState == State::Disconnected);
            assert(mSocket == INVALID_SOCKET);
            if (mSocket != INVALID_SOCKET)
                disconnect();

            mSocket = sckt;
            if (!SetReuseAddr(mSocket)|| !SetNonBlocking(mSocket))
            {
                disconnect();
                return false;
            }
            onConnected(addr);
            return true;
        }

        bool ClientImpl::connect(const std::string& ipaddress, unsigned short port)
        {
            assert(mState == State::Disconnected);
            assert(mSocket == INVALID_SOCKET);
            if (mSocket != INVALID_SOCKET)
                disconnect();
            if (mConnectionHandler.connect(std::move(mSocket), ipaddress, port))
            {
                mState = State::Connecting;
                return true;
            }
            return false;
        }

        void ClientImpl::disconnect()
        {
            if (mSocket != INVALID_SOCKET)
                CloseSocket(mSocket);

            mSocket = INVALID_SOCKET;
            memset(&mAddress, 0, sizeof(mAddress));
            mState = State::Disconnected;
        }

        bool ClientImpl::send(const unsigned char* data, unsigned int len)
        {
            return mSendingHandler.send(data, len);
        }

        bool ClientImpl::sendFile(std::string data)
        {
            return mSendingHandler.sendFile(data);
        }

        std::unique_ptr<Messages::Base> ClientImpl::poll()
        {
            switch (mState)
            {
                case State::Connecting:
                {
                    auto msg = mConnectionHandler.poll();
                    if (msg)
                    {
                        if (msg->result == Messages::Connection::Result::Success)
                            onConnected(mConnectionHandler.connectedAddress());
                        else
                            disconnect();
                    }
                    return msg;
                } break;
                case State::Connected:
                {
                    mSendingHandler.update();
                    auto msg = mReceivingHandler.recv();
                    if (msg)
                    {
                        if (msg->is<Messages::Disconnection>())
                            disconnect();
                    }
                    return msg;
                } break;
                case State::Disconnected:
                     break;
            }
            return nullptr;
        }

        void ClientImpl::onConnected(const sockaddr_in& addr)
        {
            mAddress = addr;
            mSendingHandler.init(mSocket);
            mReceivingHandler.init(mSocket);
            mState = State::Connected;
        }

        ////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////Client///////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////
        Client::Client() {}
        Client::~Client() {}
        Client::Client(Client&& other)
        : mImpl(std::move(other.mImpl))
        {}

        Client& Client::operator=(Client&& other)
        {
            mImpl = std::move(other.mImpl);
            return *this;
        }

        bool Client::init(SOCKET&& sckt, const sockaddr_in& addr)
        {
            if (!mImpl)
                mImpl = std::make_unique<ClientImpl>();

            return mImpl && mImpl->init(std::move(sckt), addr);
        }

        bool Client::connect(const std::string& ipaddress, unsigned short port)
        {
            if (!mImpl)
                mImpl = std::make_unique<ClientImpl>();
            return mImpl && mImpl->connect(ipaddress, port);
        }
        void Client::disconnect() { if (mImpl) mImpl->disconnect(); }
        bool Client::send(const unsigned char* data, unsigned int len) { return mImpl && mImpl->send(data, len); }
        bool Client::sendFile(std::string data){ return mImpl && mImpl->sendFile(data); }
        std::unique_ptr<Messages::Base> Client::poll() { return mImpl ? mImpl->poll() : nullptr; }
        uint64_t Client::id() const { return mImpl ? mImpl->id() : 0xffffffffffffffff; }
        const sockaddr_in& Client::destinationAddress() const { static sockaddr_in empty{ 0 }; return mImpl ? mImpl->destinationAddress() : empty; }
    }
}
