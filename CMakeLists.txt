cmake_minimum_required(VERSION 3.16)
project(Biocinoz LANGUAGES CXX)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 2.6 COMPONENTS graphics window system QUIET)

if(NOT SFML_FOUND)
    include(FetchContent)

    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build SFML as static libraries" FORCE)
    set(SFML_BUILD_AUDIO OFF CACHE BOOL "Disable SFML audio module" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "Disable SFML network module" FORCE)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "Disable SFML examples" FORCE)
    set(SFML_BUILD_DOC OFF CACHE BOOL "Disable SFML documentation" FORCE)

    FetchContent_Declare(
        SFML
        URL https://github.com/SFML/SFML/archive/refs/tags/2.6.2.zip
    )

    FetchContent_MakeAvailable(SFML)
endif()

add_executable(biocinoz
    src/Animal.cpp
    src/Animal.h
    src/Entity.cpp
    src/Entity.h
    src/Herbivore.cpp
    src/Herbivore.h
    src/main.cpp
    src/Plant.cpp
    src/Plant.h
    src/Predator.cpp
    src/Predator.h
    src/Renderer.h
    src/Renderer.cpp
    src/RenderParticle.h
    src/SpeciesConfig.h
    src/Vec3.cpp
    src/Vec3.h
    src/World.cpp
    src/World.h
)

target_link_libraries(biocinoz PRIVATE sfml-graphics sfml-window sfml-system)
target_compile_definitions(biocinoz PRIVATE SFML_STATIC)

if(MSVC)
    target_compile_options(biocinoz PRIVATE /W4)
else()
    target_compile_options(biocinoz PRIVATE -Wall -Wextra -Wpedantic)
endif()
