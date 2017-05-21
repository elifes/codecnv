cmake_minimum_required(VERSION 2.6)
project(libskf)

add_executable(libskf main.cpp)

install(TARGETS libskf RUNTIME DESTINATION bin)
