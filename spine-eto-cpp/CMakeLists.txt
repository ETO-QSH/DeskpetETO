cmake_minimum_required(VERSION 3.28)
project(spine_eto_cpp)
set(CMAKE_CXX_STANDARD 26)

# 指定 SFML 的路径
set(SFML_DIR "${CMAKE_SOURCE_DIR}/dependencies/SFML-2.6.2")

# 添加 SFML 的包含目录和库目录
include_directories(${SFML_DIR}/include)
link_directories(${SFML_DIR}/lib)

# 找到 SFML 的 DLL 文件
set(SFML_DLL_DIR "${SFML_DIR}/bin")
file(GLOB SFML_DLLS "${SFML_DLL_DIR}/*.dll")

# 设置 Spine 路径
set(SPINE_CPP_DIR "${CMAKE_SOURCE_DIR}/spine-cpp/spine-cpp")
set(SPINE_SFML_DIR "${CMAKE_SOURCE_DIR}/spine-sfml")

# 添加spine-cpp和spine-sfml源文件
file(GLOB_RECURSE SPINE_CPP_SOURCES "${SPINE_CPP_DIR}/src/spine/*.cpp")
file(GLOB_RECURSE SPINE_SFML_SOURCES "${SPINE_SFML_DIR}/src/spine/*.cpp")
include_directories(${SPINE_CPP_DIR}/include ${SPINE_SFML_DIR}/src)

# 添加 json.hpp 头文件目录
include_directories(${CMAKE_SOURCE_DIR}/dependencies)

# 添加spine-eto源文件
file(GLOB_RECURSE SPINE_ETO_SOURCES "${CMAKE_SOURCE_DIR}/spine-eto/*.cpp")

# 添加可执行文件，并包含spine的源文件
add_executable(spine_eto_cpp main.cpp ${SPINE_CPP_SOURCES} ${SPINE_SFML_SOURCES} ${SPINE_ETO_SOURCES})

# 链接 SFML 库
target_link_libraries(spine_eto_cpp PRIVATE sfml-graphics sfml-window sfml-system)

# 将 SFML 的 DLL 文件复制到构建目录
add_custom_command(TARGET spine_eto_cpp POST_BUILD COMMAND ${CMAKE_COMMAND}
        -E copy_directory "${SFML_DLL_DIR}" "$<TARGET_FILE_DIR:spine_eto_cpp>")
