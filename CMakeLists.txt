cmake_minimum_required(VERSION 3.19)

# set project name
project(Downloads)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++") # Specify to use clang++ as out C++ compiler
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")

find_package(OpenGL REQUIRED) # Finding OpenGL Package
find_package(glfw3 3.3 REQUIRED) # Finding glfw Package

include_directories("${CMAKE_SOURCE_DIR}/Include")
include_directories("${CMAKE_SOURCE_DIR}/Lib")
include_directories(${OPENGL_INCLUDE_DIR})

add_executable(Downloads
        ${PROJECT_SOURCE_DIR}/Apps/2-ClearScreen-Mac.cpp
#        ${PROJECT_SOURCE_DIR}/Apps/3-RayTrace.cpp # Does not work yet
#        ${PROJECT_SOURCE_DIR}/Apps/4-ColorfulTriangle.cpp
#        ${PROJECT_SOURCE_DIR}/Apps/7-CubePersp.cpp # Does not work yet
        ${PROJECT_SOURCE_DIR}/Lib/GLXtras.cpp
        ${PROJECT_SOURCE_DIR}/Lib/glad.c
        )


target_link_libraries(Downloads ${OPENGL_LIBRARIES}) # Adding OpenGL to the linker
target_link_libraries(Downloads glfw) # Adding glfw to the linker



# Ignore deprecated errors
target_compile_options(
        Downloads PRIVATE -Wno-deprecated
)

# Set compiler feature -std=c++17
target_compile_features(Downloads PRIVATE cxx_std_17)
