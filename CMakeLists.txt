cmake_minimum_required(VERSION 3.14)
project(webrtc-demo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g2 -ggdb -fpermissive -fno-rtti -Wall -O0 -pthread")
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")

add_definitions(-DWEBRTC_POSIX -DWEBRTC_LINUX -DWEBRTC_USE_H264 -DWEBRTC_APM_DEBUG_DUMP=0)
add_definitions("-fno-rtti -Wno-attributes")

message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(${CMAKE_SOURCE_DIR}/quickview_link_sdk/include)


add_executable(pusher main.cc reader.cc)
target_link_libraries(pusher
    ${CMAKE_SOURCE_DIR}/quickview_link_sdk/lib/libquickview_link.so
    dl
)

#编译  cmake ..   -DCMAKE_TOOLCHAIN_FILE=../arm64-toolchain.cmake
#运行  qemu-aarch64 -L /usr/aarch64-linux-gnu ./pusher -a ws://127.0.0.1:8080 -r 10