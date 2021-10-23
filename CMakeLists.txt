cmake_minimum_required(VERSION 3.20)
project(EvolvingRobby)

set(CMAKE_CXX_STANDARD 17)

find_package(fmt)

add_executable(evolve src/main.cpp)
target_link_libraries(evolve fmt::fmt-header-only)