#include "handmade.h"

internal void
GameSoundOutput(game_sound_output_buffer *SoundBuffer, int32 ToneHz)
{
	local_persist real32 tSine;
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	int16 *SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
		real32 SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
		tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
	}
}

internal void
RenderWierdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
	uint8 *Row = (uint8 *)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer->Width; ++X)
		{
			uint8 Blue = (uint8)(X + XOffset);
			uint8 Red = (uint8)(Y + YOffset);

			*Pixel++ = (Red << 8 | Blue);
		}
		Row += Buffer->Pitch;
	}
}

internal void
GameUpdateAndRender(game_memory *Memory, game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer, game_input *Input)
{
	Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!Memory->IsInitialized)
	{
		//char *FileName = (char *) "test.bmp";

		char *FileName = (char *)__FILE__;
		debug_read_file_result File = DEBUGPlatformReadEntireFile(FileName);
		if (File.Contents)
		{
			DEBUGPlatformWriteEntireFile((char *)"data/test.out", File.ContentSize, File.Contents);
			DEBUGPlatformFreeFileMemory(File.Contents);
		}
		GameState->ToneHz = 256;
		Memory->IsInitialized = true;
	}


	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		if (Controller->IsAnalog)
		{
			GameState->ToneHz = 256 + (int32)(128.0f*(Controller->StickAverageX));
			GameState->BlueOffset += (int32)(4.0f*(Controller->StickAverageY));
		}
		else
		{
			if (Controller->MOVE_LEFT.EndedDown || Controller->ACTION_LEFT.EndedDown)
			{
				GameState->BlueOffset--;
			}
			if (Controller->MOVE_RIGHT.EndedDown || Controller->ACTION_RIGHT.EndedDown)
			{
				GameState->BlueOffset++;
			}
			if (Controller->MOVE_UP.EndedDown || Controller->ACTION_UP.EndedDown)
			{
				GameState->GreenOffset++;
			}
			if (Controller->MOVE_DOWN.EndedDown || Controller->ACTION_DOWN.EndedDown)
			{
				GameState->GreenOffset--;
			}

		}
	}
	GameSoundOutput(SoundBuffer, GameState->ToneHz);
	RenderWierdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}