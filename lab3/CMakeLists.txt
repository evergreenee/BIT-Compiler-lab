#cmake_minimum_required(VERSION 3.22)
#project(lab2)
#
#set(CMAKE_CXX_STANDARD 14)
#
#add_executable(lab2 compilerlab2.cpp)

cmake_minimum_required(VERSION 3.16)
project(lab03)
set(CMAKE_CXX_STANDARD 14)
add_compile_options(-pedantic)
add_executable(Compilerlab3
        compilerlab3.cpp
        )
target_compile_features(Compilerlab3 PRIVATE cxx_std_14)
