#include "handmade.h"

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
GameUpdateAndRender(game_offscreen_buffer *Buffer)
{
	int BlueOffset = 0;
	int GreenOffset = 0;
	RenderWierdGradient(Buffer, BlueOffset, GreenOffset);
}