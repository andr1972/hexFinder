cmake_minimum_required(VERSION 3.1)
project(hexFinder)
find_package(Boost COMPONENTS filesystem system)
set(CMAKE_CXX_STANDARD 14)
add_executable(hexFinder hexFinder.cpp)
target_link_libraries(hexFinder
                   boost_system
                   boost_filesystem
                   boost_regex)