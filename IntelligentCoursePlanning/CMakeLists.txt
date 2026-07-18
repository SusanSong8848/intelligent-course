cmake_minimum_required(VERSION 3.16)
project(IntelligentCoursePlanning VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# 源文件收集
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/*.h
)

# 可执行文件
add_executable(course_planner ${SOURCES})

# 包含目录
target_include_directories(course_planner PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# 将data目录复制到构建输出目录（方便运行时读取数据）
add_custom_command(TARGET course_planner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:course_planner>/data
    COMMENT "Copying data directory to build output..."
)