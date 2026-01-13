#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>
#include "VideoToGifConverter.h"
#include "Config.h"

using namespace Video2Gif;

void printUsage(const char* programeName) {
    std::cout << "Video to GIF Converter v1.0.0\n" << std::endl;
    std::cout << "Usage: " << programeName << " [OPTIONS] <input_file> <output_file>" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  -w, --width <width>        Output width (0 = keep original)" << std::endl;
    std::cout << "  -h, --height <height>      Output height (0 = keep original)" << std::endl;
    std::cout << "  -f, --fps <fps>            Target frame rate (default: 10)" << std::endl;
    std::cout << "  -s, --start <seconds>      Start time in seconds (default: 0)" << std::endl;
    std::cout << "  -d, --duration <seconds>   Duration in seconds (0 = entire video)" << std::endl;
    std::cout << "  -q, --quality <1-100>      Quality (default: 100)" << std::endl;
    std::cout << "  -i, --info                 Display video information only" << std::endl;
    std::cout << "  --no-aspect                Don't maintain aspect ratio" << std::endl;
    std::cout << "  --help                     Display this help message" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  " << programeName << " video.mp4 output.gif" << std::endl;
    std::cout << "  " << programeName << " -w 480 -f 15 video.mp4 output.gif" << std::endl;
    std::cout << "  " << programeName << " -s 10 -d 5 video.mp4 output.gif" << std::endl;
    std::cout << "  " << programeName << " -i video.mp4" << std::endl;
}

void printProgressBar(int percentage) {
    const int barWidth = 50;
    std::cout << "\r[";
    
    int pos = barWidth * percentage / 100;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    
    std::cout << "] " << std::setw(3) << percentage << "%" << std::flush;
    
    if (percentage >= 100) {
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    // 解析命令行参数
    ConversionConfig config;
    std::string inputFile;
    std::string outputFile;
    bool showInfo = false;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "-i" || arg == "--info") {
            showInfo = true;
        }
        else if (arg == "--no-aspect") {
            config.maintainAspectRatio = false;
        }
        else if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
            config.width = std::stoi(argv[++i]);
        }
        else if ((arg == "-h" || arg == "--height") && i + 1 < argc) {
            config.height = std::stoi(argv[++i]);
        }
        else if ((arg == "-f" || arg == "--fps") && i + 1 < argc) {
            config.fps = std::stoi(argv[++i]);
        }
        else if ((arg == "-s" || arg == "--start") && i + 1 < argc) {
            config.startTime = std::stod(argv[++i]);
        }
        else if ((arg == "-d" || arg == "--duration") && i + 1 < argc) {
            config.duration = std::stod(argv[++i]);
        }
        else if ((arg == "-q" || arg == "--quality") && i + 1 < argc) {
            config.quality = std::stoi(argv[++i]);
            if (config.quality < 1 || config.quality > 100) {
                std::cerr << "Error: Quality must be between 1 and 100" << std::endl;
                return 1;
            }
        }
        else if (arg[0] != '-') {
            if (inputFile.empty()) {
                inputFile = arg;
            } else if (outputFile.empty()) {
                outputFile = arg;
            }
        }
        else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // 验证参数
    if (inputFile.empty()) {
        std::cerr << "Error: Input file not specified" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // 创建转换器
    VideoToGifConverter converter;
    
    // 如果只是显示信息
    if (showInfo) {
        std::cout << "Getting video information..." << std::endl;
        std::string info = converter.getVideoInfo(inputFile);
        std::cout << "\n" << info << std::endl;
        return 0;
    }
    
    // 检查输出文件
    if (outputFile.empty()) {
        std::cerr << "Error: Output file not specified" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // 显示转换配置
    std::cout << "Video to GIF Conversion" << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << "Input file:  " << inputFile << std::endl;
    std::cout << "Output file: " << outputFile << std::endl;
    
    if (config.width > 0 || config.height > 0) {
        std::cout << "Output size: ";
        if (config.width > 0) std::cout << config.width << "x";
        else std::cout << "auto x ";
        if (config.height > 0) std::cout << config.height;
        else std::cout << "auto";
        std::cout << (config.maintainAspectRatio ? " (maintaining aspect ratio)" : "") << std::endl;
    } else {
        std::cout << "Output size: original" << std::endl;
    }
    
    std::cout << "Frame rate:  " << config.fps << " fps" << std::endl;
    
    if (config.startTime > 0) {
        std::cout << "Start time:  " << config.startTime << " seconds" << std::endl;
    }
    
    if (config.duration > 0) {
        std::cout << "Duration:    " << config.duration << " seconds" << std::endl;
    }
    
    std::cout << "\nStarting conversion..." << std::endl;
    
    // 设置进度回调
    converter.setProgressCallback([](int progress) {
        printProgressBar(progress);
    });
    
    // 执行转换
    bool success = converter.convert(inputFile, outputFile, config);
    
    if (success) {
        std::cout << "\n✓ Conversion completed successfully!" << std::endl;
        std::cout << "Output saved to: " << outputFile << std::endl;
        return 0;
    } else {
        std::cerr << "\n✗ Conversion failed!" << std::endl;
        std::cerr << "Error: " << converter.getLastError() << std::endl;
        return 1;
    }
}