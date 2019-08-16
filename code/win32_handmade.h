#pragma once

#pragma comment(lib, "Winmm.lib")

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

struct win32_game_code
{
	HMODULE GameCodeDLL;
	FILETIME DLLLastWriteTime;
	game_update_and_render *UpdateAndRender;
	game_get_sound_samples *GetSoundSamples;

	bool32 IsValid;
};

struct win32_state
{
	uint64 TotalSize;
	void *GameMemoryBlock;

	HANDLE RecordingHandle;
	int32 InputRecordingIndex;

	HANDLE PlayBackHandle;
	int32 InputPlayingIndex;
};