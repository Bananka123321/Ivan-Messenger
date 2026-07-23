cmake_minimum_required(VERSION 3.15)
project(Server)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SERVER_SOURCES
    src/main.cpp
    src/TCPServer.cpp
    src/DB_UserManager.cpp
    src/ClientSession.cpp
    src/Handler.cpp
    src/SessionManager.cpp
    src/MessageDispatcher.cpp
    src/Config.cpp
    src/DB_MessageManager.cpp
    src/DB_DialogManager.cpp
    src/SignalHandler.cpp
    
    ../common/Validator.cpp
    ../common/PacketIO.cpp
)

find_library(ARGON2_LIB argon2 REQUIRED)
find_package(OpenSSL REQUIRED)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer ENABLED")
    add_compile_options(-fsanitize=address -g -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
if(ENABLE_TSAN)
    message(STATUS "ThreadSanitizer ENABLED")
    add_compile_options(-fsanitize=thread -g -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
endif()

option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
if(ENABLE_UBSAN)
    message(STATUS "UndefinedBehaviorSanitizer ENABLED")
    add_compile_options(-fsanitize=undefined -g -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
endif()

add_executable(Server ${SERVER_SOURCES})

target_include_directories(Server PRIVATE
    include
    ../common
)

target_link_libraries(Server PRIVATE
    pqxx pq crypto pthread
    ${ARGON2_LIB}
    OpenSSL::SSL
)