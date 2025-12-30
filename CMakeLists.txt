cmake_minimum_required(VERSION 3.15)
project(pxs3c LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(pxs3c_core
    src/core/Emulator.cpp
    src/core/FramePacer.cpp
    src/core/Config.cpp
    src/cpu/engines/Rpcs3Bridge.cpp
    src/cpu/PPUInterpreter.cpp
    src/rsx/VulkanRenderer.cpp
    src/loader/ElfLoader.cpp
    src/memory/MemoryManager.cpp
)

target_include_directories(pxs3c_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(pxs3c_smoke tests/smoke.cpp)
target_link_libraries(pxs3c_smoke pxs3c_core)

# Android-specific JNI shared lib is only built when targeting ANDROID
if(ANDROID)
  add_library(pxs3c_jni SHARED
    src/android/jni/pxs3c_jni.cpp
  )
  target_include_directories(pxs3c_jni PRIVATE src)
  target_link_libraries(pxs3c_jni PRIVATE pxs3c_core)
endif()

if(UNIX)
  target_link_libraries(pxs3c_core PRIVATE dl)
endif()
