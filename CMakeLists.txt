cmake_minimum_required(VERSION 3.20.0)
project(count-dots VERSION 1.0)

add_executable(main main.cc)
target_compile_features(main PUBLIC cxx_std_20)
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})