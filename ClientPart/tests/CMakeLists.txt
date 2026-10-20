include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

find_package(Qt6 REQUIRED COMPONENTS Test)

set(TEST_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)
if(NOT EXISTS ${TEST_DATA_DIR}/test.crt)
    find_program(OPENSSL_EXECUTABLE openssl REQUIRED)
    file(MAKE_DIRECTORY ${TEST_DATA_DIR})
    execute_process(
        COMMAND ${OPENSSL_EXECUTABLE} req -x509 -newkey rsa:2048 -nodes
                -keyout test.key -out test.crt -days 3650 -subj /CN=localhost
        WORKING_DIRECTORY ${TEST_DATA_DIR}
        OUTPUT_QUIET ERROR_QUIET
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

add_library(client_test_support STATIC
    support/test_main.cpp
    support/FakeServer.h
    support/FakeServer.cpp
)
target_include_directories(client_test_support PUBLIC support)
target_link_libraries(client_test_support PUBLIC client_core Qt6::Test GTest::gtest GTest::gmock)
target_compile_definitions(client_test_support PUBLIC
    TEST_CERT_FILE="${TEST_DATA_DIR}/test.crt"
    TEST_KEY_FILE="${TEST_DATA_DIR}/test.key"
)

function(add_client_test name label)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE client_test_support)
    gtest_discover_tests(${name}
        PROPERTIES LABELS ${label}
        DISCOVERY_TIMEOUT 30
    )
endfunction()

add_client_test(test_packet_io_client unit unit/test_packet_io_client.cpp)
add_client_test(test_app_state unit unit/test_app_state.cpp)
add_client_test(test_handler unit unit/test_handler.cpp)
add_client_test(test_dialog_manager unit unit/test_dialog_manager.cpp)
add_client_test(test_view_models unit unit/test_view_models.cpp)
add_client_test(test_network integration integration/test_network.cpp)
add_client_test(test_app_flow integration integration/test_app_flow.cpp)
