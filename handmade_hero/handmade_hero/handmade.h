#if !defined(HANDMADE_H)

struct game_offscreen_buffer
{
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer);

#define HANDMADE_H
#endif