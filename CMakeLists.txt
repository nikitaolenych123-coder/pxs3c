cmake_minimum_required(VERSION 3.15)
project(pxs3c LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find LLVM for JIT compilation (unless explicitly disabled or cross-compiling for Android)
if(NOT DEFINED LLVM_FOUND)
  # Disable LLVM for Android NDK builds since LLVM is not available
  if(ANDROID)
    message(STATUS "Building for Android NDK - LLVM JIT disabled")
    set(LLVM_FOUND FALSE)
  else()
    find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config)

    if(LLVM_CONFIG_EXECUTABLE)
      execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
        OUTPUT_VARIABLE LLVM_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
      )
    endif()
    
    if(LLVM_CONFIG_EXECUTABLE AND LLVM_VERSION)
      message(STATUS "LLVM found: ${LLVM_VERSION}")
      
      # Get LLVM include directories
      execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
        OUTPUT_VARIABLE LLVM_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      
      # Get LLVM libraries using llvm-config
      execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs all
        OUTPUT_VARIABLE LLVM_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      
      # Get LLVM system libs
      execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs
        OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      
      # Get LLVM flags
      execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
        OUTPUT_VARIABLE LLVM_CXXFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      
      set(LLVM_FOUND TRUE)
    else()
      message(STATUS "LLVM not found, building without JIT optimization")
      set(LLVM_FOUND FALSE)
    endif()
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

# Add LLVM source and settings if available
if(LLVM_FOUND)
  message(STATUS "Configuring LLVM JIT support")
  target_sources(pxs3c_core PRIVATE src/cpu/LLVMJITCompiler.cpp)
  target_include_directories(pxs3c_core PRIVATE ${LLVM_INCLUDE_DIR})
  target_link_libraries(pxs3c_core PRIVATE ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS})
  target_compile_options(pxs3c_core PRIVATE ${LLVM_CXXFLAGS})
  target_compile_definitions(pxs3c_core PRIVATE LLVM_AVAILABLE=1)
else()
  message(STATUS "LLVM JIT support not available, using interpreter-only execution")
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
  target_link_libraries(pxs3c_jni PRIVATE pxs3c_core android vulkan)
endif()

if(UNIX)
  target_link_libraries(pxs3c_core PRIVATE dl)
endif()
