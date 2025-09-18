cmake_minimum_required(VERSION 3.25)

project(OrderBookApp 
    VERSION 0.1.0
    DESCRIPTION "A high-performance C++ implementation of a Limit Order Book"
    LANGUAGES CXX
)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Core library ----
add_library(orderbook
    src/LimitOrderBook.cpp
)
target_include_directories(orderbook PUBLIC src)

if (MSVC)
    target_compile_options(orderbook PRIVATE /W4 /permissive-)
else()
    target_compile_options(orderbook PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---- Main app ----
add_executable(OrderBookApp src/main.cpp)
target_link_libraries(OrderBookApp PRIVATE orderbook)

# ---- GoogleTest setup ----
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# ---- Tests ----
add_executable(OrderBookTests tests/OrderBookTests.cpp)
target_link_libraries(OrderBookTests PRIVATE orderbook gtest_main)

# ---- Link gperftools profiler (optional, if installed) ----
find_library(PROFILER_LIB profiler)
if (PROFILER_LIB)
    message(STATUS "Profiler library found: ${PROFILER_LIB}")
    target_link_libraries(OrderBookTests PRIVATE ${PROFILER_LIB})
endif()

include(GoogleTest)
gtest_discover_tests(OrderBookTests)
