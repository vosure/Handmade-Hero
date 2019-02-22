#if !defined(HANDMADE_H)

#if HANDMADE_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024L)
#define Megabytes(Value) (Kilobytes(Value)*1024L)
#define Gigabytes(Value) (Megabytes(Value)*1024L)
#define Terabytes(Value) (Gigabytes(Value)*1024L)

#define ArrayCount(Array) sizeof(Array) / sizeof((Array)[0])

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
	bool32 IsAnalog;

	real32 StartX;
	real32 StartY;

	real32 MinX;
	real32 MinY;

	real32 MaxX;
	real32 MaxY;

	real32 EndX;
	real32 EndY;

	union
	{
		game_button_state Buttons[6];
		struct
		{
			game_button_state UP;
			game_button_state DOWN;
			game_button_state RIGHT;
			game_button_state LEFT;
			game_button_state LEFT_SHOULDER;
			game_button_state RIGHT_SHOULDER;
		};
	};
};

struct game_input
{
	game_controller_input Controllers[4];
};

struct game_memory
{
	bool32 IsInitialized;

	uint64 PermanentStorageSize;
	void* PermamentStrorage;

	uint64 TransientStorageSize;
	void* TransientStrorage;
};

struct game_state
{
	int BlueOffset;
	int GreenOffset;
	int32 ToneHz;
};

internal void GameUpdateAndRender(game_memory *Memory, game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer, game_input *Input);

#define HANDMADE_H
#endif