#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef size_t memory_index;

typedef float real32;
typedef double real64;

typedef struct thread_context
{
	int32 Placeholder;
} thread_context;

#if HANDMADE_INTERNAL
typedef struct debug_read_file_result
{
    uint32 ContentSize;
    void *Contents;
} debug_read_file_result;

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context *Thread, char *Filename, uint32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif

typedef struct game_offscreen_buffer
{
	void *Memory;
	int32 Width;
	int32 Height;
	int32 Pitch;
	int32 BytesPerPixel;
} game_offscreen_buffer;

typedef struct game_sound_output_buffer
{
	int32 SamplesPerSecond;
	int32 SampleCount;
	int16 *Samples;
} game_sound_output_buffer;

typedef struct game_button_state
{
	int32 HalfTransitionCount;
	bool32 EndedDown;
} game_button_state;

typedef struct game_controller_input
{
	bool32 IsConnected;
	bool32 IsAnalog;
	real32 StickAverageX;
	real32 StickAverageY;

	union
	{
		game_button_state Buttons[12];
		struct
		{
			game_button_state MOVE_UP;
			game_button_state MOVE_DOWN;
			game_button_state MOVE_RIGHT;
			game_button_state MOVE_LEFT;

			game_button_state ACTION_UP;
			game_button_state ACTION_DOWN;
			game_button_state ACTION_RIGHT;
			game_button_state ACTION_LEFT;

			game_button_state LEFT_SHOULDER;
			game_button_state RIGHT_SHOULDER;

			game_button_state BACK;
			game_button_state START;

			game_button_state Terminator;
		};
	};
} game_controller_input;

typedef struct game_input
{
	game_button_state MouseButtons[5];
	int32 MouseX, MouseY, MouseZ;

	real32 dtForFrame;

	game_controller_input Controllers[5];
} game_input;

typedef struct game_memory
{
	bool32 IsInitialized;

	uint64 PermanentStorageSize;
	void* PermanentStorage;

	uint64 TransientStorageSize;
	void* TransientStorage;

    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
} game_memory;

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, game_offscreen_buffer *Buffer, game_input *Input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory *Memory, game_sound_output_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#ifdef __cplusplus
}
#endif