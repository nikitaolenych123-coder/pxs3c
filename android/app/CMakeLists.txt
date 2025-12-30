cmake_minimum_required(VERSION 3.15)
project(pxs3c_android LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to find LLVM for Android NDK
# If not available, builds will use interpreter fallback
# LLVM_FOUND will be set by root CMakeLists.txt

# Build pxs3c_core and pxs3c_jni from root
add_subdirectory(${CMAKE_SOURCE_DIR}/../.. ${CMAKE_BINARY_DIR}/pxs3c_root)
