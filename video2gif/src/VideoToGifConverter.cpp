#include "VideoToGifConverter.h"
#include "FFmpegException.h"
#include <iostream>
#include <functional>

extern "C" {
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>
}

namespace Video2Gif {
    class VideoToGifConverter::Impl {
        public:
            //输入相关
            AVFormatContext* inputFormatCtx = nullptr;
            AVCodecContext* inputCodecCtx = nullptr;
            int videoStreamIndex = -1;

            //输出相关
            AVFormatContext* outputFormatCtx = nullptr;
            AVCodecContext* outputCodecCtx = nullptr;
            AVStream* outputStream = nullptr;

            //转换相关
            SwsContext* swsContext = nullptr;

            //参数
            int targetWidth = 0;
            int targetHeight = 0;
            int targetFps = 0;

            //回调和错误信息
            std::function<void(int)> progressCallback = nullptr;
            std::string lastError;

            ~Impl() {
                cleanup();
            }

            void cleanup() {
                if (swsContext) {
                    sws_freeContext(swsContext);
                    swsContext = nullptr;
                }

                if (inputCodecCtx) {
                    avcodec_free_context(&inputCodecCtx);
                }
                if (outputCodecCtx) {
                    avcodec_free_context(&outputCodecCtx);
                }

                if (inputFormatCtx) {
                    avformat_close_input(&inputFormatCtx);
                }

                if (outputFormatCtx) {
                    if (outputFormatCtx->pb && !(outputFormatCtx->oformat->flags & AVFMT_NOFILE)) {
                        avio_closep(&outputFormatCtx->pb);
                    }
                    avformat_free_context(outputFormatCtx);
                    outputFormatCtx = nullptr;
                }
            }
    };
    VideoToGifConverter::VideoToGifConverter() : impl_(std::make_unique<Impl>()) {}
    VideoToGifConverter::~VideoToGifConverter() = default;
    VideoToGifConverter::VideoToGifConverter(VideoToGifConverter&&) noexcept = default;
    VideoToGifConverter& VideoToGifConverter::operator=(VideoToGifConverter&&) noexcept = default;

    bool VideoToGifConverter::convert(const std::string& inputFile,
        const std::string& outputFile,
        const ConversionConfig& config) {
        try {
            impl_->targetFps = config.fps;
            //打开输入文件
            if (!openInputFile(inputFile)) {
                return false;
            }
            //计算输出尺寸
            calculateOutputSize(config);
            //打开输出文件
            if (!openOutputFile(outputFile)) {
                return false;
            }
            //初始化转换上下文
            if (!initSwsContext()) {
                return false;
            }
            //执行转换
            if (!processFrames(config.startTime,config.duration)) {
                return false;
            }
            //写入文件层
            av_write_trailer(impl_->outputFormatCtx);

            std::cout << "Conversion completed successfully." << std::endl;
            return true;
        } catch (const FFmpegException& e) {
            impl_->lastError = e.what();
            std::cout << "FFmpeg error: " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            impl_->lastError = e.what();
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    void VideoToGifConverter::calculateOutputSize(const ConversionConfig& config) {
        int inputWidth = impl_->inputCodecCtx->width;
        int inputHeight = impl_->inputCodecCtx->height;

        if (config.width > 0 && config.height > 0) {
            impl_->targetWidth = config.width;
            impl_->targetHeight = config.height;

            if (config.maintainAspectRatio) {
                float aspectRatio = (float)inputWidth / inputHeight;
                float targetAspectRatio = (float)config.width / config.height;
                if (aspectRatio > targetAspectRatio) {
                    impl_->targetWidth = config.width;
                    impl_->targetHeight = (int)(config.width / aspectRatio);
                } else {
                    impl_->targetHeight = config.height;
                    impl_->targetWidth = (int)(config.height * aspectRatio);
                }

                //确保尺寸是偶数
                impl_->targetWidth = (impl_->targetWidth / 2) * 2;
                impl_->targetHeight = (impl_->targetHeight / 2) * 2;
            }
        } else if (config.width > 0) {
            impl_->targetWidth = config.width;
            float aspectRatio = (float)inputWidth / inputHeight;
            impl_->targetHeight = (int)(config.width / aspectRatio);
            impl_->targetHeight = (impl_->targetHeight / 2) * 2;
        } else if (config.height > 0) {
            impl_->targetHeight = config.height;
            float aspectRatio = (float)inputWidth / inputHeight;
            impl_->targetWidth = (int)(config.height * aspectRatio);
            impl_->targetWidth = (impl_->targetWidth / 2) * 2;
        } else {
            impl_->targetWidth = inputWidth;
            impl_->targetHeight = inputHeight;
        }
    }

    bool VideoToGifConverter::openInputFile(const std::string& filename) {
        int ret;

        //打开输入文件
        ret = avformat_open_input(&impl_->inputFormatCtx, filename.c_str(), nullptr, nullptr);
        if (ret < 0) {
            throw FFmpegException("Failed to open input file: " + filename,ret);
        }
        //获取流信息
        ret = avformat_find_stream_info(impl_->inputFormatCtx, nullptr);
        if (ret < 0) {
            throw FFmpegException("Failed to find stream info for file: " + filename,ret);
        }

        //找到视频流
        for (unsigned int i = 0;i < impl_->inputFormatCtx->nb_streams; i++) {
            if (impl_->inputFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                impl_->videoStreamIndex = i;
                break;
            }
        }
        if (impl_->videoStreamIndex == -1) {
            throw FFmpegException("Could not find video stream");
        }
        //获取解码器
        AVCodecParameters* codecParams = impl_->inputFormatCtx->streams[impl_->videoStreamIndex]->codecpar;
        const AVCodec* decoder = avcodec_find_decoder(codecParams->codec_id);
        if (!decoder) {
            throw FFmpegException("Failed to find decoder for video stream");
        }
        //创建解码器上下文
        impl_->inputCodecCtx = avcodec_alloc_context3(decoder);
        if (!impl_->inputCodecCtx) {
            throw FFmpegException("Failed to allocate input codec context");
        }
        ret = avcodec_parameters_to_context(impl_->inputCodecCtx, codecParams);
        if (ret < 0) {
            throw FFmpegException("Failed to copy codec parameters to input codec context",ret);
        }
        //打开解码器
        ret = avcodec_open2(impl_->inputCodecCtx, decoder, nullptr);
        if (ret < 0) {
            throw FFmpegException("Failed to open input codec context",ret);
        }
        return true;
    }

    bool VideoToGifConverter::openOutputFile(const std::string& filename) {
        int ret;
        //分配输出格式上下文
        ret = avformat_alloc_output_context2(&impl_->outputFormatCtx, nullptr, "gif", filename.c_str());
        if (ret < 0) {
            throw FFmpegException("Failed to allocate output format context",ret);
        }

        //查找GIF编码器
        const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_GIF);
        if (!encoder) {
            throw FFmpegException("Failed to find GIF encoder");
        }

        //创建输出流
        impl_->outputStream = avformat_new_stream(impl_->outputFormatCtx, nullptr);
        if (!impl_->outputStream) {
            throw FFmpegException("Failed to create output stream");
        }

        //创建编码器上下文
        impl_->outputCodecCtx = avcodec_alloc_context3(encoder);
        if (!impl_->outputCodecCtx) {
            throw FFmpegException("Failed to allocate output codec context");
        }
        impl_->outputCodecCtx->width = impl_->targetWidth;
        impl_->outputCodecCtx->height = impl_->targetHeight;
        impl_->outputCodecCtx->time_base = AVRational{1,impl_->targetFps};
        impl_->outputCodecCtx->framerate = AVRational{impl_->targetFps,1};
        impl_->outputCodecCtx->pix_fmt = AV_PIX_FMT_PAL8; //GIF使用调色板模式

        AVDictionary* opts = nullptr;
        av_dict_set(&opts,"loop","0",0);
        av_dict_set_int(&opts,"gifflags",0,0);

        if (impl_->outputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
            impl_->outputCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        //打开编码器
        ret = avcodec_open2(impl_->outputCodecCtx,encoder,nullptr);
        if (ret < 0) {
            throw FFmpegException("Failed to open output encoder context",ret);
        }

        //复制编码器参数到流
        ret = avcodec_parameters_from_context(impl_->outputStream->codecpar, impl_->outputCodecCtx);
        if (ret < 0) {
            throw FFmpegException("Failed to copy codec parameters to output stream",ret);
        }
        impl_->outputStream->time_base = impl_->outputCodecCtx->time_base;

        //打开输出文件
        if (!(impl_->outputFormatCtx->oformat->flags & AVFMT_NOFILE)) {
            ret = avio_open(&impl_->outputFormatCtx->pb, filename.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0) {
                throw FFmpegException("Failed to open output file ",ret);
            }
        }
        //写入文件头
        ret = avformat_write_header(impl_->outputFormatCtx, nullptr);
        if (ret < 0) {
            throw FFmpegException("Failed to write header to output file ",ret);
        }

        return true;
    }

    bool VideoToGifConverter::initSwsContext() {
        impl_->swsContext = sws_getContext(
            impl_->inputCodecCtx->width,
            impl_->inputCodecCtx->height,
            impl_->inputCodecCtx->pix_fmt,
            impl_->targetWidth,
            impl_->targetHeight,
            AV_PIX_FMT_RGB8,
            SWS_BILINEAR,
            nullptr,nullptr,nullptr
        );
        if (!impl_->swsContext) {
            throw FFmpegException("Failed to allocate SWS context");
        }
        return true;
    }

    bool VideoToGifConverter::processFrames(double startTime, double duration) {
        AVPacket* packet = av_packet_alloc();
        AVFrame* inputFrame = av_frame_alloc();
        AVFrame* outputFrame = av_frame_alloc();

        if (!packet || !inputFrame || !outputFrame) {
            throw FFmpegException("Failed to allocate frame");
        }

        //分配输出帧数据
        outputFrame->format = AV_PIX_FMT_PAL8;
        outputFrame->width = impl_->targetWidth;
        outputFrame->height = impl_->targetHeight;
        int ret = av_frame_get_buffer(outputFrame, 0);
        if (ret < 0) {
            throw FFmpegException("Could not allocate frame buffer",ret);
        }

        //如果指定了起始时间，进行seek
        if (startTime > 0) {
            int64_t seekTarget = startTime * AV_TIME_BASE;
            ret = av_seek_frame(impl_->inputFormatCtx, -1 , seekTarget, AVSEEK_FLAG_BACKWARD);
            if (ret < 0) {
                throw FFmpegException("Failed to seek to start time",ret);
            }
            avcodec_flush_buffers(impl_->inputCodecCtx);
        }
        
        int frameCount = 0;
        int outputFrameCount = 0;
        double inputFps = av_q2d(impl_->inputFormatCtx->streams[impl_->videoStreamIndex]->r_frame_rate);

        if (impl_->targetFps <= 0 ) {
            impl_->targetFps = 10;
        }

        int frameSkip = std::max(1, (int)(inputFps / impl_->targetFps));

        double startPts = -1;
        double endPts = -1;

        if (duration > 0) {
            endPts = startTime + duration;
        }

        //计算总帧数用于进度显示
        int64_t totalFrames = 0;
        if (duration > 0) {
            totalFrames = (int64_t)(duration * inputFps);
        } else {
            totalFrames = impl_->inputFormatCtx->streams[impl_->videoStreamIndex]->nb_frames;
            if (totalFrames == 0) {
                //如果nb_frames不可用，尝试估算
                int64_t durationUs = impl_->inputFormatCtx->duration;
                if (durationUs != AV_NOPTS_VALUE) {
                    totalFrames = (int64_t)((durationUs / (double)AV_TIME_BASE) * inputFps);
                }
            }
        }

        bool finished = false;
        while (!finished && av_read_frame(impl_->inputFormatCtx,packet) >= 0) {
            if (packet->stream_index == impl_->videoStreamIndex) {
                //发送packet到解码器
                ret = avcodec_send_packet(impl_->inputCodecCtx,packet);
                if (ret < 0) {
                    std::cerr << "Error sending packet to decoder: " << ret << std::endl;
                    av_packet_unref(packet);
                    continue;
                }

                while (avcodec_receive_frame(impl_->inputCodecCtx,inputFrame) >= 0) {
                    //计算当前帧时间
                    double framePts = inputFrame->pts * av_q2d(impl_->inputFormatCtx->streams[impl_->videoStreamIndex]->time_base);
                    if (startPts < 0) {
                        startPts = framePts;
                    }
                    //检查是否超过指定时长
                    if (endPts > 0 && framePts >= endPts) {
                        finished = true;
                        break;
                    }
                    //按帧率抽取帧
                    if (frameCount % frameSkip == 0) {
                        //确保帧可写
                        av_frame_make_writable(outputFrame);
                        
                        //生成3:3:2调色板
                        uint32_t* pal = (uint32_t*)outputFrame->data[1];
                        for (int i = 0; i < 256; i++) {
                            int r = (i >> 5) & 0x7;
                            int g = (i >> 2) & 0x7;
                            int b = i & 0x3;
                            r = (r * 255 + 3) / 7;
                            g = (g * 255 + 3) / 7;
                            b = (b * 255 + 1) / 3;
                            pal[i] = (0xFFU << 24) | (r << 16) | (g << 8) | b;
                        }

                        //转换帧格式与尺寸
                        sws_scale(impl_->swsContext,
                            inputFrame->data,
                            inputFrame->linesize,
                            0,
                            impl_->inputCodecCtx->height,
                            outputFrame->data,
                            outputFrame->linesize);
                        outputFrame->pts = outputFrameCount++;
                        //编码帧
                        if (!encodeFrame(outputFrame)) {
                            std::cerr << "Warning Failed to encoding frame: "  << std::endl;
                        }
                        //更新进度
                        if (impl_->progressCallback && totalFrames > 0) {
                            int progress = (int)((frameCount * 100) / totalFrames);
                            impl_->progressCallback(std::min(progress,99));
                        }
                        std::cout << "\rProgress frames: " << outputFrameCount << std::flush;
                    }
                    frameCount++;
                    av_frame_unref(inputFrame);
                }
            }
            av_packet_unref(packet); 
       }
       //刷新编码器
       encodeFrame(nullptr);
       std::cout << std::endl;
       //最终进度
       if (impl_->progressCallback) {
           impl_->progressCallback(100);
       }
       av_frame_free(&inputFrame);
       av_frame_free(&outputFrame);
       av_packet_free(&packet);

       return true;
    }

    bool VideoToGifConverter::encodeFrame(AVFrame* frame) {
        AVPacket* packet = av_packet_alloc();
        if (!packet) {
            return false;
        }

        int ret = avcodec_send_frame(impl_->outputCodecCtx,frame);
        if (ret < 0 && ret != AVERROR_EOF) {
            av_packet_free(&packet);
            return false;
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(impl_->outputCodecCtx,packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                av_packet_free(&packet);
                return false;
            }

            //重新计算时间戳
            av_packet_rescale_ts(packet,impl_->outputCodecCtx->time_base,impl_->outputStream->time_base);
            packet->stream_index = impl_->outputStream->index;

            //写入输出文件
            ret = av_interleaved_write_frame(impl_->outputFormatCtx,packet);
            av_packet_unref(packet);

            if (ret < 0) {
                av_packet_free(&packet);
                return false;
            }
        }
        av_packet_free(&packet);
        return true;
    }

    void VideoToGifConverter::cleanup() {
        impl_->cleanup();
    }

    std::string VideoToGifConverter::getVideoInfo(const std::string& inputFile) {
        AVFormatContext* formatCtx = nullptr;
        
        int ret = avformat_open_input(&formatCtx,inputFile.c_str(),nullptr,nullptr);
        if (ret < 0) {
            return "Error: Could not open file";
        }
        ret = avformat_find_stream_info(formatCtx,nullptr);
        if (ret < 0) {
            avformat_close_input(&formatCtx);
            return "Error: Could not find stream info";
        }

        std::string info;
        info += "File :" + inputFile + "\n";
        info += "Duration :" + std::to_string(formatCtx->duration / AV_TIME_BASE) + "seconds\n";
        info += "Bit rate :" + std::to_string(formatCtx->bit_rate / 1000) + "kbps\n";

        //查找视频流
        for (unsigned int i = 0;i < formatCtx->nb_streams; i++) {
            if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                AVCodecParameters* codecParams = formatCtx->streams[i]->codecpar;
                AVRational frameRate = formatCtx->streams[i]->r_frame_rate;
                info += "\nVideo stream #" + std::to_string(i) + ":\n";
                info += " Codec :" + std::string(avcodec_get_name(codecParams->codec_id)) + "\n";
                info += " Resolution :" + std::to_string(codecParams->width) + "x" + std::to_string(codecParams->height) + "\n";
                info += " Frame rate :" + std::to_string(av_q2d(frameRate)) + "fps\n";
                info += " Bit rate :" + std::to_string(codecParams->bit_rate / 1000) + "kbps\n";
                break;
            }
        }
        avformat_close_input(&formatCtx);
        return info;
    }

    void VideoToGifConverter::setProgressCallback(std::function<void(int)> callback) {
        impl_->progressCallback = callback;
    }

    std::string VideoToGifConverter::getLastError() const {
        return impl_->lastError;
    }
}