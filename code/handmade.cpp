#include "handmade.h"
#include "handmade_intrinsics.h"

internal void
GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    int16 ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16 *SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
#if 0
        real32 SineValue = sinf(GameState->tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
        int16 SampleValue = 0;
#endif
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
#if 0
        GameState->tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
        if (GameState->tSine > 2.0f * Pi32)
        {
            GameState->tSine -= 2.0f * Pi32;
        }
#endif
    }
}

internal void
DrawRectangle(game_offscreen_buffer *Buffer,
              real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY,
              real32 R, real32 G, real32 B)
{
    int32 MinX = RoundReal32ToInt32(RealMinX);
    int32 MinY = RoundReal32ToInt32(RealMinY);
    int32 MaxX = RoundReal32ToInt32(RealMaxX);
    int32 MaxY = RoundReal32ToInt32(RealMaxY);

    if (MinX < 0)
    {
        MinX = 0;
    }
    if (MinY < 0)
    {
        MinY = 0;
    }
    if (MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    if (MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint8 *Row = ((uint8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);

    for (int32 Y = MinY; Y < MaxY; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int32 X = MinX; X < MaxX; ++X)
        {
            *Pixel++ = (RoundReal32ToUInt32(R * 255.0f) << 16) |
                       (RoundReal32ToUInt32(G * 255.0f) << 8) |
                       (RoundReal32ToUInt32(B * 255.0f) << 0);
        }
        Row += Buffer->Pitch;
    }
}

inline tile_chunk *
GetTileChunk(world *World, int32 TileChunkX, int32 TileChunkY)
{
    tile_chunk *TileChunk = 0;

    if ((TileChunkX >= 0) && (TileChunkX < World->TileChunkCountX) &&
        (TileChunkY >= 0) && (TileChunkY < World->TileChunkCountY))
    {
        TileChunk = &World->TileChunks[TileChunkY * World->TileChunkCountX + TileChunkX];
    }
    return (TileChunk);
}

inline uint32
GetTileValueUnchecked(world *World, tile_chunk *TileChunk, uint32 TileX, uint32 TileY)
{
    Assert(TileChunk);
    Assert(TileX < World->ChunkDim);
    Assert(TileY < World->ChunkDim);
    uint32 TileChunkValue = TileChunk->Tiles[TileY * World->ChunkDim + TileX];
    return (TileChunkValue);
}

internal bool32
GetTileValue(world *World, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY)
{
    uint32 TileChunkValue = 0;

    if (TileChunk)
    {
        TileChunkValue = GetTileValueUnchecked(World, TileChunk, TestTileX, TestTileY);
    }

    return (TileChunkValue);
}

inline void
RecanonicalizeCoord(world *World, uint32 *Tile, real32 *TileRelative)
{
    int32 Offset = FloorReal32ToInt32(*TileRelative / World->TileSideInMeters);
    *Tile += Offset;
    *TileRelative -= Offset * World->TileSideInMeters;

    Assert(*TileRelative >= 0);
    Assert(*TileRelative <= World->TileSideInMeters);
}

inline world_position
RecanonicalizePosition(world *World, world_position Position)
{
    world_position Result = Position;

    RecanonicalizeCoord(World, &Result.AbsoluteTileX, &Result.TileRelativeX);
    RecanonicalizeCoord(World, &Result.AbsoluteTileY, &Result.TileRelativeY);

    return (Result);
}

inline tile_chunk_position
GetChunkPositionFor(world *World, uint32 AbsoluteTileX, uint32 AbsoluteTileY)
{
    tile_chunk_position Result;

    Result.TileChunkX = AbsoluteTileX >> World->ChunkShift;
    Result.TileChunkY = AbsoluteTileY >> World->ChunkShift;
    Result.RelativeTileX = AbsoluteTileX & World->ChunkMask;
    Result.RelativeTileY = AbsoluteTileY & World->ChunkMask;

    return (Result);
}

internal uint32
GetTileValue(world *World, uint32 AbsoluteTileX, uint32 AbsoluteTileY)
{
    bool32 Empty = false;

    tile_chunk_position ChunkPosition = GetChunkPositionFor(World, AbsoluteTileX, AbsoluteTileY);
    tile_chunk *TileChunk = GetTileChunk(World, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY);
    uint32 TileChunkValue = GetTileValue(World, TileChunk, ChunkPosition.RelativeTileX, ChunkPosition.RelativeTileY);

    return (TileChunkValue);
}

internal bool32
IsWorldPointEmpty(world *World, world_position CanonicalPosition)
{
    uint32 TileChunkValue = GetTileValue(World, CanonicalPosition.AbsoluteTileX, CanonicalPosition.AbsoluteTileY);
    bool32 Empty = (TileChunkValue == 0);

    return (Empty);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

#define TILE_MAP_COUNT_X 256
#define TILE_MAP_COUNT_Y 256
    uint32 TempTiles[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
        {
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

    world World;
    World.ChunkShift = 8;
    World.ChunkMask = 0xFF;
    World.ChunkDim = 256;
    World.TileChunkCountX = 1;
    World.TileChunkCountY = 1;

    tile_chunk TileChunk;
    TileChunk.Tiles = (uint32 *)TempTiles;
    World.TileChunks = &TileChunk;

    World.TileSideInMeters = 1.4f;
    World.TileSideInPixels = 60;
    World.MetersToPixels = (real32)(World.TileSideInPixels) / (real32)(World.TileSideInMeters);

    real32 PlayerHeight = 1.4f;
    real32 PlayerWidth = 0.75f * PlayerHeight;

    real32 LowerLeftX = -(real32)World.TileSideInPixels / 2;
    real32 LowerLeftY = (real32)Buffer->Height;

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        GameState->PlayerPosition.AbsoluteTileX = 3;
        GameState->PlayerPosition.AbsoluteTileY = 3;
        GameState->PlayerPosition.TileRelativeX = 5.0f;
        GameState->PlayerPosition.TileRelativeY = 5.0f;

        Memory->IsInitialized = true;
    }

    for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if (Controller->IsAnalog)
        {
        }
        else
        {
            real32 dPlayerX = 0.0f;
            real32 dPlayerY = 0.0f;

            if (Controller->MOVE_UP.EndedDown)
            {
                dPlayerY = 1.0f;
            }
            if (Controller->MOVE_DOWN.EndedDown)
            {
                dPlayerY = -1.0f;
            }
            if (Controller->MOVE_LEFT.EndedDown)
            {
                dPlayerX = -1.0f;
            }
            if (Controller->MOVE_RIGHT.EndedDown)
            {
                dPlayerX = 1.0f;
            }

            dPlayerX *= 5.0f;
            dPlayerY *= 5.0f;

            world_position NewPlayerPosition = GameState->PlayerPosition;
            NewPlayerPosition.TileRelativeX += Input->dtForFrame * dPlayerX;
            NewPlayerPosition.TileRelativeY += Input->dtForFrame * dPlayerY;
            NewPlayerPosition = RecanonicalizePosition(&World, NewPlayerPosition);

            world_position PlayerLeft = NewPlayerPosition;
            PlayerLeft.TileRelativeX -= 0.5f * PlayerWidth;
            PlayerLeft = RecanonicalizePosition(&World, PlayerLeft);

            world_position PlayerRight = NewPlayerPosition;
            PlayerRight.TileRelativeX += 0.5f * PlayerWidth;
            PlayerRight = RecanonicalizePosition(&World, PlayerRight);

            if (IsWorldPointEmpty(&World, NewPlayerPosition) &&
                IsWorldPointEmpty(&World, PlayerLeft) &&
                IsWorldPointEmpty(&World, PlayerRight))
            {
                GameState->PlayerPosition = NewPlayerPosition;
            }
        }
    }
    DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 1.0f, 0.0f, 1.0f);

    real32 CenterX = (real32)Buffer->Width * 0.5f;
    real32 CenterY = (real32)Buffer->Height * 0.5f;

    for (int32 RelativeRow = -10; RelativeRow < 10; RelativeRow++)
    {
        for (int32 RelativeColumn = -20; RelativeColumn < 20; RelativeColumn++)
        {
            uint32 Column = RelativeColumn + GameState->PlayerPosition.AbsoluteTileX;
            uint32 Row = RelativeRow + GameState->PlayerPosition.AbsoluteTileY;

            uint32 TileID = GetTileValue(&World, Column, Row);
            real32 Gray = 0.5f;
            if (TileID == 1)
            {
                Gray = 1.0f;
            }
            if ((Column == GameState->PlayerPosition.AbsoluteTileX) &&
                (Row == GameState->PlayerPosition.AbsoluteTileY))
            {
                Gray = 0.0f;
            }

            real32 MinX = CenterX + ((real32)RelativeColumn) * World.TileSideInPixels;
            real32 MinY = CenterY - ((real32)RelativeRow) * World.TileSideInPixels;
            real32 MaxX = MinX + World.TileSideInPixels;
            real32 MaxY = MinY - World.TileSideInPixels;
            DrawRectangle(Buffer, MinX, MaxY, MaxX, MinY, Gray, Gray, Gray);
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;

    real32 PlayerLeft = CenterX + World.MetersToPixels * GameState->PlayerPosition.TileRelativeX - 0.5f * PlayerWidth * World.MetersToPixels;
    real32 PlayerTop = CenterY - World.MetersToPixels * GameState->PlayerPosition.TileRelativeY - PlayerHeight * World.MetersToPixels;
    DrawRectangle(Buffer, PlayerLeft, PlayerTop,
                  PlayerLeft + PlayerWidth * World.MetersToPixels,
                  PlayerTop + PlayerHeight * World.MetersToPixels,
                  PlayerR, PlayerG, PlayerB);

    //DrawRectangle(Buffer, 10.0f, 10.0f, 40.0f, 40.0f, 0.0f, 1.0f, 1.0f);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer, 400);
}

/*
internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{

    uint8 *Row = (uint8 *)Buffer->Memory;
    for (int Y = 0;
         Y < Buffer->Height;
         ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0;
             X < Buffer->Width;
             ++X)
        {
            uint8 Blue = (uint8)(X + BlueOffset);
            uint8 Green = (uint8)(Y + GreenOffset);

            *Pixel++ = ((Green << 16) | Blue);
        }

        Row += Buffer->Pitch;
    }
}
*/
