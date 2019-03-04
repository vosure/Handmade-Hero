#if !defined(HANDMADE_H)

#if HANDMADE_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#if HANDMADE_INTERNAL
struct debug_read_file_result
{
	uint32 ContentSize;
	void *Contents;

};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *FileName);
internal void DEBUGPlatformFreeFileMemory(void *Memory);
internal bool32 DEBUGPlatformWriteEntireFile(char *FileName, uint32 MemorySize, void *Memory);
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
	int Width;
	int Height;
	int Pitch;
};

struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
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
};

struct game_state
{
	int32 BlueOffset;
	int32 GreenOffset;
	int32 ToneHz;
};

internal void GameUpdateAndRender(game_memory *Memory, game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer, game_input *Input);

#define HANDMADE_H
#endif