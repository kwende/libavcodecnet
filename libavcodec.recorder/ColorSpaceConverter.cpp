#include "pch.h"
#include "ColorSpaceConverter.h"

using namespace libavcodecnet; 

ColorSpaceConverter::ColorSpaceConverter()
{
}

bool ColorSpaceConverter::InitializeH265Encoder(int width, int height, int crf)
{
    _h265EncoderCodec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    _h265EncoderContext = avcodec_alloc_context3(_h265EncoderCodec);
    _h265EncoderContext->time_base = { 10, 75 };
    _h265EncoderContext->framerate = { 75, 10 };
    _h265EncoderContext->codec = _h265EncoderCodec;
    _h265EncoderContext->codec_type = AVMEDIA_TYPE_VIDEO;
    _h265EncoderContext->codec_id = _h265EncoderCodec->id;
    _h265EncoderContext->profile = FF_PROFILE_HEVC_MAIN;
    //this->_nativePointers->codecCtx->gop_size = 10;
    //this->_nativePointers->codecCtx->level = FF_LEVEL_UNKNOWN;
    _h265EncoderContext->height = height;
    _h265EncoderContext->width = width;
    _h265EncoderContext->max_b_frames = 0;
    _h265EncoderContext->bit_rate = 60000000;
    _h265EncoderContext->pix_fmt = AV_PIX_FMT_YUV420P12;

    AVDictionary* av_dict_opts = nullptr;
    if (av_opt_set_int(_h265EncoderContext, "crf", crf, AV_OPT_SEARCH_CHILDREN) > 0)
    {
        return false;
    }

    //avformat_alloc_output_context2(&_nativePointers->formatContext, nullptr, nullptr, nullptr);
    //AVStream* avStream = avformat_new_stream(_nativePointers->formatContext, codec);
    //avcodec_parameters_from_context(avStream->codecpar, _codecContext);

    // yuv
    int imgBufSize = av_image_get_buffer_size(_h265EncoderContext->pix_fmt, _h265EncoderContext->width, _h265EncoderContext->height, 1);
    int ySize = _h265EncoderContext->height * _h265EncoderContext->width;

    _avFrame = av_frame_alloc();
    _avFrame->height = _h265EncoderContext->height;
    _avFrame->width = _h265EncoderContext->width;
    _avFrame->format = _h265EncoderContext->pix_fmt;

    _avBuffer = (uint8_t*)av_malloc(imgBufSize);

    int ret = av_image_fill_arrays(_avFrame->data,
        _avFrame->linesize, _avBuffer, _h265EncoderContext->pix_fmt, _h265EncoderContext->width, _h265EncoderContext->height, 1);
    if (ret > 0)
    {
        ret = avcodec_open2(_h265EncoderContext, _h265EncoderCodec, nullptr);
        if (ret == 0)
        {
            // TODO: now need to create the h265 decoder. 

            _h265DecoderCodec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
            _h265DecoderContext = avcodec_alloc_context3(_h265DecoderCodec);
            ret = avcodec_open2(_h265DecoderContext, _h265DecoderCodec, nullptr);

            return ret == 0; 
            //ret = avio_open2(&_nativePointers->formatContext->pb, _nativePointers->fileName.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
            //if (ret == 0)
            //{
            //    //ret = avformat_write_header(this->_nativePointers->formatCtx, &av_dict_opts);
            //    ret = avformat_write_header(this->_nativePointers->formatCtx, nullptr);
            //    if (ret == 0) {
            //        this->_nativePointers->swsContext =
            //            sws_getContext(_width, _height, AVPixelFormat::AV_PIX_FMT_GRAY16LE, _width, _height, DestFormat, SWS_BICUBIC, nullptr, nullptr, nullptr);
            //        return true;
            //    }
            //}
        }
    }

    return false; 
}

void ColorSpaceConverter::Convert16Bit2H265PNG(String^ inputPath, int width, int height, String^ destinationPath)
{
    marshal_context^ marshalcontext = gcnew marshal_context();

    const char* inputFileName = marshalcontext->marshal_as<const char*>(inputPath);

    AVFormatContext* formatContext = avformat_alloc_context();
    int ret = avformat_open_input(&formatContext, inputFileName, nullptr, nullptr);
    if (ret == 0)
    {
        AVCodecID codecId = AV_CODEC_ID_PNG;
        int index = 0;
        const AVCodec* pngInDecoder = avcodec_find_decoder(codecId);

        AVCodecContext* pngInCodecContext = avcodec_alloc_context3(pngInDecoder);
        ret = avcodec_parameters_to_context(pngInCodecContext, formatContext->streams[index]->codecpar);
        if (ret == 0)
        {
            ret = avcodec_open2(pngInCodecContext, pngInDecoder, nullptr);

            if (ret == 0)
            {
                AVFrame* decodedPngFrame = av_frame_alloc();
                AVPacket encodedPngPacket;
                av_init_packet(&encodedPngPacket);
                encodedPngPacket.data = NULL;
                encodedPngPacket.size = 0;

                SwsContext* png2YUV = sws_getContext(width, height, AVPixelFormat::AV_PIX_FMT_GRAY16BE, width, height, AV_PIX_FMT_YUV420P12, SWS_BICUBIC, nullptr, nullptr, nullptr);
                SwsContext* yuv2PNG = sws_getContext(width, height, AVPixelFormat::AV_PIX_FMT_YUV420P12, width, height, AV_PIX_FMT_GRAY16BE, SWS_BICUBIC, nullptr, nullptr, nullptr);

                int yuvBufSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P12, width, height, 1);
                auto* yuvBuf = (uint8_t*)av_malloc(yuvBufSize);

                AVFrame* yuvFrame = av_frame_alloc();
                yuvFrame->height = height;
                yuvFrame->width = width;
                yuvFrame->format = AV_PIX_FMT_YUV420P12;

                ret = av_image_fill_arrays(yuvFrame->data,
                    yuvFrame->linesize, yuvBuf, AV_PIX_FMT_YUV420P12, width, height, 1);
                if (ret > 0)
                {
                    int grayBufSize = av_image_get_buffer_size(AV_PIX_FMT_GRAY16BE, width, height, 1);
                    auto* grayBuf = (uint8_t*)av_malloc(grayBufSize);

                    AVFrame* grayFrame = av_frame_alloc();
                    grayFrame->height = height;
                    grayFrame->width = width;
                    grayFrame->format = AV_PIX_FMT_GRAY16BE;

                    ret = av_image_fill_arrays(grayFrame->data,
                        grayFrame->linesize, grayBuf, AV_PIX_FMT_GRAY16BE, width, height, 1);

                    if (ret > 0)
                    {
                        while (av_read_frame(formatContext, &encodedPngPacket) >= 0)
                        {
                            ret = ::avcodec_send_packet(pngInCodecContext, &encodedPngPacket);
                            if (ret == 0)
                            {
                                ret = avcodec_receive_frame(pngInCodecContext, decodedPngFrame);
                                if (ret == 0)
                                {
                                    int scaleRet = sws_scale(png2YUV, decodedPngFrame->data, decodedPngFrame->linesize, 0,
                                        height, yuvFrame->data, yuvFrame->linesize);

                                    if (scaleRet == height)
                                    {
                                        AVPacket* encodedH265Packet = av_packet_alloc();
                                        encodedH265Packet->data = nullptr;
                                        encodedH265Packet->size = 0;

                                        for (;;)
                                        {
                                            ret = ::avcodec_send_frame(_h265EncoderContext, yuvFrame);
                                            if (ret < 0)
                                            {
                                                break; 
                                            }
                                            ret = ::avcodec_receive_packet(_h265EncoderContext, encodedH265Packet);
                                            if (ret == AVERROR(EAGAIN))
                                            {
                                                continue;
                                            }
                                            else if (ret < 0)
                                            {
                                                break; 
                                            }

                                            break;
                                        }
                                        if (ret == 0)
                                        {
                                            ret = avcodec_send_packet(_h265DecoderContext, encodedH265Packet); 

                                            if (ret == 0)
                                            {
                                                int decodedYuvBufSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P12, width, height, 1);
                                                auto* decodedYuvBuf = (uint8_t*)av_malloc(decodedYuvBufSize);

                                                AVFrame* decodedYuvFrame = av_frame_alloc();
                                                decodedYuvFrame->height = height;
                                                decodedYuvFrame->width = width;
                                                decodedYuvFrame->format = AV_PIX_FMT_YUV420P12;

                                                ret = av_image_fill_arrays(decodedYuvFrame->data,
                                                    decodedYuvFrame->linesize, decodedYuvBuf, AV_PIX_FMT_GRAY16BE, width, height, 1);

                                                if (ret >0)
                                                {
                                                    ret = avcodec_receive_frame(_h265DecoderContext, decodedYuvFrame); 

                                                    if (ret == 0)
                                                    {
                                                        scaleRet = sws_scale(yuv2PNG, decodedYuvFrame->data, decodedYuvFrame->linesize, 0,
                                                            height, grayFrame->data, grayFrame->linesize);
                                                        if (scaleRet == height)
                                                        {
                                                            const AVCodec* outCodec = avcodec_find_encoder(AVCodecID::AV_CODEC_ID_PNG);
                                                            AVCodecContext* outCodecCtx = avcodec_alloc_context3(outCodec);

                                                            outCodecCtx->width = width;
                                                            outCodecCtx->height = height;
                                                            outCodecCtx->pix_fmt = AVPixelFormat::AV_PIX_FMT_GRAY16BE; // AVPixelFormat::AV_PIX_FMT_RGBA;
                                                            outCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
                                                            outCodecCtx->time_base.num = 1;
                                                            outCodecCtx->time_base.den = 25;

                                                            ret = avcodec_open2(outCodecCtx, outCodec, nullptr);
                                                            if (ret == 0)
                                                            {
                                                                ret = ::avcodec_send_frame(outCodecCtx, grayFrame);
                                                                if (ret == 0)
                                                                {
                                                                    AVPacket* yuvEncodedPacket = av_packet_alloc();
                                                                    yuvEncodedPacket->data = nullptr;
                                                                    yuvEncodedPacket->size = 0;
                                                                    ret = ::avcodec_receive_packet(outCodecCtx, yuvEncodedPacket);

                                                                    if (ret == 0)
                                                                    {
                                                                        const char* outputFileName = marshalcontext->marshal_as<const char*>(destinationPath);

                                                                        FILE* outPng = fopen(outputFileName, "wb");
                                                                        fwrite(yuvEncodedPacket->data, yuvEncodedPacket->size, 1, outPng);
                                                                        fclose(outPng);
                                                                    }

                                                                    if (yuvEncodedPacket)
                                                                    {
                                                                        av_packet_free(&yuvEncodedPacket);
                                                                    }
                                                                }
                                                            }

                                                            if (outCodecCtx)
                                                            {
                                                                avcodec_free_context(&outCodecCtx);
                                                            }
                                                        }
                                                        else
                                                        {
                                                            printf("Invalid scale ret.");
                                                        }
                                                    }
                                                }

                                                if (decodedYuvBuf)
                                                {
                                                    av_free(decodedYuvBuf); 
                                                }

                                                if (decodedYuvFrame)
                                                {
                                                    av_frame_free(&decodedYuvFrame); 
                                                }
                                            }
                                        }

                                        if (encodedH265Packet)
                                        {
                                            av_packet_free(&encodedH265Packet); 
                                        }
                                    }
                                    else
                                    {
                                        printf("Invalid scale ret.");
                                    }
                                }
                            }

                            av_packet_unref(&encodedPngPacket);
                        }
                    }

                    if (grayFrame)
                    {
                        av_frame_free(&grayFrame);
                    }
                    if (grayBuf)
                    {
                        av_free(grayBuf);
                    }
                }
                if (decodedPngFrame)
                {
                    av_frame_free(&decodedPngFrame);
                }
                if (yuvBuf)
                {
                    av_free(yuvBuf);
                }
                if (yuvFrame)
                {
                    av_frame_free(&yuvFrame);
                }
                if (png2YUV)
                {
                    sws_freeContext(png2YUV);
                }
                if (yuv2PNG)
                {
                    sws_freeContext(yuv2PNG);
                }
                avcodec_close(pngInCodecContext);
            }
        }

        if (pngInCodecContext)
        {
            avcodec_free_context(&pngInCodecContext);
        }
    }

    if (formatContext)
    {
        avformat_close_input(&formatContext);
    }

    if (ret != 0)
    {
        char szBuffer[1024];
        av_strerror(ret, szBuffer, sizeof(szBuffer));
    }
}

void ColorSpaceConverter::Convert16Bit2YChannelPNG(String^ inputPath, int width, int height, String^ destinationPath)
{
    marshal_context^ marshalcontext = gcnew marshal_context();

    const char* inputFileName = marshalcontext->marshal_as<const char*>(inputPath);

    AVFormatContext* formatContext = avformat_alloc_context();
    int ret = avformat_open_input(&formatContext, inputFileName, nullptr, nullptr);
    if (ret == 0)
    {
        AVCodecID codecId = AV_CODEC_ID_PNG;
        int index = 0;
        const AVCodec* decoder = avcodec_find_decoder(codecId);

        AVCodecContext* context = avcodec_alloc_context3(decoder);
        ret = avcodec_parameters_to_context(context, formatContext->streams[index]->codecpar);
        if (ret == 0)
        {
            ret = avcodec_open2(context, decoder, nullptr);
            
            if (ret == 0)
            {
                AVFrame* decodedFrame = av_frame_alloc();
                AVPacket packetToDecode;
                av_init_packet(&packetToDecode);
                packetToDecode.data = NULL;
                packetToDecode.size = 0;

                SwsContext* png2YUV = sws_getContext(width, height, AVPixelFormat::AV_PIX_FMT_GRAY16BE, width, height, AV_PIX_FMT_YUV420P12, SWS_BICUBIC, nullptr, nullptr, nullptr);
                SwsContext* yuv2PNG = sws_getContext(width, height, AVPixelFormat::AV_PIX_FMT_YUV420P12, width, height, AV_PIX_FMT_GRAY16BE, SWS_BICUBIC, nullptr, nullptr, nullptr);

                int yuvBufSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P12, width, height, 1);
                auto* yuvBuf = (uint8_t*)av_malloc(yuvBufSize);

                AVFrame* yuvFrame = av_frame_alloc();
                yuvFrame->height = height;
                yuvFrame->width = width;
                yuvFrame->format = AV_PIX_FMT_YUV420P12;

                ret = av_image_fill_arrays(yuvFrame->data,
                    yuvFrame->linesize, yuvBuf, AV_PIX_FMT_YUV420P12, width, height, 1);
                if (ret > 0)
                {
                    int grayBufSize = av_image_get_buffer_size(AV_PIX_FMT_GRAY16BE, width, height, 1);
                    auto* grayBuf = (uint8_t*)av_malloc(grayBufSize);

                    AVFrame* grayFrame = av_frame_alloc();
                    grayFrame->height = height;
                    grayFrame->width = width;
                    grayFrame->format = AV_PIX_FMT_GRAY16BE;

                    ret = av_image_fill_arrays(grayFrame->data,
                        grayFrame->linesize, grayBuf, AV_PIX_FMT_GRAY16BE, width, height, 1);

                    if (ret > 0)
                    {
                        while (av_read_frame(formatContext, &packetToDecode) >= 0)
                        {
                            ret = ::avcodec_send_packet(context, &packetToDecode);
                            if (ret == 0)
                            {
                                ret = avcodec_receive_frame(context, decodedFrame);
                                if (ret == 0)
                                {
                                    int scaleRet = sws_scale(png2YUV, decodedFrame->data, decodedFrame->linesize, 0,
                                        height, yuvFrame->data, yuvFrame->linesize);
                                    if (scaleRet == height)
                                    {
                                        scaleRet = sws_scale(yuv2PNG, yuvFrame->data, yuvFrame->linesize, 0,
                                            height, grayFrame->data, grayFrame->linesize);
                                        if (scaleRet == height)
                                        {
                                            const AVCodec* outCodec = avcodec_find_encoder(AVCodecID::AV_CODEC_ID_PNG);
                                            AVCodecContext* outCodecCtx = avcodec_alloc_context3(outCodec);

                                            outCodecCtx->width = width;
                                            outCodecCtx->height = height;
                                            outCodecCtx->pix_fmt = AVPixelFormat::AV_PIX_FMT_GRAY16BE; // AVPixelFormat::AV_PIX_FMT_RGBA;
                                            outCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
                                            outCodecCtx->time_base.num = 1;
                                            outCodecCtx->time_base.den = 25;

                                            ret = avcodec_open2(outCodecCtx, outCodec, nullptr);
                                            if (ret == 0)
                                            {
                                                ret = ::avcodec_send_frame(outCodecCtx, grayFrame);
                                                if (ret == 0)
                                                {
                                                    AVPacket* encodedPacket = av_packet_alloc();
                                                    encodedPacket->data = nullptr;
                                                    encodedPacket->size = 0;
                                                    ret = ::avcodec_receive_packet(outCodecCtx, encodedPacket);

                                                    if (ret == 0)
                                                    {
                                                        const char* outputFileName = marshalcontext->marshal_as<const char*>(destinationPath);

                                                        FILE* outPng = fopen(outputFileName, "wb");
                                                        fwrite(encodedPacket->data, encodedPacket->size, 1, outPng);
                                                        fclose(outPng);
                                                    }

                                                    if (encodedPacket)
                                                    {
                                                        av_packet_free(&encodedPacket); 
                                                    }
                                                }
                                            }

                                            if (outCodecCtx)
                                            {
                                                avcodec_free_context(&outCodecCtx);
                                            }
                                        }
                                        else
                                        {
                                            printf("Invalid scale ret.");
                                        }
                                    }
                                    else
                                    {
                                        printf("Invalid scale ret."); 
                                    }
                                }
                            }

                            av_packet_unref(&packetToDecode);
                        }
                    }

                    if (grayFrame)
                    {
                        av_frame_free(&grayFrame); 
                    }
                    if (grayBuf)
                    {
                        av_free(grayBuf); 
                    }
                }
                if (decodedFrame)
                {
                    av_frame_free(&decodedFrame);
                }
                if (yuvBuf)
                {
                    av_free(yuvBuf); 
                }
                if (yuvFrame)
                {
                    av_frame_free(&yuvFrame); 
                }
                if (png2YUV)
                {
                    sws_freeContext(png2YUV); 
                }
                if (yuv2PNG)
                {
                    sws_freeContext(yuv2PNG); 
                }
                avcodec_close(context);
            }
        }

        if (context)
        {
            avcodec_free_context(&context);
        }
    }

    if (formatContext)
    {
        avformat_close_input(&formatContext);
    }

    if (ret != 0)
    {
        char szBuffer[1024]; 
        av_strerror(ret, szBuffer, sizeof(szBuffer)); 
    }
}

void ColorSpaceConverter::Save16BitYChannelPNG(array<UInt16>^ frameData, int width, int height, String^ destinationPath)
{
    pin_ptr<unsigned short> frameDataPtr = &frameData[0];

    uint16_t biggest = 0; 
    for (int c = 0; c < frameData->Length; c++)
    {
        if (frameData[c] > biggest)
        {
            biggest = frameData[c]; 
        }
    }

    AVFrame* avFrame = av_frame_alloc();
    avFrame->height = height;
    avFrame->width = width;
    avFrame->format = AV_PIX_FMT_YUV420P12;

    int imgBufSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P12, width, height, 1);
    auto* imgBuf = (uint8_t*)av_malloc(imgBufSize);

    int ret = av_image_fill_arrays(avFrame->data,
        avFrame->linesize, imgBuf, AV_PIX_FMT_YUV420P12, width, height, 1);

    auto swsContextToYUV = sws_getContext(width, height, AVPixelFormat::AV_PIX_FMT_GRAY16LE, width, height, AV_PIX_FMT_YUV420P12, SWS_BICUBIC, nullptr, nullptr, nullptr);

    int inLinesize[1] = { 2 * width };
    const uint8_t* ptr = (uint8_t*)frameDataPtr;
    ret = sws_scale(swsContextToYUV, (const uint8_t* const*)&ptr, inLinesize, 0,
        height, avFrame->data, avFrame->linesize);

    auto swsContextBackTo16 = sws_getContext(width, height, AV_PIX_FMT_YUV420P12, width, height, AVPixelFormat::AV_PIX_FMT_GRAY16BE, SWS_BICUBIC, nullptr, nullptr, nullptr);
    uint16_t* convertedBack16 = new uint16_t[width * height]; 

    ret = sws_scale(swsContextBackTo16, (const uint8_t* const*)avFrame->data, avFrame->linesize, 0,
        height, (uint8_t* const*)&convertedBack16, inLinesize);

    AVFrame* avFrameBack = av_frame_alloc();
    avFrameBack->height = height;
    avFrameBack->width = width;
    avFrameBack->format = AV_PIX_FMT_GRAY16BE;
    ret = av_image_fill_arrays(avFrameBack->data,
        avFrameBack->linesize,(uint8_t*)convertedBack16, AV_PIX_FMT_GRAY16BE, width, height, 1);

    //uint16_t biggest2 = 0;
    //for (int c = 0; c < 512*424; c++)
    //{
    //    if (convertedBack16[c] > biggest2)
    //    {
    //        biggest2 = convertedBack16[c];
    //    }
    //}

    const AVCodec* outCodec = avcodec_find_encoder(AVCodecID::AV_CODEC_ID_PNG);
    AVCodecContext* outCodecCtx = avcodec_alloc_context3(outCodec);

    outCodecCtx->width = width;
    outCodecCtx->height = height; 
    outCodecCtx->pix_fmt = AVPixelFormat::AV_PIX_FMT_GRAY16BE; // AVPixelFormat::AV_PIX_FMT_RGBA;
    outCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    outCodecCtx->time_base.num = 1;  
    outCodecCtx->time_base.den = 25; 
     
    ret = avcodec_open2(outCodecCtx, outCodec, nullptr); 

    ret = ::avcodec_send_frame(outCodecCtx, avFrameBack);
    printf("encoded: %d\n", ret); 

    AVPacket* packet = av_packet_alloc();
    packet->data = nullptr;
    packet->size = 0;
    ret = ::avcodec_receive_packet(outCodecCtx, packet);
    printf("decoded: %d\n", ret); 

    marshal_context^ context = gcnew marshal_context();
    const char* nativefileName = context->marshal_as<const char*>(destinationPath);

    FILE* outPng = fopen(nativefileName, "wb");
    fwrite(packet->data, packet->size, 1, outPng);
    fclose(outPng);

    av_packet_free(&packet); 
    av_frame_free(&avFrame); 
    sws_freeContext(swsContextToYUV);
    av_free(imgBuf); 

    delete convertedBack16; 
}
