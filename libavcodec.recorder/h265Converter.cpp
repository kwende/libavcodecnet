#include "pch.h"
#include "h265Converter.h"

h265Converter::~h265Converter()
{
	delete _yuvDepthConverter; 

	avcodec_free_context(&_nativeInstances->Encoder); 
	avcodec_free_context(&_nativeInstances->Decoder); 

	delete _nativeInstances; 
}

int h265Converter::Initialize(int width, int height, int crf)
{
	_nativeInstances = new NativeInstances(); 

	_width = width; 
	_height = height; 

	_yuvDepthConverter = gcnew YUVDepthConverter(); 
	_yuvDepthConverter->Initialize(width, height); 

	const AVCodec* h265EncoderCodec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
	_nativeInstances->Encoder = avcodec_alloc_context3(h265EncoderCodec);
	_nativeInstances->Encoder->time_base = { 10, 75 };
	_nativeInstances->Encoder->framerate = { 75, 10 };
	_nativeInstances->Encoder->codec = h265EncoderCodec;
	_nativeInstances->Encoder->codec_type = AVMEDIA_TYPE_VIDEO;
	_nativeInstances->Encoder->codec_id = h265EncoderCodec->id;
	_nativeInstances->Encoder->profile = FF_PROFILE_HEVC_MAIN;
	_nativeInstances->Encoder->height = height;
	_nativeInstances->Encoder->width = width;
	_nativeInstances->Encoder->max_b_frames = 0;
	_nativeInstances->Encoder->bit_rate = 60000000;
	_nativeInstances->Encoder->pix_fmt = AV_PIX_FMT_YUV420P12;

	AVDictionary* av_dict_opts = nullptr;
	int res = av_opt_set_int(_nativeInstances->Encoder, "crf", crf, AV_OPT_SEARCH_CHILDREN);
	if (res == 0)
	{
		int ret = avcodec_open2(_nativeInstances->Encoder, h265EncoderCodec, nullptr);
		if (ret == 0)
		{
			const AVCodec* h265DecoderCodec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
			_nativeInstances->Decoder = avcodec_alloc_context3(h265DecoderCodec);
			ret = avcodec_open2(_nativeInstances->Decoder, h265DecoderCodec, nullptr);
		}
	}
	else
	{
		avcodec_free_context(&_nativeInstances->Encoder);
	}

	return res; 
}

array<UInt16>^ h265Converter::H2652Depth(AVPacket* yuv)
{
	array<UInt16>^ ret = nullptr; 
	int res = avcodec_send_packet(_nativeInstances->Decoder, yuv);
	if (res == 0)
	{
		int decodedDepthBufSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P12, _width, _height, 1);
		auto* decodedDepthBuf = (uint8_t*)av_malloc(decodedDepthBufSize);

		AVFrame* decodedDepthFrame = av_frame_alloc();
		decodedDepthFrame->height = _height;
		decodedDepthFrame->width = _width;
		decodedDepthFrame->format = AV_PIX_FMT_YUV420P12;

		res = av_image_fill_arrays(decodedDepthFrame->data,
			decodedDepthFrame->linesize, decodedDepthBuf, AV_PIX_FMT_GRAY16BE, _width, _height, 1);

		if (res > 0)
		{
			res = avcodec_receive_frame(_nativeInstances->Decoder, decodedDepthFrame);
			if (res == 0)
			{
				ret = _yuvDepthConverter->YUV2Depth(decodedDepthFrame);
			}
		}

		if (decodedDepthFrame)
		{
			av_frame_free(&decodedDepthFrame); 
		}

		if (decodedDepthBuf)
		{
			av_free(decodedDepthBuf); 
		}
	}
	return ret;
}

AVPacket* h265Converter::Depth2H265(array<UInt16>^ depth)
{
	AVFrame* yuvFrame = _yuvDepthConverter->Depth2YUV(depth); 

	AVPacket* h265EncodedPacket = av_packet_alloc();
	h265EncodedPacket->data = nullptr;
	h265EncodedPacket->size = 0;

	int res = 0; 
	for (;;)
	{
		res = ::avcodec_send_frame(_nativeInstances->Encoder, yuvFrame);
		if (res < 0)
		{
			break;
		}
		res = ::avcodec_receive_packet(_nativeInstances->Encoder, h265EncodedPacket);
		if (res == AVERROR(EAGAIN))
		{
			continue;
		}
		else if (res < 0)
		{
			break;
		}

		break;
	}

	av_free(yuvFrame->data[0]); 
	av_frame_free(&yuvFrame); 

	return h265EncodedPacket;
}

array<UInt16>^ h265Converter::EncodeAndDecodeDepth(array<UInt16>^ input)
{
	AVPacket* encoded = Depth2H265(input); 
	array<UInt16>^ ret= H2652Depth(encoded);

	av_packet_free(&encoded); 

	//auto output = _yuvDepthConverter->Depth2YUV(input); 
	//auto ret = _yuvDepthConverter->YUV2Depth(output); 
	//av_frame_free(&output);

	return ret;
}