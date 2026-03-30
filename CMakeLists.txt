cmake_minimum_required(VERSION 3.16)
project(Biocinoz LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

add_executable(biocinoz
    src/main.cpp
)

target_link_libraries(biocinoz PRIVATE sfml-graphics sfml-window sfml-system)

target_compile_options(biocinoz PRIVATE -Wall -Wextra -Wpedantic)
