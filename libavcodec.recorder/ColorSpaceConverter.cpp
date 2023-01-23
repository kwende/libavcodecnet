#include "pch.h"
#include "ColorSpaceConverter.h"

using namespace libavcodecnet; 

ColorSpaceConverter::ColorSpaceConverter()
{
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
