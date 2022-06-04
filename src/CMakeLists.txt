include_directories(.)
add_executable(dslmon main.cpp)
target_link_libraries(dslmon PRIVATE prometheus-cpp::pull)
