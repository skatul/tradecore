cmake_minimum_required(VERSION 3.24)
project(tradecore VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/Dependencies.cmake)

# Compile protobuf schema
set(PROTO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/proto/fix_messages.proto)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto_gen)
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

add_custom_command(
    OUTPUT ${PROTO_GEN_DIR}/fix_messages.pb.cc ${PROTO_GEN_DIR}/fix_messages.pb.h
    COMMAND protobuf::protoc
        --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
        --cpp_out=${PROTO_GEN_DIR}
        ${PROTO_SRC}
    DEPENDS ${PROTO_SRC} protobuf::protoc
    COMMENT "Compiling protobuf schema"
)

add_library(fix_proto STATIC ${PROTO_GEN_DIR}/fix_messages.pb.cc)
target_include_directories(fix_proto PUBLIC ${PROTO_GEN_DIR})
target_link_libraries(fix_proto PUBLIC protobuf::libprotobuf)

add_executable(tradecore
    src/main.cpp
    src/messaging/zmq_server.cpp
    src/messaging/protocol.cpp
    src/orders/order_manager.cpp
    src/matching/matching_engine.cpp
    src/matching/order_book.cpp
    src/booking/book_keeper.cpp
    src/core/config.cpp
)

target_include_directories(tradecore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(tradecore PRIVATE cppzmq fix_proto spdlog::spdlog tomlplusplus::tomlplusplus)

# Tests
option(TRADECORE_BUILD_TESTS "Build tests" ON)
if(TRADECORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
