cmake_minimum_required(VERSION 3.25) # modern baseline, works with your CMake 4.1.1+

# Project metadata
project(OrderBookApp 
    VERSION 0.1.0
    DESCRIPTION "A high-performance C++ implementation of a Limit Order Book"
    LANGUAGES CXX
)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Avoid compiler-specific extensions

# Add the main executable target
add_executable(OrderBookApp src/main.cpp)

# Enable stricter warnings (per-target, not globally)
if (MSVC)
    target_compile_options(OrderBookApp PRIVATE /W4 /permissive-)
else()
    target_compile_options(OrderBookApp PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Example: if you later add include directories
# target_include_directories(OrderBookApp PRIVATE include)

