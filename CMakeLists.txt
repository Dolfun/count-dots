cmake_minimum_required(VERSION 3.20.0)
project(count-dots VERSION 1.0)

add_executable(main main.cc)
target_compile_features(main PUBLIC cxx_std_17)