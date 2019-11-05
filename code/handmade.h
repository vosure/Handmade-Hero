#if !defined(HANDMADE_H)

#include <math.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#if HANDMADE_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

struct thread_context
{
	int32 Placeholder;
};

#if HANDMADE_INTERNAL
/* IMPORTANT(casey):

   These are NOT for doing anything in the shipping game - they are
   blocking and the write doesn't protect against lost data!
*/
struct debug_read_file_result
{
    uint32 ContentSize;
    void *Contents;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context *Thread, char *Filename, uint32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define ArrayCount(Array) sizeof(Array) / sizeof((Array)[0])

inline uint32
SafeTruncateUInt64(uint64 Value)
{
	Assert(Value <= 0xFFFFFFFFFF);
	uint32 Result = (uint32)Value;
	return(Result);
}

struct game_offscreen_buffer
{
	void *Memory;
	int32 Width;
	int32 Height;
	int32 Pitch;
	int32 BytesPerPixel;
};

struct game_sound_output_buffer
{
	int32 SamplesPerSecond;
	int32 SampleCount;
	int16 *Samples;
};

struct game_button_state
{
	int32 HalfTransitionCount;
	bool32 EndedDown;
};

struct game_controller_input
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

			// 

			game_button_state Terminator;
		};
	};
};

struct game_input
{
	game_button_state MouseButtons[5];
	int32 MouseX, MouseY, MouseZ;

	real32 dtForFrame;

	game_controller_input Controllers[5];
};

inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));

	game_controller_input *Result = &Input->Controllers[ControllerIndex];
	return(Result);
}

struct game_memory
{
	bool32 IsInitialized;

	uint64 PermanentStorageSize;
	void* PermanentStorage;

	uint64 TransientStorageSize;
	void* TransientStorage;

    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
	
};

struct game_state
{
	real32 PlayerX;
	real32 PlayerY;
};

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, game_offscreen_buffer *Buffer, game_input *Input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory *Memory, game_sound_output_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#define HANDMADE_H
#endif