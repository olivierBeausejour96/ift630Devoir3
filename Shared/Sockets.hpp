#ifndef SHARED_SOCKETS_HPP
#define SHARED_SOCKETS_HPP

namespace Network
{

#define SOCKET int
#define INVALID_SOCKET -1
#include <exception>

    class FunctionUndefinedException: std::exception {};

    static bool SetNonBlocking(int) throw()
    {
        throw FunctionUndefinedException();
    }
    static bool CloseSocket(int) throw()
    {
        throw FunctionUndefinedException();
    }
    static bool SetReuseAddr(int) throw()
    {
        throw FunctionUndefinedException();
    }

}
#endif //SHARED_SOCKETS_HPP

