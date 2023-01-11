#include "pch.h"
#include "Recoder16.h"

using namespace libavcodecnet; 

Recorder16::Recorder16(std::string fileName, int width, int height, int fps, int crf)
{
	_nativePointers = new NativePointers16();
	_nativePointers->fileName = std::string(fileName);

	_width = width;
	_height = height;
	_fps = fps;
	_crf = crf;
	_frameCounter = 0;
}

Recorder16^ Recorder16::Create(String^ filename, int width, int height, int fps, int crf)
{
	marshal_context^ context = gcnew marshal_context();
	const char* nativefileName = context->marshal_as<const char*>(filename);

	Recorder16^ recorder = gcnew Recorder16(std::string(nativefileName), width, height, fps, crf);
	recorder->Initialize();

	delete context;
	return recorder;
}

bool Recorder16::Initialize()
{
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    this->_nativePointers->codecCtx = avcodec_alloc_context3(codec);
    this->_nativePointers->codecCtx->time_base = { 1, _fps };
    this->_nativePointers->codecCtx->framerate = { _fps, 1 };
    this->_nativePointers->codecCtx->codec = codec;
    this->_nativePointers->codecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    this->_nativePointers->codecCtx->codec_id = codec->id;
    this->_nativePointers->codecCtx->profile = FF_PROFILE_HEVC_MAIN;
    //this->_nativePointers->codecCtx->gop_size = 10;
    //this->_nativePointers->codecCtx->level = FF_LEVEL_UNKNOWN;
    this->_nativePointers->codecCtx->height = _height;
    this->_nativePointers->codecCtx->width = _width;
    this->_nativePointers->codecCtx->max_b_frames = 0;
    this->_nativePointers->codecCtx->bit_rate = 60000000;
    this->_nativePointers->codecCtx->pix_fmt = DestFormat;

    //this->_nativePointers->codecCtx->qmin = 10;
    //this->_nativePointers->codecCtx->qmax = 51;
    if (codec->id == AV_CODEC_ID_HEVC) {


        int ret = av_opt_set(this->_nativePointers->codecCtx->priv_data, "x265-params", "lossless", 1);
        printf("%d\n", ret); 
        //av_opt_set(this->_nativePointers->codecCtx->priv_data, "preset", "medium", 0);
        //av_opt_set(this->_nativePointers->codecCtx->priv_data, "tune", "zero-latency", 0);
    }

    AVDictionary* av_dict_opts = nullptr;
    av_dict_set(&av_dict_opts, "x265-params", "lossless=1:log-level=4", AV_DICT_MATCH_CASE);


    avformat_alloc_output_context2(&this->_nativePointers->formatCtx, nullptr, nullptr, this->_nativePointers->fileName.c_str());
    AVStream* avStream = avformat_new_stream(this->_nativePointers->formatCtx, codec);
    avcodec_parameters_from_context(avStream->codecpar, this->_nativePointers->codecCtx);

    // yuv
    int imgBufSize = av_image_get_buffer_size(this->_nativePointers->codecCtx->pix_fmt, this->_nativePointers->codecCtx->width, this->_nativePointers->codecCtx->height, 1);
    int ySize = this->_nativePointers->codecCtx->height * this->_nativePointers->codecCtx->width;

    this->_nativePointers->avFrame = av_frame_alloc();
    this->_nativePointers->avFrame->height = this->_nativePointers->codecCtx->height;
    this->_nativePointers->avFrame->width = this->_nativePointers->codecCtx->width;
    this->_nativePointers->avFrame->format = this->_nativePointers->codecCtx->pix_fmt;

    auto* imgBuf = (uint8_t*)av_malloc(imgBufSize);

    int ret = av_image_fill_arrays(this->_nativePointers->avFrame->data, 
        this->_nativePointers->avFrame->linesize, imgBuf, this->_nativePointers->codecCtx->pix_fmt, this->_nativePointers->codecCtx->width, this->_nativePointers->codecCtx->height, 1);
    if (ret < 0) {
        std::cout << "av_image_fill_arrays fail " << ret << std::endl;
        return false;
    }
    ret = avcodec_open2(this->_nativePointers->codecCtx, codec, nullptr);
    if (ret < 0) {
        std::cout << "avcodec_open2 fail " << ret << std::endl;
        return false; 
    }

    ret = avio_open2(&this->_nativePointers->formatCtx->pb, _nativePointers->fileName.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
    if (ret < 0) {
        std::cout << "avio_open2 fail " << ret << std::endl;
    }


    ret = avformat_write_header(this->_nativePointers->formatCtx, &av_dict_opts);
    if (ret < 0) {
        std::cout << "avio_open2 fail " << ret << std::endl;
        return -1;
    }

    this->_nativePointers->swsContext = 
        sws_getContext(_width, _height, AVPixelFormat::AV_PIX_FMT_GRAY16LE, _width, _height, DestFormat, SWS_BICUBIC, nullptr, nullptr, nullptr);

    return true; 
}

void Recorder16::WriteFrame(array<UInt16>^ frameData)
{
    AVPacket* packet = av_packet_alloc();
    packet->data = nullptr;
    packet->size = 0;

    pin_ptr<unsigned short> frameDataPtr = &frameData[0]; 

    int inLinesize[1] = { 2 * _width };
    const uint8_t* ptr = (uint8_t*)frameDataPtr;
    int ret = sws_scale(this->_nativePointers->swsContext, (const uint8_t* const*)&ptr, inLinesize, 0, 
        _height, this->_nativePointers->avFrame->data, this->_nativePointers->avFrame->linesize);

    //FILE* file; 
    //fopen_s(&file, "c:/users/brush/desktop/binary.dat", "wb+"); 
    ////size_t written = fwrite(this->_nativePointers->avFrame->data[0], sizeof(uint16_t), 512 * 424, file); 
    //size_t written = fwrite(this->_nativePointers->avFrame->data[0], sizeof(uint16_t), 512 * 424, file);
    //fclose(file); 

    //FILE* file2;
    //fopen_s(&file2, "c:/users/brush/desktop/binary_orig.dat", "wb+");
    ////size_t written = fwrite(this->_nativePointers->avFrame->data[0], sizeof(uint16_t), 512 * 424, file); 
    //written = fwrite(ptr, sizeof(uint16_t), 512 * 424, file2);
    //fclose(file2);

    //uint16_t* backFromOriginal = new uint16_t[512 * 424]; 

    //auto swscontext2 = sws_getContext(_width, _height, DestFormat, _width, _height, AVPixelFormat::AV_PIX_FMT_GRAY16LE, SWS_BICUBIC, nullptr, nullptr, nullptr);

    //ret = sws_scale(swscontext2, (const uint8_t* const*)this->_nativePointers->avFrame->data, this->_nativePointers->avFrame->linesize, 0,
    //    _height, (uint8_t* const*)&backFromOriginal, inLinesize);


    //FILE* file3;
    //fopen_s(&file3, "c:/users/brush/desktop/binary_back.dat", "wb+");
    ////size_t written = fwrite(this->_nativePointers->avFrame->data[0], sizeof(uint16_t), 512 * 424, file); 
    //written = fwrite(backFromOriginal, sizeof(uint16_t), 512 * 424, file3);
    //fclose(file3);

    this->_nativePointers->avFrame->pts = (1.0 / 25) * 90000 * (_frameCounter); // i think there's a bug here

    for (;;)
    {
        ret = ::avcodec_send_frame(this->_nativePointers->codecCtx, this->_nativePointers->avFrame);
        if (ret < 0)
        {
            return;
        }

        ret = ::avcodec_receive_packet(this->_nativePointers->codecCtx, packet);
        if (ret == AVERROR(EAGAIN))
        {
            continue;
        }
        else if (ret < 0)
        {
            return;
        }
        break;
    }

    ret = av_interleaved_write_frame(this->_nativePointers->formatCtx, packet);
    if (ret < 0)
    {
        return;
    }

    if (packet)
    {
        av_packet_free(&packet);
    }

    _frameCounter++; 
}

Recorder16::~Recorder16()
{
    if (_nativePointers != nullptr)
    {
        Close();

        //if (_nativePointers->videoFrame != nullptr) {
        //    av_frame_free(&_nativePointers->videoFrame);
        //}
        if (_nativePointers->codecCtx != nullptr) {
            avcodec_free_context(&_nativePointers->codecCtx);
        }
        if (_nativePointers->formatCtx != nullptr) {
            avformat_free_context(_nativePointers->formatCtx);
        }
        //if (_nativePointers->swsctx != nullptr) {
        //    sws_freeContext(_nativePointers->swsctx);
        //}
        //if (_nativePointers->scratchData != nullptr)
        //{
        //    delete _nativePointers->scratchData;
        //}
        //if (_nativePointers->fileName != nullptr)
        //{
        //    delete _nativePointers->fileName;
        //}

        delete _nativePointers;

        _nativePointers = nullptr;
    }
}

void Recorder16::Close()
{
    if (_nativePointers != nullptr && !_flushed)
    {
        //DELAYED FRAMES
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        for (;;) {
            avcodec_send_frame(_nativePointers->codecCtx, NULL);
            if (avcodec_receive_packet(_nativePointers->codecCtx, &pkt) == 0) {
                av_interleaved_write_frame(_nativePointers->formatCtx, &pkt);
                av_packet_unref(&pkt);
            }
            else {
                break;
            }
        }

        av_write_trailer(_nativePointers->formatCtx);
        if (!(_nativePointers->formatCtx->flags & AVFMT_NOFILE)) {
            int err = avio_close(_nativePointers->formatCtx->pb);
            if (err < 0) {
            }
        }

        _flushed = true;
    }
}