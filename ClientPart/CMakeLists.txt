cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Client)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Quick QuickControls2 Qml Network
)

set(CLIENT_SOURCES
    Network/TcpClient.cpp
    Network/Router.cpp
    Network/PacketIO_Client.cpp

    App/AppState.cpp
    App/Handler.cpp
    App/Appcontroller.cpp
    App/DialogManager.cpp
    App/Logging.cpp

    ViewModels/LoginViewModel.cpp
    ViewModels/SearchViewModel.cpp

    ../common/Validator.cpp
)

set(CLIENT_HEADERS
    Network/TcpClient.h
    Network/Router.h
    Network/PacketIO_Client.h

    App/AppState.h
    App/Handler.h
    App/Appcontroller.h
    App/Logging.h

    App/DialogManager.h

    ViewModels/LoginViewModel.h
    ViewModels/SearchViewModel.h

    ../common/Message.h
    ../common/protocol.h
    ../common/Validator.h
    ../common/User.h
)

add_library(client_core STATIC
    ${CLIENT_SOURCES}
    ${CLIENT_HEADERS}
)

target_include_directories(client_core PUBLIC
    Network
    App
    ViewModels
    ../common
)

target_link_libraries(client_core PUBLIC
    Qt6::Network
    Qt6::Qml
)

qt_add_executable(Client
    main.cpp

    resources.qrc
    qml.qrc
    android/src/org/qtproject/example/BackgroundService.java
)

set_target_properties(Client PROPERTIES
   QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")

qt_add_qml_module(Client
    URI ClientModule
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/LoginScreen.qml
        qml/MainScreen.qml
        qml/RegisterScreen.qml
        qml/DesktopLayout.qml
        qml/MobileLayout.qml
        qml/ConnectionIndicator.qml
        qml/ServerDialog.qml
    NO_PLUGIN
    SOURCES
    SOURCES
)

target_include_directories(Client PRIVATE
    Network
    App
    ViewModels
    ../common
    styles
    qml
)

target_link_libraries(Client PRIVATE
    client_core
    Qt6::Network
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
)

if (WIN32)
    target_link_libraries(Client PRIVATE ws2_32)
endif()

if (ANDROID)
    include(FetchContent)
    FetchContent_Declare(
      android_openssl
      DOWNLOAD_EXTRACT_TIMESTAMP true
      URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    add_android_openssl_libraries(Client)
    qt_finalize_target(Client)
endif()

if (NOT ANDROID)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(Client PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug-sanitize" AND NOT ANDROID)
    option(ENABLE_SANITIZERS "Enable sanitizers" OFF)

    if(ENABLE_SANITIZERS)
        set(SANITIZER_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer -g)

        # set(SANITIZER_FLAGS -fsanitize=thread,undefined -fno-omit-frame-pointer -g)

        target_compile_options(client_core PRIVATE ${SANITIZER_FLAGS})
        target_compile_options(Client PRIVATE ${SANITIZER_FLAGS})
        target_link_options(Client PRIVATE ${SANITIZER_FLAGS})

    endif()
endif()