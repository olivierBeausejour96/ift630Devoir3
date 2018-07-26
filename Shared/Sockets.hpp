#ifndef SHARED_SOCKETS_HPP
#define SHARED_SOCKETS_HPP

#include <sys/socket.h>
#include <sys/types.h>
#include <zconf.h>
#include <fcntl.h>

namespace Network
{

#define SOCKET int
#define INVALID_SOCKET -1
#define BOOL char
#define TRUE 1
#include <exception>

    class FunctionUndefinedException: std::exception {};

    static bool SetNonBlocking(SOCKET socketfd) throw()
    {
        //https://stackoverflow.com/questions/1543466/how-do-i-change-a-tcp-socket-to-be-non-blocking
        // where socketfd is the socket you want to make non-blocking
        int status = fcntl(socketfd, F_SETFL, fcntl(socketfd, F_GETFL, 0) | O_NONBLOCK);

        if (status == -1){
            perror("calling fcntl");
            // handle the error.  By the way, I've never seen fcntl fail in this way
        }
        return true;
    }

    static bool CloseSocket(SOCKET socketfd) throw()
    {
        close(socketfd);
    }

    static bool SetReuseAddr(SOCKET socketfd) throw()
    {
        int enable = 1;
        // Forcefully attaching socket to the port 8080
        if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                       &enable, sizeof(enable)))
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        return true;
    }

}
#endif //SHARED_SOCKETS_HPP

