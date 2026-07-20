cmake_minimum_required(VERSION 3.20)

project(
    crypto_trading_platform
    VERSION 0.1.0
    DESCRIPTION "C++ event-driven crypto trading infrastructure"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CTP_BUILD_TESTS "Build unit tests" ON)

add_subdirectory(cpp)

if(CTP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
