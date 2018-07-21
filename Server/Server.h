//
// Created by beao3002 on 18-07-21.
//

#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H


#include <bits/unique_ptr.h>

class Server
{
public:
    Server();
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&);
    Server& operator=(Server&&);
    ~Server();

    bool start(unsigned short _port);
    void stop();
    void update();
    std::unique_ptr<Messages::Base> poll();

private:
    std::unique_ptr<Server> mImpl;
};


#endif //SERVER_SERVER_H
