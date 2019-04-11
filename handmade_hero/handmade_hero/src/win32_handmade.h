#pragma once

#pragma comment(lib, "winmm.lib")

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void *Memory;
	int32 Width;
	int32 Height;
	int32 Pitch;
	int32 BytesPerPixel;
};

struct win32_window_dimension
{
	int32 Width;
	int32 Height;
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
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;

	DWORD OutputLocation;
	DWORD OutputByteCount;

	DWORD ExpectedFlipPlayCursor;
	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor;
};