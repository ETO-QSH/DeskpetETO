cmake_minimum_required(VERSION 3.26)
project(keyboard_hook)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 指定 SFML 的路径
set(SFML_DIR "${CMAKE_SOURCE_DIR}/dependencies/SFML-2.6.2")

# 添加 SFML 的包含目录和库目录
include_directories(${SFML_DIR}/include)
link_directories(${SFML_DIR}/lib)

# 找到 SFML 的 DLL 文件
set(SFML_DLL_DIR "${SFML_DIR}/bin")
file(GLOB SFML_DLLS "${SFML_DLL_DIR}/*.dll")

add_executable(keyboard_hook main.cpp vk_code_2_string.cpp keyboard_hook_tool.cpp)

# 链接 SFML 库
target_link_libraries(keyboard_hook PRIVATE sfml-graphics sfml-window sfml-system)

# 链接 Imm32 库
target_link_libraries(keyboard_hook PRIVATE Imm32)

# 将 SFML 的 DLL 文件复制到构建目录
add_custom_command(TARGET keyboard_hook POST_BUILD COMMAND ${CMAKE_COMMAND}
        -E copy_directory "${SFML_DLL_DIR}" "$<TARGET_FILE_DIR:keyboard_hook>")
