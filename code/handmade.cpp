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

inline tile_map *
GetTileMap(world *World, int32 TileMapX, int32 TileMapY)
{
    tile_map *TileMap = 0;
    if ((TileMapX >= 0) && (TileMapX < World->TileMapCountX) &&
        (TileMapY >= 0) && (TileMapY < World->TileMapCountY))
    {
        TileMap = &World->TileMaps[TileMapY * World->TileMapCountX + TileMapX];
    }
    return (TileMap);
}

inline uint32
GetTileValueUnchecked(world *World, tile_map *TileMap, int32 TileX, int32 TileY)
{
    Assert(TileMap);
    Assert((TileX >= 0) && (TileX < World->CountX) &&
           (TileY >= 0) && (TileY < World->CountY));
    uint32 TileMapValue = TileMap->Tiles[TileY * World->CountX + TileX];
    return (TileMapValue);
}

internal bool32
IsTileMapPointEmpty(world *World, tile_map *TileMap, int32 TestTileX, int32 TestTileY)
{
    bool32 Empty = false;

    if (TileMap)
    {
        if ((TestTileX >= 0) && (TestTileX < World->CountX) &&
            (TestTileY >= 0) && (TestTileY < World->CountY))
        {
            uint32 TileMapValue = GetTileValueUnchecked(World, TileMap, TestTileX, TestTileY);
            Empty = (TileMapValue == 0);
        }
    }
    return (Empty);
}

inline void
RecanonicalizeCoord(world *World, int32 TileCount, int32 *TileMap, int32 *Tile, real32 *TileRelative)
{
    int32 Offset = FloorReal32ToInt32(*TileRelative / World->TileSideInMeters);
    *Tile += Offset;
    *TileRelative -= Offset * World->TileSideInMeters;

    Assert(*TileRelative >= 0);
    Assert(*TileRelative < World->TileSideInMeters);

    if (*Tile < 0)
    {
        *Tile = TileCount + *Tile;
        --*TileMap;
    }
    if (*Tile >= TileCount)
    {
        *Tile = *Tile - TileCount;
        ++*TileMap;
    }
}

inline canonical_position
RecanonicalizePosition(world *World, canonical_position Position)
{
    canonical_position Result = Position;

    RecanonicalizeCoord(World, World->CountX, &Result.TileMapX, &Result.TileX, &Result.TileRelativeX);
    RecanonicalizeCoord(World, World->CountY, &Result.TileMapY, &Result.TileY, &Result.TileRelativeY);

    return (Result);
}

internal bool32
IsWorldPointEmpty(world *World, canonical_position CanonicalPosition)
{
    bool32 Empty = false;

    tile_map *TileMap = GetTileMap(World, CanonicalPosition.TileMapX, CanonicalPosition.TileMapY);
    Empty = IsTileMapPointEmpty(World, TileMap, CanonicalPosition.TileX, CanonicalPosition.TileY);

    return (Empty);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9
    uint32 Tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
        {
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
            {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

    uint32 Tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
        {
            {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};
    uint32 Tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
        {
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}};
    uint32 Tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
        {
            {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

    tile_map TileMaps[2][2];
    TileMaps[0][0].Tiles = (uint32 *)Tiles00;
    TileMaps[0][1].Tiles = (uint32 *)Tiles10;
    TileMaps[1][0].Tiles = (uint32 *)Tiles01;
    TileMaps[1][1].Tiles = (uint32 *)Tiles11;

    world World;
    World.TileMapCountX = 2;
    World.TileMapCountY = 2;
    World.CountX = TILE_MAP_COUNT_X;
    World.CountY = TILE_MAP_COUNT_Y;
    World.TileSideInMeters = 1.4f;
    World.TileSideInPixels = 60;
    World.MetersToPixels = (real32)(World.TileSideInPixels) / (real32)(World.TileSideInMeters);
    World.UpperLeftX = -(real32)World.TileSideInPixels / 2;
    World.UpperLeftY = 0;
    World.TileMaps = (tile_map *)TileMaps;

    real32 PlayerHeight = 1.4f;
    real32 PlayerWidth = 0.75f * PlayerHeight;

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        GameState->PlayerPosition.TileMapX = 0;
        GameState->PlayerPosition.TileMapY = 0;
        GameState->PlayerPosition.TileRelativeX = 5.0f;
        GameState->PlayerPosition.TileRelativeY = 5.0f;
        GameState->PlayerPosition.TileX = 3;
        GameState->PlayerPosition.TileY = 3;

        Memory->IsInitialized = true;
    }

    tile_map *TileMap = GetTileMap(&World, GameState->PlayerPosition.TileMapX, GameState->PlayerPosition.TileMapY);
    Assert(TileMap);

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
                dPlayerY = -1.0f;
            }
            if (Controller->MOVE_DOWN.EndedDown)
            {
                dPlayerY = 1.0f;
            }
            if (Controller->MOVE_LEFT.EndedDown)
            {
                dPlayerX = -1.0f;
            }
            if (Controller->MOVE_RIGHT.EndedDown)
            {
                dPlayerX = 1.0f;
            }

            dPlayerX *= 10.0f;
            dPlayerY *= 10.0f;

            canonical_position NewPlayerPosition = GameState->PlayerPosition;
            NewPlayerPosition.TileRelativeX += Input->dtForFrame * dPlayerX;
            NewPlayerPosition.TileRelativeY += Input->dtForFrame * dPlayerY;
            NewPlayerPosition = RecanonicalizePosition(&World, NewPlayerPosition);

            canonical_position PlayerLeft = NewPlayerPosition;
            PlayerLeft.TileRelativeX -= 0.5f * PlayerWidth;
            PlayerLeft = RecanonicalizePosition(&World, PlayerLeft);

            canonical_position PlayerRight = NewPlayerPosition;
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

    for (int32 Row = 0; Row < TILE_MAP_COUNT_Y; Row++)
    {
        for (int32 Column = 0; Column < TILE_MAP_COUNT_X; Column++)
        {
            uint32 TileID = GetTileValueUnchecked(&World, TileMap, Column, Row);
            real32 Gray = 0.5f;
            if (TileID == 1)
            {
                Gray = 1.0f;
            }

            real32 MinX = World.UpperLeftX + ((real32)Column) * World.TileSideInPixels;
            real32 MinY = World.UpperLeftY + ((real32)Row) * World.TileSideInPixels;
            real32 MaxX = MinX + World.TileSideInPixels;
            real32 MaxY = MinY + World.TileSideInPixels;
            DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;

    real32 PlayerLeft = World.UpperLeftX + World.TileSideInPixels * GameState->PlayerPosition.TileX +
                        World.MetersToPixels * GameState->PlayerPosition.TileRelativeX - 0.5f * PlayerWidth * World.MetersToPixels;
    real32 PlayerTop = World.UpperLeftY + World.TileSideInPixels * GameState->PlayerPosition.TileY +
                       World.MetersToPixels * GameState->PlayerPosition.TileRelativeY - PlayerHeight * World.MetersToPixels;
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
