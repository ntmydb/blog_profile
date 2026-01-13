#ifndef VIDEO_TO_GIF_CONVERTER_H
#define VIDEO_TO_GIF_CONVERTER_H

#include <string>
#include <memory>
#include <functional>
#include "Config.h"

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
}

namespace Video2Gif {
    class VideoToGifConverter {
        public:
            VideoToGifConverter();
            ~VideoToGifConverter();

            //禁止拷贝s
            VideoToGifConverter(const VideoToGifConverter&) = delete;
            VideoToGifConverter& operator=(const VideoToGifConverter&) = delete;

            //允许移动
            VideoToGifConverter(VideoToGifConverter&&) noexcept;
            VideoToGifConverter& operator=(VideoToGifConverter&&) noexcept;

            //转换视频为Gif
            bool convert(const std::string& inputFile,
                const std::string& outputFile,
                const ConversionConfig& config = ConversionConfig());

            //获取视频信息
            std::string getVideoInfo(const std::string& inputFile);

            //设置进度回调函数
            void setProgressCallback(std::function<void(int)> callback);
            
            //获取最后一次错误信息
            std::string getLastError() const;
        private:
            class Impl;
            std::unique_ptr<Impl> impl_;

            bool openInputFile(const std::string& filename);
            bool openOutputFile(const std::string& filename);
            bool initSwsContext();
            bool processFrames(double startTime,double duration);
            bool encodeFrame(AVFrame* frame);
            void cleanup();
            void calculateOutputSize(const ConversionConfig& config);
    };
}

#endif