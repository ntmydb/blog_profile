#include "FFmpegException.h"

extern "C" {
    #include <libavutil/error.h>
}

namespace Video2Gif {
    FFmpegException::FFmpegException(const std::string& message,int errorCode) 
        : message_(message), errorCode_(errorCode) {
        if (errorCode_ != 0) {
            message_ += ": " + getFFmpegErrorString(errorCode);
        }
    }
    
    const char* FFmpegException::what() const noexcept {
        return message_.c_str();
    }

    int FFmpegException::getErrorCode() const noexcept {
        return errorCode_;
    }
    
    std::string FFmpegException::getFFmpegErrorString(int errorCode) {
        char errBuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(errorCode,errBuf,AV_ERROR_MAX_STRING_SIZE);
        return std::string(errBuf);
    }
}