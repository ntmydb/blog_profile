#ifndef FFMPEG_EXCEPTION_H
#define FFMPEG_EXCEPTION_H

#include <exception>
#include <string>

namespace Video2Gif {
    class FFmpegException : public std::exception {
        private:
            std::string message_;
            int errorCode_;
        public:
            FFmpegException(const std::string& message,int errorCode = 0);
            virtual const char* what() const noexcept override;
            int getErrorCode() const noexcept;
            static std::string getFFmpegErrorString(int errorCode);
    };
}

#endif