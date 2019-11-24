#pragma once

struct tile_map_position
{
    uint32 AbsoluteTileX;
    uint32 AbsoluteTileY;
    uint32 AbsoluteTileZ;

    real32 TileRelativeX;
    real32 TileRelativeY;
};

struct tile_chunk_position
{
    uint32 TileChunkX;
    uint32 TileChunkY;
    uint32 TileChunkZ;

    uint32 RelativeTileX;
    uint32 RelativeTileY;
};

struct tile_chunk
{
    uint32 *Tiles;
};

struct tile_map
{
    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;

    real32 TileSideInMeters;

    uint32 TileChunkCountX;
    uint32 TileChunkCountY;
    uint32 TileChunkCountZ;

    tile_chunk *TileChunks;
};