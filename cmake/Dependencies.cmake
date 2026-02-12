include(FetchContent)

# --- libzmq ---
FetchContent_Declare(
    libzmq
    GIT_REPOSITORY https://github.com/zeromq/libzmq.git
    GIT_TAG        v4.3.5
    GIT_SHALLOW    TRUE
)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC ON CACHE BOOL "" FORCE)
set(WITH_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libzmq)

# --- cppzmq (header-only) ---
# Download header directly and create an interface target that links to libzmq-static.
# This avoids cppzmq's find_package(ZeroMQ) which doesn't work with FetchContent.
FetchContent_Declare(
    cppzmq
    GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
    GIT_TAG        v4.10.0
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(cppzmq)
if(NOT cppzmq_POPULATED)
    FetchContent_Populate(cppzmq)
endif()

add_library(cppzmq INTERFACE)
target_include_directories(cppzmq INTERFACE ${cppzmq_SOURCE_DIR})
target_link_libraries(cppzmq INTERFACE libzmq-static)
# cppzmq needs the libzmq include dir for zmq.h
FetchContent_GetProperties(libzmq SOURCE_DIR libzmq_SOURCE_DIR)
target_include_directories(cppzmq INTERFACE
    ${libzmq_SOURCE_DIR}/include
    ${libzmq_BINARY_DIR}
)

# --- nlohmann/json ---
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

# --- GoogleTest (for tests only) ---
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
