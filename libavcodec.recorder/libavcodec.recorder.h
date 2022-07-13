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

	public ref class Recorder : IDisposable
	{
	public:
		static Recorder^ Create(String^ filename, int width, int height, float fps, int crf);
		void WriteFrame(array<Byte>^ frameData); 
		void Close();
		~Recorder();
	private:
		Recorder(std::string fileName, int width, int height, float fps, int crf); 
		void Initialize(); 

		NativePointers* _nativePointers; 
		float _fps; 
		int _width, _height, _frameCounter, _crf;
		bool _flushed = false; 
	};
}
