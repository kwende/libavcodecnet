#pragma once
#include <msclr/marshal.h>

using namespace System;
using namespace msclr::interop;

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <string>

namespace libavcodecnet {

	struct NativePointers
	{
		AVCodecContext* cctx = nullptr;
		SwsContext* swsctx = nullptr; 
		AVFormatContext* ofctx = nullptr;
		AVFrame* videoFrame = nullptr; 
		std::string* fileName = nullptr; 
		AVOutputFormat* oformat = nullptr; 
		uint8_t* scratchData = nullptr; 
	};

	public ref class Recorder
	{
	public:
		static Recorder^ Create(String^ filename, int width, int height, int fps, int bitrate);
		void WriteFrame(array<Byte>^ frameData); 
		void Close(); 
	private:
		Recorder(std::string fileName, int width, int height, int fps, int bitrate); 
		void Initialize(); 

		NativePointers* _nativePointers; 
		int _width, _height, _fps, _bitrate, _frameCounter; 
	};
}
