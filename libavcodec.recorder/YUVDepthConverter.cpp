#include "pch.h"
#include "YUVDepthConverter.h"

void YUVDepthConverter::Initialize(int width, int height)
{
	_width = width; 
	_height = height; 

	_depth2YUVContext = sws_getContext(width, height, 
		AVPixelFormat::AV_PIX_FMT_GRAY16LE, width, height, 
		AV_PIX_FMT_YUV420P12, SWS_BICUBIC, nullptr, nullptr, nullptr);

	_yuv2PNGContext = sws_getContext(width, height, 
		AVPixelFormat::AV_PIX_FMT_YUV420P12, width, height, 
		AV_PIX_FMT_GRAY16LE, SWS_BICUBIC, nullptr, nullptr, nullptr);

	_yuvBufSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P12, _width, _height, 1);
}

array<UInt16>^ YUVDepthConverter::YUV2Depth(AVFrame* yuv)
{
	array<UInt16>^ ret = gcnew array<UInt16>(_width * _height); 

	pin_ptr<UInt16> retPtr = &ret[0]; 

	int depthLineSize[1] = { 2 * _width };
	int scaleRet = sws_scale(_yuv2PNGContext, yuv->data, yuv->linesize, 0,
		_height, (uint8_t* const*)&retPtr, depthLineSize);
	if (scaleRet == _height)
	{
		return ret; 
	}

	return ret;
}

AVFrame* YUVDepthConverter::Depth2YUV(array<UInt16>^ depth)
{
	AVFrame* yuvFrame = av_frame_alloc();
	yuvFrame->height = _height;
	yuvFrame->width = _width;
	yuvFrame->format = AV_PIX_FMT_YUV420P12;

	auto* yuvBuf = (uint8_t*)av_malloc(_yuvBufSize);

	int ret = av_image_fill_arrays(yuvFrame->data,
		yuvFrame->linesize, yuvBuf, AV_PIX_FMT_YUV420P12, _width, _height, 1);

	if (ret > 0) 
	{
		int depthLineSize[1] = { 2 * _width };
		pin_ptr<unsigned short> depthPtr = &depth[0];

		int scaleRet = sws_scale(_depth2YUVContext, (const uint8_t* const*)&depthPtr, depthLineSize, 0,
			_height, yuvFrame->data, yuvFrame->linesize);

		if (scaleRet == _height)
		{
			return yuvFrame; 
		}
	}

	av_frame_free(&yuvFrame); 
	av_free(yuvBuf); 

	return nullptr; 
}

YUVDepthConverter::~YUVDepthConverter()
{
	if (_depth2YUVContext)
	{
		sws_freeContext(_depth2YUVContext); 
		_depth2YUVContext = nullptr; 
	}

	if (_yuv2PNGContext)
	{
		sws_freeContext(_yuv2PNGContext); 
		_yuv2PNGContext = nullptr; 
	}
}