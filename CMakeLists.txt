cmake_minimum_required(VERSION 3.15)
project(pxs3c LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find LLVM for JIT compilation (unless explicitly disabled)
if(NOT DEFINED LLVM_FOUND)
  execute_process(
    COMMAND llvm-config --version
    OUTPUT_VARIABLE LLVM_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  
  if(LLVM_VERSION)
    message(STATUS "LLVM found: ${LLVM_VERSION}")
    
    # Get LLVM include directories
    execute_process(
      COMMAND llvm-config --includedir
      OUTPUT_VARIABLE LLVM_INCLUDE_DIR
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Get LLVM libraries using llvm-config
    execute_process(
      COMMAND llvm-config --libs all
      OUTPUT_VARIABLE LLVM_LIBS
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Get LLVM system libs
    execute_process(
      COMMAND llvm-config --system-libs
      OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Get LLVM flags
    execute_process(
      COMMAND llvm-config --cxxflags
      OUTPUT_VARIABLE LLVM_CXXFLAGS
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    set(LLVM_FOUND TRUE)
    include_directories(${LLVM_INCLUDE_DIR})
  else()
    message(STATUS "LLVM not found, building without JIT optimization")
    set(LLVM_FOUND FALSE)
  endif()
else()
  if(LLVM_FOUND)
    message(STATUS "LLVM disabled by parent project")
  endif()
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
  target_link_libraries(pxs3c_core PRIVATE ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS})
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
