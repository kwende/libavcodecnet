#pragma once

#include <msclr/marshal.h>
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

using namespace System; 

namespace libavcodecnet {

	public ref class ColorSpaceConverter
	{
	public:
		ColorSpaceConverter();
		void Save16BitYChannelPNG(array<UInt16>^ frameData, int width, int height, String^ destinationPath);
		void Convert16Bit2YChannelPNG(String^ inputPath, int width, int height, String^ destinationPath);
	};
}

