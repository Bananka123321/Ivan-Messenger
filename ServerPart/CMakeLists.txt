cmake_minimum_required(VERSION 3.15)
project(Server)

set(SERVER_SOURCES
    src/main.cpp
    src/TCPServer.cpp
    src/UserManager.cpp
    src/ClientSession.cpp
    src/Handler.cpp
    src/SessionManager.cpp
    src/MessageDispatcher.cpp
    src/Config.cpp
    ../common/PacketIO.cpp
)

add_executable(Server ${SERVER_SOURCES})

target_include_directories(Server PRIVATE
    include
    ../common
)

target_link_libraries(Server PRIVATE
    pqxx pq crypto
)