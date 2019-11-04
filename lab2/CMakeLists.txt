cmake_minimum_required(VERSION 3.8)
project(lab2)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Main router executable

file(GLOB sources src/*.cpp)

add_executable(router ${sources} src/bin/router.cpp)
target_include_directories(router PRIVATE libs/boost_1_71_0/include)
target_link_libraries(router PRIVATE pthread rt)

# An utility for cleaning message queues (for some reason
# sometimes boost::interprocess glitches and creates multiple
# copies of one message queue, then it's impossible to use it
# from different processes)

add_executable(clean_channels src/bin/clean_channels.cpp)
target_include_directories(clean_channels PRIVATE libs/boost_1_71_0/include)
target_link_libraries(clean_channels PRIVATE pthread rt)
