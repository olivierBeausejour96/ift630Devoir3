//
// Created by beao3002 on 18-07-21.
//

#ifndef SHARED_MESSAGES_H
#define SHARED_MESSAGES_H

#include <stdint.h>
#include <netinet/in.h>
#include <vector>


namespace Network
{
    namespace Messages
    {
        class Base
        {
            public:
                template<class M>
                bool is() const { return mType == M::StaticType; }
                template<class M>
                const M* as() const { return static_cast<const M*>(this); }

                uint64_t idFrom;
                sockaddr_in from;

            protected:
                enum class Type {
                    Connection,
                    Disconnection,
                    UserData,
                };
                Base(Type type)
                        : mType(type)
                {}
            private:
                Type mType;
        };

        class Connection : public Base {
            public:
                const static Type StaticType = Type::Connection;
                enum class Result
                {
                    Success,
                    Failed
                };
                Connection() : Base(StaticType) {}
                Connection(Result result) : Base(StaticType),
                                            result(result)
                {}


                Result result;
        };

        class Disconnection : public Base {
            public:
                const static Type StaticType = Type::Disconnection;
                enum class Reason
                {
                    Disconnected,
                    Lost
                };
                Disconnection() : Base(StaticType) {}
                Disconnection(Reason reason) : Base(StaticType),
                                               mReason(reason)
                {}

            protected:
                Reason mReason;
        };

        class UserData : public Base {
            public:
                const static Type StaticType = Type::UserData;
                std::vector<unsigned char> data;
                UserData() : Base(StaticType) {}
                UserData(std::vector<unsigned char> _data) : Base(StaticType) {data = _data;}
                bool isFile;
                bool morePacket;
                int fileId;
        };


    }
}
#endif //SHARED_MESSAGES_H
