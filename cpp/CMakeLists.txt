add_library(trading_project_options INTERFACE)

target_compile_features(trading_project_options INTERFACE cxx_std_20)



add_library(trading_common STATIC common/logging.cpp)
target_link_libraries(trading_common PUBLIC trading_project_options)
target_include_directories(trading_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)



add_executable(trading_app app/main.cpp)
target_link_libraries(trading_app PRIVATE trading_project_options trading_common)

# Order book library
add_library(trading_order_book STATIC
    order_book/order_book.cpp
)

target_link_libraries(trading_order_book PUBLIC
    trading_project_options
    trading_common
)

target_include_directories(trading_order_book PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/order_book
)

add_library(trading_strategy STATIC
    strategy/market_making_strategy.cpp
)

target_link_libraries(trading_strategy PUBLIC
    trading_project_options
    trading_common
    trading_order_book
)

target_include_directories(trading_strategy PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/strategy
    ${CMAKE_CURRENT_SOURCE_DIR}/order_manager
)

add_library(trading_risk STATIC
    risk/risk_engine.cpp
)

target_include_directories(trading_risk
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/risk
        ${CMAKE_CURRENT_SOURCE_DIR}/order_manager
)

target_link_libraries(trading_risk
    PUBLIC
        trading_project_options
        trading_common
)

add_library(trading_order_manager STATIC
    order_manager/order_manager.cpp
)

target_include_directories(trading_order_manager
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/order_manager
)

target_link_libraries(trading_order_manager
    PUBLIC
        trading_project_options
        trading_common
)
