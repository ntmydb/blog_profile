#ifndef CONFIG_H
#define CONFIG_H

namespace Video2Gif {
    struct ConversionConfig {
        int width = 0;
        int height = 0;
        int fps = 10;
        double startTime = 0.0;
        double duration = 0.0;
        int quality = 100;
        bool maintainAspectRatio = true;

        ConversionConfig() = default;

        ConversionConfig(int w,int h,int f = 10) : width(w), height(h), fps(f) {}
    };
}

#endif