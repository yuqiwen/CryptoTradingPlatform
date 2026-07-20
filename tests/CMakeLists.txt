include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(order_book_tests
    order_book_tests.cpp
)

target_link_libraries(order_book_tests PRIVATE
    GTest::gtest_main
    trading_order_book
)

add_executable(order_status_transition_tests
    order_status_transition_tests.cpp
)

target_link_libraries(order_status_transition_tests PRIVATE
    GTest::gtest_main
    trading_common
)
target_include_directories(order_status_transition_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/cpp/order_manager
    ${PROJECT_SOURCE_DIR}/cpp/common
)
add_executable(market_making_strategy_tests
    market_making_strategy_tests.cpp
)

target_link_libraries(market_making_strategy_tests PRIVATE
    GTest::gtest_main
    trading_strategy
)

add_executable(risk_engine_tests
    risk_engine_tests.cpp
)

target_link_libraries(risk_engine_tests PRIVATE
    GTest::gtest_main
    trading_risk
)

add_executable(order_manager_tests
    order_manager_tests.cpp
)

target_link_libraries(order_manager_tests PRIVATE
    GTest::gtest_main
    trading_order_manager
)

include(GoogleTest)
gtest_discover_tests(order_book_tests)
gtest_discover_tests(order_status_transition_tests)
gtest_discover_tests(market_making_strategy_tests)
gtest_discover_tests(risk_engine_tests)
gtest_discover_tests(order_manager_tests)
