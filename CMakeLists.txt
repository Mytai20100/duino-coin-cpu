cmake_minimum_required(VERSION 3.10)
project(duino-cpu VERSION 4.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -mtune=native -funroll-loops -ffast-math -flto")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -march=native -mtune=native -funroll-loops -ffast-math -flto")

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

add_executable(duino-cpu
    src/main.cpp
    src/miner.cpp
    src/network.cpp
    src/hasher.cpp
    src/benchmark.cpp
    src/logger.cpp
    src/http.cpp
    src/json.cpp
)

target_include_directories(duino-cpu PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CURL_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
)

target_link_libraries(duino-cpu PRIVATE 
    Threads::Threads
    OpenSSL::SSL 
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
)

install(TARGETS duino-cpu DESTINATION bin)