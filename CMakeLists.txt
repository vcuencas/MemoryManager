cmake_minimum_required(VERSION 3.21)
project(P2)

set(CMAKE_CXX_STANDARD 14)

add_executable(P2 cmake-build-debug/MemoryManager.cpp cmake-build-debug/MemoryManager.h cmake-build-debug/CommandLineTest.cpp)
