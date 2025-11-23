cmake_minimum_required(VERSION 3.21)
project(bvh_exp)

# Use C++20 for the project
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_executable(bvh_exp bvh_exp.cpp)
add_subdirectory(contrib/bvh)
target_link_libraries(bvh_exp PUBLIC bvh)
# The example uses a helper from the bvh tests (load_obj). It's not part
# of the bvh library, so add the test loader directly to the executable.
target_sources(bvh_exp PRIVATE ${CMAKE_SOURCE_DIR}/contrib/bvh/test/load_obj.cpp)
