cmake_minimum_required(VERSION 3.10)
project(duino-cpu VERSION 4.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Haswell architecture - có AVX2 nhưng KHÔNG có AVX-512
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=haswell -mtune=haswell")
    
    # Force enable AVX2 only (NO AVX-512)
    add_definitions(-DUSE_AVX2)
    set(AVX_FLAGS "-mavx2")
    message(STATUS "AVX2 support enabled (forced)")
endif()

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CURL_INCLUDE_DIRS})

set(SOURCES
    src/main.cpp
    src/network.cpp
    src/logger.cpp
    src/json.cpp
    src/http.cpp
    src/benchmark.cpp
)

set(HASHER_SOURCE src/hasher.cpp)
set(MINER_SOURCE src/miner.cpp)

add_executable(duino-cpu ${SOURCES} ${HASHER_SOURCE} ${MINER_SOURCE})

# Apply AVX2 flags to hasher and miner
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND AVX_FLAGS)
    set_source_files_properties(${HASHER_SOURCE} PROPERTIES 
        COMPILE_FLAGS ${AVX_FLAGS})
    set_source_files_properties(${MINER_SOURCE} PROPERTIES 
        COMPILE_FLAGS ${AVX_FLAGS})
endif()

target_link_libraries(duino-cpu 
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
)

install(TARGETS duino-cpu DESTINATION bin)