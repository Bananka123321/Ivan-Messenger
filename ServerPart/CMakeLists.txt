cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#add_compile_options(-Wall -Wextra -Wshadow -Werror -Wno-unused-parameter)

project(Server)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SERVER_SOURCES
    src/TcpServer.cpp
    src/ClientSession.cpp
    src/Handler.cpp
    src/SessionManager.cpp
    src/MessageDispatcher.cpp
    src/Config.cpp
    src/SignalHandler.cpp
    src/PacketIO_Server.cpp

    src/DB_ConnectionPool.cpp
    src/DB_MessageManager.cpp
    src/DB_DialogManager.cpp
    src/DB_UserManager.cpp
    src/DB_DevicesSessionManager.cpp
    src/DB_TemporaryTokenManager.cpp

    ../common/Validator.cpp
)

find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
pkg_check_modules(ARGON2 REQUIRED libargon2)

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

add_library(server_core STATIC ${SERVER_SOURCES})

target_include_directories(server_core PUBLIC
    include
    ../common
)

target_link_libraries(server_core PUBLIC
    pqxx pq crypto pthread
    ${ARGON2_LIBRARIES}
    OpenSSL::SSL
)

add_executable(Server src/main.cpp)
target_link_libraries(Server PRIVATE server_core)

option(BUILD_TESTS "Build unit, integration and contract tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
