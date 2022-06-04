cmake_minimum_required(VERSION 3.15)
project(dlsmon
  LANGUAGES CXX
  VERSION 1.0
)

add_subdirectory(packages/prometheus-cpp)
add_subdirectory(src)
