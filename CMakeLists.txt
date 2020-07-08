cmake_minimum_required(VERSION 3.10)
project(osfinal)

set(CMAKE_CPP_STANDARD 11)

link_libraries( -lpthread )
add_executable(osfinal main.cpp memory.cpp memory.h sortmem.cpp sortmem.h)
add_executable(createfile createfile.cpp)
