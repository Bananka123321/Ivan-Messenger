include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

function(add_server_test name label)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE server_core GTest::gtest_main GTest::gmock)
    target_include_directories(${name} PRIVATE support)
    gtest_discover_tests(${name}
        PROPERTIES LABELS ${label}
        DISCOVERY_TIMEOUT 30
    )
endfunction()

add_server_test(test_validator unit unit/test_validator.cpp)
