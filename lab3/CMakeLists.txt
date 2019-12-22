cmake_minimum_required(VERSION 3.8)
project(lab3)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Main router executable

file(GLOB sources src/*.cpp)

include_directories(libs/boost_1_71_0/include)
link_libraries(pthread rt)

add_library(core STATIC ${sources})
link_libraries(core)

add_executable(control_node_bin src/bin/control_node_bin.cpp)

add_executable(worker_node_bin src/bin/worker_node_bin.cpp)

add_executable(focus_node_bin src/bin/focus_node_bin.cpp)

add_executable(sun_node_bin src/bin/sun_node_bin.cpp)

# An utility for cleaning message queues (for some reason
# sometimes boost::interprocess glitches and creates multiple
# copies of one message queue, then it's impossible to use it
# from different processes)

add_executable(clean_channels_bin src/bin/clean_channels_bin.cpp)