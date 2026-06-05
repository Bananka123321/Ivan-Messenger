qt_add_executable(Tests

    test_validator.cpp

    ../App/AppState.cpp
    ../App/Handler.cpp
    ../App/appcontroller.cpp
    ../App/DialogManager.cpp
    ../App/StateBinder.cpp
    ../Network/tcp_client.cpp
    ../Network/MessageRouter.cpp
    ../UI/mainwindow.cpp
    ../UI/loginwindow.cpp
    ../UI/uicontroller.cpp
    ../../common/PacketIO.cpp
    ../../common/Validator.cpp

    ../UI/mainwindow.ui
    ../UI/loginwindow.ui
)

target_include_directories(Tests PRIVATE
    ../Network
    ../App
    ../UI
    ../../common
    ../../common/nlohmann
)

target_link_libraries(Tests PRIVATE
    Qt6::Test
    Qt6::Widgets
    Qt6::Core
    OpenSSL::SSL
)

enable_testing()
add_test(NAME Tests COMMAND Tests)
