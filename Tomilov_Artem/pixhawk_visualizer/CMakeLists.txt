cmake_minimum_required(VERSION 3.10)
project(PixhawkIMUVisualizer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0072 NEW)

find_package(OpenGL REQUIRED)

find_package(glfw3 QUIET)
if (glfw3_FOUND)
    message(STATUS "GLFW3: найден через find_package")
    set(GLFW_LIBRARIES    glfw)
    set(GLFW_INCLUDE_DIRS "")
else()
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_search_module(GLFW REQUIRED glfw3)
        message(STATUS "GLFW3: найден через PkgConfig: ${GLFW_LIBRARIES}")
    else()
        find_path(GLFW_INCLUDE_DIRS NAMES GLFW/glfw3.h
            PATHS /usr/include /usr/local/include)
        find_library(GLFW_LIBRARIES NAMES glfw glfw3
            PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
        if (NOT GLFW_LIBRARIES)
            message(FATAL_ERROR "GLFW3 не найден.\nУстановите: sudo apt install libglfw3-dev")
        endif()
        message(STATUS "GLFW3: найден вручную: ${GLFW_LIBRARIES}")
    endif()
endif()

find_package(GLEW REQUIRED)

find_package(glm QUIET)
if (NOT glm_FOUND)
    find_path(GLM_INCLUDE_DIR NAMES glm/glm.hpp PATHS /usr/include /usr/local/include)
    if (NOT GLM_INCLUDE_DIR)
        message(FATAL_ERROR "GLM не найден. Установите: sudo apt install libglm-dev")
    endif()
    message(STATUS "GLM: найден вручную: ${GLM_INCLUDE_DIR}")
endif()

set(MAVLINK_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/c_library_v2)
if (NOT EXISTS ${MAVLINK_INCLUDE_DIR})
    message(WARNING "MAVLink c_library_v2 не найдена: ${MAVLINK_INCLUDE_DIR}")
endif()

set(SOURCES
    src/pixhawk_imu_reader.cpp
    src/imu_visualizer.cpp
)

add_executable(pixhawk_imu_reader ${SOURCES})

target_include_directories(pixhawk_imu_reader PRIVATE
    ${MAVLINK_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)
if (GLM_INCLUDE_DIR)
    target_include_directories(pixhawk_imu_reader PRIVATE ${GLM_INCLUDE_DIR})
endif()

target_link_libraries(pixhawk_imu_reader
    pthread
    OpenGL::GL
    ${GLFW_LIBRARIES}
    GLEW::GLEW
)

target_compile_options(pixhawk_imu_reader PRIVATE -Wall -Wextra -O2)