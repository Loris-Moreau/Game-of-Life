cmake_minimum_required(VERSION 3.10)
project(GameOfLife LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------------------------------------
# 1) Add GLAD
# --------------------------------------------------------------------------------
add_library(glad STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/glad.c
)
target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/glad    # has glad.h and KHR/
)

# --------------------------------------------------------------------------------
# 2) Add ImGui
# --------------------------------------------------------------------------------
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui)
file(GLOB IMGUI_CORE
    ${IMGUI_DIR}/*.cpp
)
set(IMGUI_BACKENDS
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC
    ${IMGUI_CORE}
    ${IMGUI_BACKENDS}
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# --------------------------------------------------------------------------------
# 3) GLFW
# --------------------------------------------------------------------------------
# Assumes you have a copy of GLFW in libs/GLFW
# You can also install it system‑wide and use find_package(GLFW3 REQUIRED)
add_subdirectory(libs/glfw)
# GLFW will define the target glfw (or glfw3) and set include directories

# --------------------------------------------------------------------------------
# 4) Our executable
# --------------------------------------------------------------------------------
add_executable(GameOfLife
    main.cpp
)

target_link_libraries(GameOfLife PRIVATE
    glad
    imgui
    glfw        # if your GLFW subdirectory defines target glfw
    # or glfw3  # if that's the target name
    opengl32    # on Windows, the system OpenGL library
)

# Ensure the headers are visible to our .cpp
target_include_directories(GameOfLife PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/glad			  # for glad.h
    ${IMGUI_DIR}									 # for imgui.h
    ${IMGUI_DIR}/backends							# for imgui_impl_*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw/include  # for glfw3.h
)

# On Windows, if you built static GLFW you may need these system libs:
if (WIN32)
    target_link_libraries(GameOfLife PRIVATE
        gdi32
        user32
        shell32
        advapi32
    )
endif()
