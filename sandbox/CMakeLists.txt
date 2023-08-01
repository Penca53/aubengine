cmake_minimum_required(VERSION 3.20.2) 

include(FetchContent)

project(sandbox)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/include/*.h"
        "${PROJECT_SOURCE_DIR}/src/*.cc"
        )
add_executable(${PROJECT_NAME} ${SRC_FILES})

# For ASIO, header-only lib without CMake support
target_include_directories(${PROJECT_NAME} PRIVATE ./third_party/asio/asio/include)
target_include_directories(${PROJECT_NAME} PUBLIC ./include)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /Wall)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE aubengine)