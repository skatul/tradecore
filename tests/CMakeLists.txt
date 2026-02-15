add_executable(tradecore_tests
    test_protocol.cpp
    test_matching_engine.cpp
    test_order_book.cpp
    test_book_keeper.cpp
    test_order_manager.cpp
    test_metrics.cpp
    test_config.cpp
    ../src/messaging/protocol.cpp
    ../src/matching/matching_engine.cpp
    ../src/matching/order_book.cpp
    ../src/booking/book_keeper.cpp
    ../src/orders/order_manager.cpp
    ../src/core/config.cpp
)

target_include_directories(tradecore_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tradecore_tests PRIVATE
    GTest::gtest_main
    fix_proto
    cppzmq
    spdlog::spdlog
    tomlplusplus::tomlplusplus
)

include(GoogleTest)
gtest_discover_tests(tradecore_tests)

# Integration tests (require ZMQ sockets)
add_executable(tradecore_integration_tests
    test_integration.cpp
    ../src/messaging/protocol.cpp
    ../src/messaging/zmq_server.cpp
    ../src/matching/matching_engine.cpp
    ../src/matching/order_book.cpp
    ../src/booking/book_keeper.cpp
    ../src/orders/order_manager.cpp
)

target_include_directories(tradecore_integration_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tradecore_integration_tests PRIVATE
    GTest::gtest_main
    fix_proto
    cppzmq
    spdlog::spdlog
)

gtest_discover_tests(tradecore_integration_tests)
