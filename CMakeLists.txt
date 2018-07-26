cmake_minimum_required(VERSION 3.8)

include_directories(.)

project(Server)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES_SERVER Server/main.cpp Server/Server.h Server/Server.cpp Client/Client.h Client/Client.cpp)
add_executable(Server ${SOURCE_FILES_SERVER})

project(Client)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES_CLIENT Client/main.cpp Client/Client.cpp Client/Client.h)
add_executable(Client ${SOURCE_FILES_CLIENT})

project(Shared)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES_SHARED Shared/Messages.h Shared/Messages.cpp)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
#add_executable(Shared ${SOURCE_FILES_SHARED})