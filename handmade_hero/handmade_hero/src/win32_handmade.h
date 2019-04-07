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
	uint32 RunningSampleIndex;
	int32 BytesPerSample;
	uint32 SecondaryBufferSize;
	DWORD SafetyBytes;
	real32 tSine;
	int32 LatencySampleCount;
};

struct win32_debug_time_marker
{
	DWORD PlayCursor;
	DWORD WriteCursor;
};