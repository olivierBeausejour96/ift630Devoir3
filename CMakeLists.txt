cmake_minimum_required(VERSION 3.8)
project(Server)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES_SERVER Server/main.cpp)
add_executable(Server ${SOURCE_FILES_SERVER})

project(Client)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES_CLIENT Client/main.cpp)
add_executable(Client ${SOURCE_FILES_CLIENT})

project(Shared)

set(CMAKE_CXX_STANDARD 17)

#set(SOURCE_FILES_SHARED Shared/main.cpp)
#add_executable(Shared ${SOURCE_FILES_SHARED})