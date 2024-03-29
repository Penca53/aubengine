cmake_minimum_required(VERSION 3.20.2) 

include(FetchContent)

project(aubengine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/include/*.h"
        "${PROJECT_SOURCE_DIR}/src/*.cc"
        )
add_library(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ./include)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /Wall)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

FetchContent_Declare(
    glm
    URL https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip
)

FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    glad
    URL https://github.com/Dav1dde/glad/archive/refs/tags/v2.0.4.zip
)

FetchContent_MakeAvailable(glad)

add_subdirectory("${glad_SOURCE_DIR}/cmake" glad_cmake)
glad_add_library(glad_gl_core_mx_46 MX API gl:core=4.6)

FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.zip
)
FetchContent_MakeAvailable(glfw)

target_link_libraries(${PROJECT_NAME} PUBLIC glfw)

FetchContent_Declare(
    box2d
    URL https://github.com/erincatto/box2d/archive/refs/tags/v2.4.1.zip
)

set(BOX2D_BUILD_TESTBED CACHE INTERNAL OFF)
set(BOX2D_BUILD_UNIT_TESTS CACHE INTERNAL OFF)
FetchContent_MakeAvailable(box2d)

target_link_libraries(${PROJECT_NAME} PUBLIC glm glad_gl_core_mx_46 box2d)
