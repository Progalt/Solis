cmake_minimum_required(VERSION 3.20)

project(SolisLang VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)


add_subdirectory(Solis)
add_subdirectory(Testbed)
add_subdirectory(CLI)