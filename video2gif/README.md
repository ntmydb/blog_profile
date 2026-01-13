# Video to GIF Converter

一个基于 FFmpeg 的高性能视频转 GIF 工具，使用 C++ 编写。

## 功能特性

- ✅ 支持多种视频格式（MP4, AVI, MOV, MKV 等）
- ✅ 自定义输出尺寸和帧率
- ✅ 时间范围选择（指定开始时间和持续时间）
- ✅ 自动保持宽高比
- ✅ 实时进度显示
- ✅ 视频信息查看
- ✅ 批量转换支持
- ✅ 高质量输出
- ✅ 跨平台支持（Linux, macOS, Windows）

## 系统要求

### 依赖库

- FFmpeg 4.0 或更高版本
  - libavformat
  - libavcodec
  - libavutil
  - libswscale
- CMake 3.10 或更高版本
- C++17 编译器

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    pkg-config
```

### CentOS/RHEL

```bash
sudo yum install -y \
    gcc-c++ \
    cmake \
    ffmpeg-devel
```

### macOS

```bash
brew install cmake ffmpeg pkg-config
```

### Windows

1. 下载并安装 [FFmpeg](https://ffmpeg.org/download.html)
2. 安装 [Visual Studio](https://visualstudio.microsoft.com/) 或 MinGW
3. 安装 [CMake](https://cmake.org/download/)

## 编译安装

### 快速开始

```bash
# 克隆或下载项目
cd video2gif

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
cmake --build .

# 安装（可选）
sudo cmake --install .
```

### 指定 FFmpeg 路径（如果需要）

```bash
cmake -DAVCODEC_INCLUDE_DIR=/path/to/ffmpeg/include \
      -DAVCODEC_LIBRARY=/path/to/ffmpeg/lib/libavcodec.so \
      ..
```

## 使用方法

### 基本用法

```bash
# 最简单的转换
./video2gif input.mp4 output.gif

# 指定输出尺寸
./video2gif -w 480 -h 270 input.mp4 output.gif

# 指定帧率
./video2gif -f 15 input.mp4 output.gif

# 转换视频片段（从10秒开始，持续5秒）
./video2gif -s 10 -d 5 input.mp4 output.gif

# 组合使用
./video2gif -w 640 -f 12 -s 5 -d 10 input.mp4 output.gif
```

### 命令行选项

```
Usage: video2gif [OPTIONS] <input_file> <output_file>

Options:
  -w, --width <width>        输出宽度（0 = 保持原始）
  -h, --height <height>      输出高度（0 = 保持原始）
  -f, --fps <fps>            目标帧率（默认: 10）
  -s, --start <seconds>      开始时间（秒，默认: 0）
  -d, --duration <seconds>   持续时间（秒，0 = 整个视频）
  -q, --quality <1-100>      质量（默认: 100）
  -i, --info                 仅显示视频信息
  --no-aspect                不保持宽高比
  --help                     显示帮助信息
```

### 可视化界面 (GUI)

本项目提供了一个简单的图形用户界面，方便用户操作。

#### 运行 GUI

确保已安装 Python 3 和 tkinter（通常随 Python 安装）。

```bash
# 运行 GUI
python3 gui/video2gif_gui.py
```

GUI 界面会自动查找编译好的 `video2gif` 可执行文件。如果未找到，请先按照[编译安装](#编译安装)步骤编译项目。

### 查看视频信息

```bash
./video2gif -i input.mp4
```

输出示例：
```
File: input.mp4
Duration: 120 seconds
Bit rate: 5000 kbps

Video Stream #0:
  Codec: h264
  Resolution: 1920x1080
  Frame rate: 30.000000 fps
  Bit rate: 4800 kbps
```

## 代码示例

### C++ API 使用

#### 示例 1: 基本转换

```cpp
#include "VideoToGifConverter.h"

using namespace Video2Gif;

int main() {
    VideoToGifConverter converter;
    
    bool success = converter.convert("input.mp4", "output.gif");
    
    if (success) {
        std::cout << "Conversion completed!" << std::endl;
    }
    
    return 0;
}
```

#### 示例 2: 自定义配置

```cpp
#include "VideoToGifConverter.h"
#include "Config.h"

using namespace Video2Gif;

int main() {
    VideoToGifConverter converter;
    
    ConversionConfig config;
    config.width = 480;
    config.height = 270;
    config.fps = 15;
    config.startTime = 10.0;
    config.duration = 5.0;
    
    bool success = converter.convert("input.mp4", "output.gif", config);
    
    return success ? 0 : 1;
}
```

#### 示例 3: 带进度回调

```cpp
#include "VideoToGifConverter.h"

using namespace Video2Gif;

int main() {
    VideoToGifConverter converter;
    
    // 设置进度回调
    converter.setProgressCallback([](int progress) {
        std::cout << "Progress: " << progress << "%" << std::endl;
    });
    
    ConversionConfig config;
    config.width = 640;
    config.fps = 10;
    
    converter.convert("input.mp4", "output.gif", config);
    
    return 0;
}
```

#### 示例 4: 批量转换

```cpp
#include "VideoToGifConverter.h"
#include <vector>

using namespace Video2Gif;

int main() {
    std::vector<std::pair<std::string, std::string>> files = {
        {"video1.mp4", "output1.gif"},
        {"video2.mp4", "output2.gif"},
        {"video3.mp4", "output3.gif"}
    };
    
    ConversionConfig config;
    config.width = 480;
    config.fps = 10;
    
    for (const auto& [input, output] : files) {
        VideoToGifConverter converter;
        
        std::cout << "Converting: " << input << std::endl;
        
        if (converter.convert(input, output, config)) {
            std::cout << "✓ Success: " << output << std::endl;
        } else {
            std::cerr << "✗ Failed: " << converter.getLastError() << std::endl;
        }
    }
    
    return 0;
}
```

#### 示例 5: 错误处理

```cpp
#include "VideoToGifConverter.h"
#include "FFmpegException.h"

using namespace Video2Gif;

int main() {
    try {
        VideoToGifConverter converter;
        
        ConversionConfig config;
        config.width = 800;
        config.fps = 20;
        
        if (!converter.convert("input.mp4", "output.gif", config)) {
            std::cerr << "Error: " << converter.getLastError() << std::endl;
            return 1;
        }
        
    } catch (const FFmpegException& e) {
        std::cerr << "FFmpeg error: " << e.what() << std::endl;
        std::cerr << "Error code: " << e.getErrorCode() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

## 性能优化建议

### 1. 降低输出尺寸

```bash
# 较小的尺寸会大幅减少文件大小和转换时间
./video2gif -w 480 input.mp4 output.gif
```

### 2. 调整帧率

```bash
# 较低的帧率可以减少文件大小
./video2gif -f 10 input.mp4 output.gif  # 推荐
./video2gif -f 15 input.mp4 output.gif  # 较流畅
./video2gif -f 5 input.mp4 output.gif   # 最小文件
```

### 3. 转换视频片段

```bash
# 只转换需要的部分
./video2gif -s 0 -d 3 input.mp4 output.gif  # 只转换前3秒
```

### 4. 推荐配置

对于大多数场景，推荐以下配置：

```bash
# 标准质量（适合分享）
./video2gif -w 480 -f 10 input.mp4 output.gif

# 高质量（适合演示）
./video2gif -w 720 -f 15 input.mp4 output.gif

# 小文件（适合聊天）
./video2gif -w 320 -f 8 input.mp4 output.gif
```

## 常见问题

### Q: 生成的 GIF 文件太大怎么办？

A: 尝试以下方法：
- 减小输出尺寸（使用 `-w` 和 `-h`）
- 降低帧率（使用 `-f`，推荐 8-12）
- 只转换需要的部分（使用 `-s` 和 `-d`）

### Q: 如何保持视频的宽高比？

A: 只指定宽度或高度之一，程序会自动计算另一个维度：

```bash
./video2gif -w 480 input.mp4 output.gif
```

### Q: 转换速度慢怎么办？

A: 
- 使用较低的输出分辨率
- 减少帧率
- 确保 FFmpeg 使用了硬件加速
- 使用 SSD 存储

### Q: 支持哪些视频格式？

A: 支持所有 FFmpeg 支持的格式，包括但不限于：
- MP4
- AVI
- MOV
- MKV
- WMV
- FLV
- WEBM

### Q: 编译时找不到 FFmpeg 库？

A: 
1. 确认已安装 FFmpeg 开发库
2. 使用 `pkg-config --libs libavcodec` 检查
3. 手动指定路径：
```bash
cmake -DAVCODEC_INCLUDE_DIR=/usr/local/include \
      -DAVCODEC_LIBRARY=/usr/local/lib/libavcodec.so \
      ..
```

### Q: Windows 下如何编译？

A: 使用 Visual Studio 或 MinGW，并指定 FFmpeg 路径（FFmpeg 根目录，包含 include 和 lib）：

```bash
# Visual Studio
mkdir build && cd build
cmake -G "Visual Studio 16 2019" -DFFMPEG_DIR="C:/path/to/ffmpeg" ..
cmake --build . --config Release

# MinGW
mkdir build && cd build
cmake -G "MinGW Makefiles" -DFFMPEG_DIR="C:/path/to/ffmpeg" ..
mingw32-make
```

## 项目结构

```
video2gif/
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 项目说明文档
├── include/                # 头文件目录
│   ├── VideoToGifConverter.h    # 主转换器类
│   ├── FFmpegException.h        # 异常处理类
│   └── Config.h                 # 配置结构
├── src/                    # 源文件目录
│   ├── VideoToGifConverter.cpp  # 转换器实现
│   ├── FFmpegException.cpp      # 异常实现
│   └── main.cpp                 # 命令行工具
├── examples/               # 示例代码
│   └── example_usage.cpp        # API 使用示例
├── gui/                    # 图形界面
│   └── video2gif_gui.py         # Python GUI 脚本
└── build/                  # 构建目录（gitignore）
```

## 技术细节

### 转换流程

1. **打开输入文件**：使用 `avformat_open_input` 打开视频文件
2. **查找视频流**：定位视频流并获取编解码器信息
3. **初始化解码器**：创建并配置视频解码器
4. **初始化编码器**：创建 GIF 编码器（使用 PAL8 像素格式）
5. **帧转换**：使用 `swscale` 进行尺寸调整和像素格式转换
6. **帧率控制**：按目标帧率抽取帧
7. **编码输出**：将处理后的帧编码为 GIF 格式
8. **写入文件**：保存最终的 GIF 文件

### 像素格式

- 输入：支持所有 FFmpeg 支持的格式
- 中间格式：RGB8
- 输出格式：PAL8（GIF 调色板格式）

### 内存管理

使用 RAII 和智能指针确保资源正确释放：
- `unique_ptr` 用于 PIMPL 实现
- 自动析构函数清理 FFmpeg 资源

## 许可证

本项目采用 MIT 许可证。

## 贡献

欢迎提交 Issue 和 Pull Request！

### 开发指南

1. Fork 本项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 更新日志

### v1.0.0 (2024-01-XX)
- ✨ 初始版本发布
- ✅ 基本视频转 GIF 功能
- ✅ 自定义尺寸和帧率
- ✅ 时间范围选择
- ✅ 进度显示
- ✅ 命令行工具

## 相关项目

- [FFmpeg](https://ffmpeg.org/) - 多媒体处理框架
- [gifski](https://gif.ski/) - 高质量 GIF 编码器
- [gifsicle](https://www.lcdf.org/gifsicle/) - GIF 优化工具

## 联系方式

- 问题反馈：[GitHub Issues](https://github.com/your-username/video2gif/issues)
- 邮件：your-email@example.com

## 致谢

感谢 FFmpeg 项目提供