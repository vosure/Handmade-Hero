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
			uint8 Blue = X + XOffset;
			uint8 Red = Y + YOffset;

			*Pixel++ = (Red << 16 | Blue);
		}
		Row += Buffer->Pitch;
	}
}

internal void
GameUpdateAndRender(game_memory *Memory, game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer, game_input *Input)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state *)Memory->PermamentStrorage;
	if (!Memory->IsInitialized)
	{
		GameState->ToneHz = 256;
		GameState->GreenOffset = 0;
		GameState->BlueOffset = 0;

		Memory->IsInitialized = true;
	}

	game_controller_input *Input0 = &Input->Controllers[0];
	if (Input0->IsAnalog)
	{
		GameState->ToneHz = 256 + (int)(128.0f*(Input0->EndX));
		GameState->BlueOffset += (int)4.0f*(Input0->EndY);
	}
	else
	{
	}
	if (Input0->DOWN.EndedDown)
	{
		GameState->GreenOffset++;
	}
	GameSoundOutput(SoundBuffer, GameState->ToneHz);
	RenderWierdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}