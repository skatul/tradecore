add_executable(tradecore_tests
    test_protocol.cpp
    test_matching_engine.cpp
    test_book_keeper.cpp
    test_order_manager.cpp
    ../src/messaging/protocol.cpp
    ../src/matching/matching_engine.cpp
    ../src/booking/book_keeper.cpp
    ../src/orders/order_manager.cpp
)

target_include_directories(tradecore_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tradecore_tests PRIVATE
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    cppzmq
)

include(GoogleTest)
gtest_discover_tests(tradecore_tests)
