#include "pch.h"

#include "libavcodec.recorder.h"

using namespace libavcodecnet; 
using namespace System::Runtime::InteropServices; 
using namespace System; 

Recorder::Recorder(std::string fileName, int width, int height, int fps, int bitrate)
{
	_nativePointers = new NativePointers(); 
	_width = width; 
	_height = height; 
	_fps = fps; 
    _bitrate = bitrate; 
    _frameCounter = 0; 
}

Recorder::~Recorder()
{
    if (_nativePointers != nullptr)
    {
        Close(); 

        if (_nativePointers->videoFrame != nullptr) {
            av_frame_free(&_nativePointers->videoFrame);
        }
        if (_nativePointers->cctx != nullptr) {
            avcodec_free_context(&_nativePointers->cctx);
        }
        if (_nativePointers->ofctx != nullptr) {
            avformat_free_context(_nativePointers->ofctx);
        }
        if (_nativePointers->swsctx != nullptr) {
            sws_freeContext(_nativePointers->swsctx);
        }
        if (_nativePointers->scratchData != nullptr)
        {
            delete _nativePointers->scratchData; 
        }

        delete _nativePointers; 

        _nativePointers = nullptr; 
    }
}

void Recorder::Initialize()
{
    _nativePointers->oformat = (AVOutputFormat*)av_guess_format(nullptr, "test.mp4", nullptr);
    if (!_nativePointers->oformat)
    {
        return;
    }
    //oformat->video_codec = AV_CODEC_ID_H265;

    int err = avformat_alloc_output_context2(&_nativePointers->ofctx, _nativePointers->oformat, nullptr, "test.mp4");
    if (err)
    {
        return;
    }

     const AVCodec* codec = avcodec_find_encoder(_nativePointers->oformat->video_codec);
    if (!codec)
    {
        return;
    }

    AVStream* stream = avformat_new_stream(_nativePointers->ofctx, codec);

    if (!stream)
    {
        return;
    }

    _nativePointers->cctx = avcodec_alloc_context3(codec);

    if (!_nativePointers->cctx)
    {
        return;
    }

    stream->codecpar->codec_id = _nativePointers->oformat->video_codec;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->codecpar->width = _width;
    stream->codecpar->height = _height;
    stream->codecpar->format = AV_PIX_FMT_YUV420P;
    stream->codecpar->bit_rate = _bitrate * 1000;
    avcodec_parameters_to_context(_nativePointers->cctx, stream->codecpar);
    _nativePointers->cctx->time_base.num = 1;
    _nativePointers->cctx->time_base.den = 1;
    _nativePointers->cctx->max_b_frames = 2;
    _nativePointers->cctx->gop_size = 12;
    _nativePointers->cctx->framerate.den = _fps; 
    _nativePointers->cctx->framerate.num = 1;

    if (stream->codecpar->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(_nativePointers->cctx, "preset", "ultrafast", 0);
    }
    else if (stream->codecpar->codec_id == AV_CODEC_ID_H265)
    {
        av_opt_set(_nativePointers->cctx, "preset", "ultrafast", 0);
    }

    avcodec_parameters_from_context(stream->codecpar, _nativePointers->cctx);

    if ((err = avcodec_open2(_nativePointers->cctx, codec, NULL)) < 0) {
        return;
    }

    if (!(_nativePointers->oformat->flags & AVFMT_NOFILE)) {
        if ((err = avio_open(&_nativePointers->ofctx->pb, "test.mp4", AVIO_FLAG_WRITE)) < 0) {
            return;
        }
    }

    if ((err = avformat_write_header(_nativePointers->ofctx, NULL)) < 0) {
        return;
    }

    //av_dump_format(ofctx, 0, "test.mp4", 1);
}

void Recorder::Close()
{
    if (_nativePointers != nullptr)
    {
        //DELAYED FRAMES
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        for (;;) {
            avcodec_send_frame(_nativePointers->cctx, NULL);
            if (avcodec_receive_packet(_nativePointers->cctx, &pkt) == 0) {
                av_interleaved_write_frame(_nativePointers->ofctx, &pkt);
                av_packet_unref(&pkt);
            }
            else {
                break;
            }
        }

        av_write_trailer(_nativePointers->ofctx);
        if (!(_nativePointers->oformat->flags & AVFMT_NOFILE)) {
            int err = avio_close(_nativePointers->ofctx->pb);
            if (err < 0) {
            }
        }
    }
}

void Recorder::WriteFrame(array<Byte>^ frameData)
{
    if (_nativePointers->scratchData == nullptr)
    {
        _nativePointers->scratchData = new uint8_t[frameData->Length];
    }

    Marshal::Copy(frameData, 0, IntPtr(_nativePointers->scratchData), frameData->Length);

    int err;
    if (!_nativePointers->videoFrame) {
        _nativePointers->videoFrame = av_frame_alloc();
        _nativePointers->videoFrame->format = AV_PIX_FMT_YUV420P;
        _nativePointers->videoFrame->width = _nativePointers->cctx->width;
        _nativePointers->videoFrame->height = _nativePointers->cctx->height;
        if ((err = av_frame_get_buffer(_nativePointers->videoFrame, 32)) < 0) {
            return;
        }
    }
    if (!_nativePointers->swsctx) {
        _nativePointers->swsctx = sws_getContext(_nativePointers->cctx->width, 
            _nativePointers->cctx->height, AV_PIX_FMT_RGB24, _nativePointers->cctx->width,
            _nativePointers->cctx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);
    }
    int inLinesize[1] = { 3 * _nativePointers->cctx->width };
    // From RGB to YUV
    sws_scale(_nativePointers->swsctx, (const uint8_t* const*)&_nativePointers->scratchData, inLinesize, 0, _nativePointers->cctx->height,
        _nativePointers->videoFrame->data, _nativePointers->videoFrame->linesize);
    _nativePointers->videoFrame->pts = (1.0 / _fps) * 90000 * (_frameCounter++); // i think there's a bug here
    if ((err = avcodec_send_frame(_nativePointers->cctx, _nativePointers->videoFrame)) < 0) {
        return;
    }
    AV_TIME_BASE;
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    pkt.flags |= AV_PKT_FLAG_KEY;
    if (avcodec_receive_packet(_nativePointers->cctx, &pkt) == 0) {
        uint8_t* size = ((uint8_t*)pkt.data);
        av_interleaved_write_frame(_nativePointers->ofctx, &pkt);
        av_packet_unref(&pkt);
    }
}

Recorder^ Recorder::Create(String^ fileName, int width, int height, int fps, int bitrate)
{
	marshal_context^ context = gcnew marshal_context();
	const char* nativefileName = context->marshal_as<const char*>(fileName);

	Recorder^ recorder = gcnew Recorder(std::string(nativefileName), width, height, fps, bitrate); 
	recorder->Initialize(); 

	delete context; 
	return recorder;
}
