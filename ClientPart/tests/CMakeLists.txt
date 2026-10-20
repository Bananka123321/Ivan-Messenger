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

add_library(client_test_support STATIC
    support/test_main.cpp
)
target_include_directories(client_test_support PUBLIC support)
target_link_libraries(client_test_support PUBLIC client_core Qt6::Test GTest::gtest GTest::gmock)

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
