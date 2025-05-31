cmake_minimum_required(VERSION 3.10)
project(spine-glfw)

include(FetchContent)
# Fetch GLFW
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG latest
        GIT_SHALLOW 1
)
FetchContent_MakeAvailable(glfw)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL " " FORCE)
set(GLFW_INSTALL OFF CACHE BOOL " " FORCE)

# Fetch glbinding
FetchContent_Declare(
        glbinding
        GIT_REPOSITORY https://github.com/cginternals/glbinding.git
        GIT_TAG v3.4.0
        GIT_SHALLOW 1
)
FetchContent_MakeAvailable(glbinding)
set(OPTION_BUILD_DOCS OFF CACHE BOOL " " FORCE)
set(OPTION_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(OPTION_BUILD_TEST OFF CACHE BOOL " " FORCE)
set(OPTION_BUILD_TOOLS OFF CACHE BOOL " " FORCE)

include_directories(${glbinding_SOURCE_DIR}/include)
include_directories(src)

# Find local OpenGL lib
find_package(OpenGL REQUIRED)

# Default flags
include(${CMAKE_SOURCE_DIR}/../flags.cmake)

# Add spine-cpp
add_subdirectory(${CMAKE_SOURCE_DIR}/../spine-cpp ${CMAKE_BINARY_DIR}/spine-cpp-build)

# 添加Dear ImGui
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/imgui)
file(GLOB IMGUI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/*.cpp")

# spine-glfw library
add_library(spine-glfw STATIC src/spine-glfw.cpp src/spine-glfw.h src/stb_image.h)
target_link_libraries(spine-glfw PRIVATE glbinding::glbinding)
target_link_libraries(spine-glfw LINK_PUBLIC spine-cpp-lite)

# spine-eto Example
add_executable(spine-glfw-example-eto example/main.cpp)
target_link_libraries(spine-glfw-example-eto PRIVATE glfw)
target_link_libraries(spine-glfw-example-eto PRIVATE OpenGL::GL)
target_link_libraries(spine-glfw-example-eto LINK_PUBLIC spine-glfw)
target_link_libraries(spine-glfw-example-eto PRIVATE glbinding::glbinding)
set_property(TARGET spine-glfw-example-eto PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/spine-glfw")

# Menu-test Example
add_executable(menu-test example/menu-test.cpp)
include_directories(src/imgui src/imgui/backends)

# 添加Dear ImGui的源文件，仅包含GLFW和OpenGL相关的后端实现文件
file(GLOB IMGUI_SOURCES src/imgui/*.cpp)
file(GLOB IMGUI_BACKENDS_GLFWSOURCES src/imgui/backends/imgui_impl_glfw.cpp src/imgui/backends/imgui_impl_opengl3.cpp)

target_sources(menu-test PRIVATE ${IMGUI_SOURCES})
target_sources(menu-test PRIVATE ${IMGUI_BACKENDS_GLFWSOURCES})

target_link_libraries(menu-test PRIVATE glfw)
target_link_libraries(menu-test PRIVATE dwmapi)
target_link_libraries(menu-test PRIVATE OpenGL::GL)
target_link_libraries(menu-test LINK_PUBLIC spine-glfw)
target_link_libraries(menu-test PRIVATE glbinding::glbinding)


# copy data to build directory
add_custom_command(TARGET spine-glfw-example PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_LIST_DIR}/data $<TARGET_FILE_DIR:spine-glfw-example>/data)
