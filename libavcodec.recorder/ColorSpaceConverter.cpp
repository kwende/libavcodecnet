#include "pch.h"
#include "ColorSpaceConverter.h"

using namespace libavcodecnet; 

ColorSpaceConverter::ColorSpaceConverter()
{

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
                                    ret = sws_scale(png2YUV, decodedFrame->data, decodedFrame->linesize, 0,
                                        height, yuvFrame->data, yuvFrame->linesize);
                                    if (ret == height)
                                    {
                                        ret = sws_scale(yuv2PNG, yuvFrame->data, yuvFrame->linesize, 0,
                                            height, grayFrame->data, grayFrame->linesize);
                                        if (ret == height)
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
                                    }
                                }
                            }
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
            }
        }

        if (context)
        {
            avcodec_free_context(&context);
        }
    }

    if (formatContext)
    {
        avformat_free_context(formatContext);
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
