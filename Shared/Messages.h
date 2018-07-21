//
// Created by beao3002 on 18-07-21.
//

#ifndef SHARED_MESSAGES_H
#define SHARED_MESSAGES_H


namespace Network
{
    namespace Messages
    {
        enum Base Type;
        enum Base Result;

        class Base
        {
        public:
            template<class M>
            bool is() const { return mType == M::StaticType; }
            template<class M>
            const M* as() const { return static_cast<const M*>(this); }

            uint64_t idFrom;

            Base(Type type, Result result)
                    : mType(type),
                      mResult(result)
            {}
        private:
            Type mType;
            Result mResult;
        };

        class Connection: Base
        {

        };

        class Disconnection: Base
        {

        };

        class UserData: Base
        {

        };

        enum Base Type
        {
            Messages::Connection,
            Messages::Disconnection,
            Messages::UserData
        };

        enum Base Result
        {
            Success
        };

    }
}



#endif //SHARED_MESSAGES_H
