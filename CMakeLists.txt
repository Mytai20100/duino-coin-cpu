cmake_minimum_required(VERSION 3.10)
project(duino-cpu VERSION 4.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detect architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(IS_ARM TRUE)
    message(STATUS "Building for ARM architecture")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
    set(IS_X86 TRUE)
    message(STATUS "Building for x86-64 architecture")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(IS_ARM)
        # ARM optimization - safe for all ARM64
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=armv8-a -mtune=cortex-a53")
        
        # Check for NEON support (ARM SIMD)
        include(CheckCXXCompilerFlag)
        CHECK_CXX_COMPILER_FLAG("-mfpu=neon" COMPILER_SUPPORTS_NEON)
        
        if(COMPILER_SUPPORTS_NEON)
            add_definitions(-DUSE_ARM_NEON)
            message(STATUS "ARM NEON support enabled")
        endif()
        
    elseif(IS_X86)
        # x86-64 baseline - compatible with all modern CPUs
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=x86-64 -mtune=generic")
        
        # Check AVX2 support
        include(CheckCXXCompilerFlag)
        CHECK_CXX_COMPILER_FLAG("-mavx2" COMPILER_SUPPORTS_AVX2)
        
        if(COMPILER_SUPPORTS_AVX2)
            set(AVX2_FLAGS "-mavx2 -mfma")
            add_definitions(-DUSE_AVX2)
            message(STATUS "AVX2 support available")
        endif()
    endif()
endif()

# Windows-specific settings
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7+
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CURL_INCLUDE_DIRS})
find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)

include_directories(${OPENSSL_INCLUDE_DIR} ${CURL_INCLUDE_DIR} ${YAML_CPP_INCLUDE_DIRS})

set(SOURCES
    src/main.cpp
    src/miner.cpp
    src/network.cpp
    src/logger.cpp
    src/hasher.cpp
    src/benchmark.cpp
    src/json.cpp
    src/http.cpp
    src/config_yaml.cpp
)

set(HASHER_SOURCE src/hasher.cpp)
set(MINER_SOURCE src/miner.cpp)

add_executable(duino-cpu ${SOURCES} ${HASHER_SOURCE} ${MINER_SOURCE})

# Apply optimization flags to hasher and miner
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(IS_X86 AND COMPILER_SUPPORTS_AVX2)
        set_source_files_properties(${HASHER_SOURCE} PROPERTIES 
            COMPILE_FLAGS ${AVX2_FLAGS})
        set_source_files_properties(${MINER_SOURCE} PROPERTIES 
            COMPILE_FLAGS ${AVX2_FLAGS})
    endif()
endif()

target_link_libraries(duino-cpu 
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    ${YAML_CPP_LIBRARIES}
    ${CURL_LIBRARIES}
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(duino-cpu ws2_32)
endif()

install(TARGETS duino-cpu DESTINATION bin)