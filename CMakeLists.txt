cmake_minimum_required(VERSION 3.24)
project(tradecore VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/Dependencies.cmake)

add_executable(tradecore
    src/main.cpp
    src/messaging/zmq_server.cpp
    src/messaging/protocol.cpp
    src/orders/order_manager.cpp
    src/matching/matching_engine.cpp
    src/booking/book_keeper.cpp
)

target_include_directories(tradecore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(tradecore PRIVATE cppzmq nlohmann_json::nlohmann_json)

# Tests
option(TRADECORE_BUILD_TESTS "Build tests" ON)
if(TRADECORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
