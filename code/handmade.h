#pragma once

#include "handmade_platform.h"

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

#if HANDMADE_SLOW
#define Assert(Expression) \
    if (!(Expression))     \
    {                      \
        *(int *)0 = 0;     \
    }
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)
#define ArrayCount(Array) sizeof(Array) / sizeof((Array)[0])

inline uint32
SafeTruncateUInt64(uint64 Value)
{
    Assert(Value <= 0xFFFFFFFFFF);
    uint32 Result = (uint32)Value;
    return (Result);
}

inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));

    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return (Result);
}

struct canonical_position
{
#if 1
    int32 TileMapX;
    int32 TileMapY;

    int32 TileX;
    int32 TileY;
#else
    uint32 Tilex;
    uint32 TileY;
#endif
    real32 TileRelativeX;
    real32 TileRelativeY;
};

struct tile_map
{
    uint32 *Tiles;
};

struct world
{
    real32 TileSideInMeters;
    int32 TileSideInPixels;
    real32 MetersToPixels;

    int32 CountX;
    int32 CountY;

    real32 UpperLeftX;
    real32 UpperLeftY;

    int32 TileMapCountX;
    int32 TileMapCountY;

    tile_map *TileMaps;
};

struct game_state
{
    canonical_position PlayerPosition;
};
