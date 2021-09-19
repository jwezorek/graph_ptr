cmake_minimum_required(VERSION 3.16)
project(graph_ptr_project LANGUAGES CXX)

add_executable(graph_ptr 
	src/main.cpp
    src/graph_ptr.cpp
 "src/test.h" "src/test.cpp")

target_include_directories(graph_ptr
	PUBLIC src
)

set_target_properties(graph_ptr
    PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS off
)