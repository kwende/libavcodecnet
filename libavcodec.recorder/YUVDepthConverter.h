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

using namespace System; 

ref class YUVDepthConverter
{
public:
	void Initialize(int width, int height); 
	array<UInt16>^ YUV2Depth(AVFrame* yuv); 
	AVFrame* Depth2YUV(array<UInt16>^ depth); 
	~YUVDepthConverter(); 
private:
	SwsContext* _depth2YUVContext, * _yuv2PNGContext;
	int _width, _height; 
	int _yuvBufSize; 
};

