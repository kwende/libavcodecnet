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

	__declspec(dllexport) bool __cdecl Save16BitYChannelPNG(uint16_t* inputBuffer, int width, int height, const char* outputPath);
}