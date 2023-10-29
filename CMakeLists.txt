cmake_minimum_required(VERSION 3.26)
project(alsa-controller)
include_directories(/usr/lib/x86_64-linux-gnu)
set(CMAKE_CXX_STANDARD 17)

add_executable(alsa-controller main.cpp)
target_link_libraries(alsa-controller libasound.so.2)
