#include "VideoToGifConverter.h"

using namespace Video2Gif;

int main() {
    VideoToGifConverter converter;
    
    ConversionConfig config;
    config.width = 480;
    config.fps = 10;
    
    bool success = converter.convert("input.mp4", "output.gif", config);
    
    return success ? 0 : 1;
}