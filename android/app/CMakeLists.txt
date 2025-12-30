cmake_minimum_required(VERSION 3.15)
project(pxs3c_android LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to find LLVM for Android NDK
# If not available, builds will use interpreter fallback
# LLVM_FOUND will be set by root CMakeLists.txt

# Build pxs3c_core from root
add_subdirectory(${CMAKE_SOURCE_DIR}/../.. ${CMAKE_BINARY_DIR}/pxs3c_root)

add_library(pxs3c_jni SHARED
  ../../src/android/jni/pxs3c_jni.cpp
)

target_include_directories(pxs3c_jni PRIVATE ../../src)

target_link_libraries(pxs3c_jni PRIVATE pxs3c_core
    log
    android
    vulkan
