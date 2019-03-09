#pragma once

#pragma comment(lib, "winmm.lib")

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

struct win32_sound_output
{
	int32 SamplesPerSecond;
	int32 ToneHz;
	int32 ToneVolume;
	uint32 RunningSampleIndex;
	int32 BytesPerSample;
	int32 SecondaryBufferSize;
	real32 tSine;
	int32 LatencySampleCount;
};