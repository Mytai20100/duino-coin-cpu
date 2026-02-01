cmake_minimum_required(VERSION 3.10)
project(duino-cpu VERSION 4.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -ffast-math -flto")

# Source files
set(SOURCES
    src/main.cpp
    src/benchmark.cpp
    src/hasher.cpp
    src/http.cpp
    src/json.cpp
    src/logger.cpp
    src/miner.cpp
    src/network.cpp
    src/config_yaml.cpp
    src/stats.cpp
)

# Required libraries
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)

# Check for SIMD support
include(CheckCXXCompilerFlag)

# AVX-512
check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512)
if(COMPILER_SUPPORTS_AVX512)
    option(ENABLE_AVX512 "Enable AVX-512 optimizations" OFF)
    if(ENABLE_AVX512)
        add_definitions(-DUSE_AVX512)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512f -mavx512bw")
        message(STATUS "AVX-512 support enabled")
    endif()
endif()

# AVX2
check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
if(COMPILER_SUPPORTS_AVX2 AND NOT ENABLE_AVX512)
    option(ENABLE_AVX2 "Enable AVX2 optimizations" ON)
    if(ENABLE_AVX2)
        add_definitions(-DUSE_AVX2)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
        message(STATUS "AVX2 support enabled")
    endif()
endif()

# ARM NEON
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    add_definitions(-DUSE_ARM_NEON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    message(STATUS "ARM NEON support enabled")
endif()

# Create executable
add_executable(duino-cpu ${SOURCES})

# Link libraries
target_link_libraries(duino-cpu
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    Threads::Threads
    yaml-cpp
)

# Include directories
target_include_directories(duino-cpu PRIVATE include)

# Installation
install(TARGETS duino-cpu DESTINATION bin)