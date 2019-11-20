#include "handmade.h"

#include "handmade_tile.cpp"

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

internal void
InitializeArena(memory_arena *Arena, memory_index Size, uint8 *Base)
{
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count) * sizeof(type))
void *
PushSize_(memory_arena *Arena, memory_index Size)
{
    Assert((Arena->Used + Size) < Arena->Size);
    void *Result = Arena->Base + Arena->Used;
    Arena->Used += Size;

    return (Result);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    real32 PlayerHeight = 1.4f;
    real32 PlayerWidth = 0.75f * PlayerHeight;

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        GameState->PlayerPosition.AbsoluteTileX = 1;
        GameState->PlayerPosition.AbsoluteTileY = 3;
        GameState->PlayerPosition.TileRelativeX = 5.0f;
        GameState->PlayerPosition.TileRelativeY = 5.0f;

        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), (uint8 *)Memory->PermanentStorage + sizeof(game_state));

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        World->TileMap = PushStruct(&GameState->WorldArena, tile_map);

        tile_map *TileMap = World->TileMap;

        TileMap->ChunkShift = 8;
        TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
        TileMap->ChunkDim = 256;

        TileMap->TileChunkCountX = 4;
        TileMap->TileChunkCountY = 4;

        TileMap->TileChunks = PushArray(&GameState->WorldArena, TileMap->TileChunkCountX * TileMap->TileChunkCountY, tile_chunk);

        for (uint32 Y = 0; Y < TileMap->TileChunkCountY; Y++)
        {
            for (uint32 X = 0; X < TileMap->TileChunkCountX; X++)
            {
                TileMap->TileChunks[Y * TileMap->TileChunkCountX + X].Tiles = PushArray(&GameState->WorldArena, TileMap->ChunkDim * TileMap->ChunkDim, uint32);
            }
        }

        TileMap->TileSideInMeters = 1.4f;
        TileMap->TileSideInPixels = 60;
        TileMap->MetersToPixels = (real32)(TileMap->TileSideInPixels) / (real32)(TileMap->TileSideInMeters);

        real32 LowerLeftX = -(real32)TileMap->TileSideInPixels / 2;
        real32 LowerLeftY = (real32)Buffer->Height;

        uint32 TilesPerWidth = 17;
        uint32 TilesPerHeight = 9;
        for (uint32 ScreenY = 0; ScreenY < 32; ScreenY++)
        {
            for (uint32 ScreenX = 0; ScreenX < 32; ScreenX++)
            {
                for (uint32 TileY = 0; TileY < TilesPerWidth; TileY++)
                {
                    for (uint32 TileX = 0; TileX < TilesPerHeight; TileX++)
                    {
                        uint32 AbsoluteTileX = ScreenX * TilesPerWidth + TileX;
                        uint32 AbsoluteTileY = ScreenY * TilesPerHeight + TileY;

                        SetTileValue(&GameState->WorldArena, World->TileMap, AbsoluteTileX, AbsoluteTileY, (TileX == TileY) && (TileY % 2) ? 1 : 0);
                    }
                }
            }
        }

        Memory->IsInitialized = true;
    }

    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;

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
            real32 PlayerSpeed = 5.0f;
            if (Controller->ACTION_UP.EndedDown)
            {
                PlayerSpeed = 10.0f;
            }

            dPlayerX *= PlayerSpeed;
            dPlayerY *= PlayerSpeed;

            tile_map_position NewPlayerPosition = GameState->PlayerPosition;
            NewPlayerPosition.TileRelativeX += Input->dtForFrame * dPlayerX;
            NewPlayerPosition.TileRelativeY += Input->dtForFrame * dPlayerY;
            NewPlayerPosition = RecanonicalizePosition(TileMap, NewPlayerPosition);

            tile_map_position PlayerLeft = NewPlayerPosition;
            PlayerLeft.TileRelativeX -= 0.5f * PlayerWidth;
            PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);

            tile_map_position PlayerRight = NewPlayerPosition;
            PlayerRight.TileRelativeX += 0.5f * PlayerWidth;
            PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);

            if (IsTileMapPointEmpty(TileMap, NewPlayerPosition) &&
                IsTileMapPointEmpty(TileMap, PlayerLeft) &&
                IsTileMapPointEmpty(TileMap, PlayerRight))
            {
                GameState->PlayerPosition = NewPlayerPosition;
            }
        }
    }
    DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 1.0f, 0.0f, 1.0f);

    real32 ScreenCenterX = (real32)Buffer->Width * 0.5f;
    real32 ScreenCenterY = (real32)Buffer->Height * 0.5f;

    for (int32 RelativeRow = -10; RelativeRow < 10; RelativeRow++)
    {
        for (int32 RelativeColumn = -20; RelativeColumn < 20; RelativeColumn++)
        {
            uint32 Column = RelativeColumn + GameState->PlayerPosition.AbsoluteTileX;
            uint32 Row = RelativeRow + GameState->PlayerPosition.AbsoluteTileY;

            uint32 TileID = GetTileValue(TileMap, Column, Row);
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

            real32 CenterX = ScreenCenterX - TileMap->MetersToPixels * GameState->PlayerPosition.TileRelativeX + ((real32)RelativeColumn) * TileMap->TileSideInPixels;
            real32 CenterY = ScreenCenterY + TileMap->MetersToPixels * GameState->PlayerPosition.TileRelativeY - ((real32)RelativeRow) * TileMap->TileSideInPixels;
            real32 MinX = CenterX - 0.5f * TileMap->TileSideInPixels;
            real32 MinY = CenterY - 0.5f * TileMap->TileSideInPixels;
            real32 MaxX = CenterX + 0.5f * TileMap->TileSideInPixels;
            real32 MaxY = CenterY + 0.5f * TileMap->TileSideInPixels;
            DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;

    real32 PlayerLeft = ScreenCenterX - 0.5f * PlayerWidth * TileMap->MetersToPixels;
    real32 PlayerTop = ScreenCenterY - PlayerHeight * TileMap->MetersToPixels;
    DrawRectangle(Buffer, PlayerLeft, PlayerTop,
                  PlayerLeft + PlayerWidth * TileMap->MetersToPixels,
                  PlayerTop + PlayerHeight * TileMap->MetersToPixels,
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
