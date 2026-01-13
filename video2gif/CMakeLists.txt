cmake_minimum_required(VERSION 3.10)
project(Video2Gif VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含项目头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 平台相关配置
if(WIN32)
    message(STATUS "Configuring for Windows...")
    
    # 检查是否定义了FFMPEG_DIR
    if(NOT DEFINED FFMPEG_DIR)
        message(FATAL_ERROR "FFMPEG_DIR not set. Please run cmake with -DFFMPEG_DIR=\"path/to/ffmpeg\"")
    endif()
    
    message(STATUS "Using FFmpeg at: ${FFMPEG_DIR}")
    
    # 设置头文件和库文件路径
    include_directories(${FFMPEG_DIR}/include)
    link_directories(${FFMPEG_DIR}/lib)
    
    # Windows下通常链接这些库
    set(FFMPEG_LIBRARIES avcodec avformat avutil swscale)
    
else()
    message(STATUS "Configuring for macOS/Linux...")
    
    # 使用pkg-config查找FFmpeg库
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavcodec
        libavformat
        libavutil
        libswscale
    )
    
    set(FFMPEG_LIBRARIES PkgConfig::FFMPEG)
endif()

# 源文件
set(SOURCES
    src/VideoToGifConverter.cpp
    src/FFmpegException.cpp
    src/main.cpp
)

# 主程序
add_executable(video2gif ${SOURCES})
target_link_libraries(video2gif PRIVATE ${FFMPEG_LIBRARIES})

# 设置输出目录
set_target_properties(video2gif PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 安装规则
install(TARGETS video2gif DESTINATION bin)

# 示例程序
add_executable(video2gif_example examples/example_usage.cpp src/VideoToGifConverter.cpp src/FFmpegException.cpp)
target_link_libraries(video2gif_example PRIVATE ${FFMPEG_LIBRARIES})

set_target_properties(video2gif_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)