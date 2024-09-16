cmake_minimum_required(VERSION 3.23)
project(TraceLogPass)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -Wall -Wextra")

# 寻找LLVM包并包含相关模块
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Found LLVM Cmake ${LLVM_CMAKE_DIR}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_llvm_library(TraceLogPass MODULE library.cpp PLUGIN_TOOL opt)
if (LLVM_LINK_DYLIB)
    message(STATUS "LLVM linking: dynamic")
    set(llvm_libs LLVM)
else()
    message(STATUS "LLVM linking: static")
    llvm_map_components_to_libnames(llvm_libs linker support core irreader passes)
endif()
target_link_libraries(TraceLogPass PRIVATE ${llvm_libs})
