cmake_minimum_required(VERSION 3.15)
project(pxs3c LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find LLVM for JIT compilation
find_package(LLVM QUIET)
if(LLVM_FOUND)
  message(STATUS "LLVM found: ${LLVM_PACKAGE_VERSION}")
  include_directories(${LLVM_INCLUDE_DIRS})
  add_definitions(${LLVM_DEFINITIONS})
else()
  message(STATUS "LLVM not found, building without JIT optimization")
endif()

add_library(pxs3c_core
    src/core/Emulator.cpp
    src/core/FramePacer.cpp
    src/core/Config.cpp
    src/core/SyscallHandler.cpp
    src/cpu/engines/Rpcs3Bridge.cpp
    src/cpu/PPUInterpreter.cpp
    src/cpu/PPUJIT.cpp
    src/cpu/SPUInterpreter.cpp
    src/cpu/SPUManager.cpp
    src/rsx/VulkanRenderer.cpp
    src/rsx/RSXCommands.cpp
    src/rsx/RSXProcessor.cpp
    src/loader/ElfLoader.cpp
    src/loader/SELFLoader.cpp
    src/memory/MemoryManager.cpp
)

# Add LLVM source if available
if(LLVM_FOUND)
  target_sources(pxs3c_core PRIVATE src/cpu/LLVMJITCompiler.cpp)
  target_link_libraries(pxs3c_core ${LLVM_LIBS})
  target_compile_definitions(pxs3c_core PRIVATE LLVM_AVAILABLE=1)
endif()

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
