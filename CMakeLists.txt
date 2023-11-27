cmake_minimum_required (VERSION 3.16)
project (myproj VERSION 1.0)

find_package (glog 0.6.0 REQUIRED)
find_package(spdlog REQUIRED)
find_package(Boost REQUIRED COMPONENTS log)

add_executable (myapp /Users/jimmyliu/VSCodeProjects/log_analysist_tool/benchmark.cpp)
target_link_libraries (myapp PRIVATE glog::glog spdlog::spdlog ${Boost_LIBRARIES})