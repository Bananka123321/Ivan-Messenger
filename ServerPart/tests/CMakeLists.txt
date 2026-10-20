include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

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

add_library(server_test_support STATIC
    support/TlsPair.cpp
    support/TestDatabase.cpp
)
target_include_directories(server_test_support PUBLIC support)
target_link_libraries(server_test_support PUBLIC server_core GTest::gtest)
target_compile_definitions(server_test_support PUBLIC
    TEST_CERT_FILE="${TEST_DATA_DIR}/test.crt"
    TEST_KEY_FILE="${TEST_DATA_DIR}/test.key"
    SCHEMA_SQL_FILE="${CMAKE_CURRENT_SOURCE_DIR}/../sql/schema.sql"
)

add_test(NAME test_db_start COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_db.sh start)
add_test(NAME test_db_stop COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_db.sh stop)
set_tests_properties(test_db_start PROPERTIES FIXTURES_SETUP test_db LABELS integration)
set_tests_properties(test_db_stop PROPERTIES FIXTURES_CLEANUP test_db LABELS integration)

function(add_server_test name label)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE server_test_support GTest::gtest_main GTest::gmock)

    set(extra_properties)
    if(label STREQUAL "integration")
        set(extra_properties
            FIXTURES_REQUIRED test_db
            RUN_SERIAL TRUE
            ENVIRONMENT MESSENGER_TEST_DB_REQUIRED=1
        )
    endif()

    gtest_discover_tests(${name}
        PROPERTIES LABELS ${label} ${extra_properties}
        DISCOVERY_TIMEOUT 30
    )
endfunction()

add_server_test(test_validator unit unit/test_validator.cpp)
add_server_test(test_message unit unit/test_message.cpp)
add_server_test(test_protocol unit unit/test_protocol.cpp)
add_server_test(test_config unit unit/test_config.cpp)
add_server_test(test_packet_io unit unit/test_packet_io.cpp)
add_server_test(test_client_session unit unit/test_client_session.cpp)
add_server_test(test_session_manager unit unit/test_session_manager.cpp)
add_server_test(test_message_dispatcher unit unit/test_message_dispatcher.cpp)
add_server_test(test_db_user_manager integration integration/test_db_user_manager.cpp)
add_server_test(test_db_tokens integration integration/test_db_tokens.cpp)
add_server_test(test_db_dialogs_messages integration integration/test_db_dialogs_messages.cpp)
