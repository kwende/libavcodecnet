#pragma once
#include <msclr/marshal.h>

using namespace System;
using namespace msclr::interop;

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

#include <string>
#include <iostream>

namespace libavcodecnet {

	struct NativePointers16
	{
		std::string fileName; 
		AVCodecContext* codecCtx = nullptr;
		AVFormatContext* formatCtx = nullptr;
		AVCodec* codec = nullptr;
		AVFrame* videoFrame = nullptr;
		AVOutputFormat* oformat = nullptr;
		SwsContext* swsContext = nullptr; 
		AVFrame* avFrame = nullptr; 
	};

	public ref class Recorder16 : IDisposable
	{
	public:
		static Recorder16^ Create(String^ filename, int width, int height, int fps, int crf);
		void WriteFrame(array<UInt16>^ frameData);
		array<byte>^ WriteAndReturnFrame(array<UInt16>^ frameData); 
		void Close();
		~Recorder16();
	private:
		Recorder16(std::string fileName, int width, int height, int fps, int crf);
		bool Initialize();
		bool InitializeDecoder();

		NativePointers16* _nativePointers;
		int _fps;
		int _width, _height, _frameCounter, _crf;
		bool _flushed = false;

		const AVPixelFormat DestFormat = AV_PIX_FMT_YUV420P12;
		//const AVPixelFormat DestFormat = AV_PIX_FMT_GRAY12LE; 
		//const AVPixelFormat DestFormat = AV_PIX_FMT_GRAY16LE;

		////// decoder shit
		const AVCodec* _decoder; 
		AVCodecContext* _decoderContext; 

		const AVCodec* _pngEncoder; 
		AVCodecContext* _pngEncoderContext; 
	};
}

