cmake_minimum_required(VERSION 2.6.0)

project(SurfelMapping)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/Shaders" CACHE PATH "Where the shaders live")
#set(GUI_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gui/Shaders" CACHE PATH "Where the GUI shaders live")

#set(CMAKE_CXX_FLAGS ${ADDITIONAL_CMAKE_CXX_FLAGS} "-O3 -msse2 -msse3 -Wall -std=c++11 -DSHADER_DIR=${efusion_SHADER_DIR}")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -O0 -std=c++11 -DSHADER_DIR=${SHADER_DIR} -DGUI_SHADER_DIR=${GUI_SHADER_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -O0 -std=c++11 -DSHADER_DIR=${SHADER_DIR}")
set(CMAKE_BUILD_TYPE Debug)




find_package(Pangolin 0.1 REQUIRED)
#find_package(CUDA REQUIRED)
#find_package(SuiteSparse REQUIRED)
find_package(OpenCV REQUIRED)


# code for lib
include_directories(
        ${Pangolin_INCLUDE_DIRS}
        ${EIGEN_INCLUDE_DIRS}
        src
        src/Utils)

file(GLOB srcs src/*.cpp)
file(GLOB utils_srcs src/Utils/*.cpp)


# code for gui
include_directories(
        ${Pangolin_INCLUDE_DIRS}
        ${EIGEN_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIR}
        gui
)

file(GLOB gui_srcs gui/*.cpp)


add_library(surfelmapping SHARED
        ${srcs}
        ${utils_srcs}
)

target_link_libraries(surfelmapping
        ${Eigen_LIBRARIES}
        ${Pangolin_LIBRARIES}
)

add_library(surmap_gui SHARED
        ${gui_srcs}
)

target_link_libraries(surmap_gui
        ${OpenCV_LIBS}
        ${Eigen_LIBRARIES}
        ${Pangolin_LIBRARIES}
)


add_executable(test test_main.cpp)

target_link_libraries(test surmap_gui surfelmapping)

