cmake_minimum_required(VERSION 3.5)
project(Client)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

set(CLIENT_SOURCES
    main.cpp
    Network/tcp_client.cpp
    Network/MessageRouter.cpp

    App/AppState.cpp
    App/Handler.cpp
    App/appcontroller.cpp
    App/StateBinder.cpp
    App/DialogManager.cpp


    UI/loginwindow.cpp
    UI/mainwindow.cpp
    UI/uicontroller.cpp

    ../common/PacketIO.cpp
    ../common/Validator.cpp
)

set(CLIENT_HEADERS
    Network/tcp_client.h
    Network/MessageRouter.h

    App/AppState.h
    App/Handler.h
    App/appcontroller.h
    App/StateBinder.h
    App/DialogManager.h


    UI/loginwindow.h
    UI/mainwindow.h
    UI/uicontroller.h

    ../common/PacketIO.h
    ../common/Message.h
    ../common/protocol.h
    ../common/Validator.h
)

find_package(OpenSSL REQUIRED)

# add_subdirectory(tests)

qt_add_executable(Client
    ${CLIENT_SOURCES}
    ${CLIENT_HEADERS}

    UI/loginwindow.ui
    UI/mainwindow.ui

    resources.qrc
)

target_include_directories(Client PRIVATE
    Network
    App
    UI
    ../common
    styles
)

target_link_libraries(Client PRIVATE
    Qt6::Widgets
    OpenSSL::SSL
)

if (WIN32)
    target_link_libraries(Client PRIVATE ws2_32)
endif()
