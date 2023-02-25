#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
}

#include "YUVDepthConverter.h"

using namespace System;

struct NativeInstances
{
	AVCodecContext* Encoder, * Decoder;
};

public ref class h265Converter
{
public: 
	~h265Converter(); 

	int Initialize(int width, int height, int crf); 
	array<UInt16>^ H2652Depth(AVPacket* yuv);
	AVPacket* Depth2H265(array<UInt16>^ depth);
	array<UInt16>^ EncodeAndDecodeDepth(array<UInt16>^ input);
private:
	YUVDepthConverter^ _yuvDepthConverter; 
	int _width, _height; 
	NativeInstances* _nativeInstances; 
};

