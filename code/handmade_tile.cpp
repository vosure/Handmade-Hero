inline tile_chunk *
GetTileChunk(tile_map *TileMap, uint32 TileChunkX, uint32 TileChunkY, uint32 TileChunkZ)
{
    tile_chunk *TileChunk = 0;

    if ((TileChunkX >= 0) && (TileChunkX < TileMap->TileChunkCountX) &&
        (TileChunkY >= 0) && (TileChunkY < TileMap->TileChunkCountY) &&
        (TileChunkZ >= 0) && (TileChunkZ < TileMap->TileChunkCountZ))
    {
        TileChunk = &TileMap->TileChunks[TileChunkZ * TileMap->TileChunkCountY * TileMap->TileChunkCountX +
                                         TileChunkY * TileMap->TileChunkCountX +
                                         TileChunkX];
    }
    return (TileChunk);
}

inline uint32
GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY)
{
    Assert(TileChunk);
    Assert(TileX < TileMap->ChunkDim);
    Assert(TileY < TileMap->ChunkDim);

    uint32 TileChunkValue = TileChunk->Tiles[TileY * TileMap->ChunkDim + TileX];

    return (TileChunkValue);
}

inline void
SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY, uint32 TileValue)
{
    Assert(TileChunk);
    Assert(TileX < TileMap->ChunkDim);
    Assert(TileY < TileMap->ChunkDim);

    TileChunk->Tiles[TileY * TileMap->ChunkDim + TileX] = TileValue;
}

internal uint32
GetTileValue(tile_map *TileMap, tile_chunk *TileChunk,
             uint32 TestTileX, uint32 TestTileY)
{
    uint32 TileChunkValue = 0;

    if (TileChunk && TileChunk->Tiles)
    {
        TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
    }

    return (TileChunkValue);
}

internal void
SetTileValue(tile_map *TileMap, tile_chunk *TileChunk,
             uint32 TestTileX, uint32 TestTileY,
             uint32 TileValue)
{
    if (TileChunk && TileChunk->Tiles)
    {
        SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
    }
}

inline tile_chunk_position
GetChunkPositionFor(tile_map *TileMap, uint32 AbsoluteTileX, uint32 AbsoluteTileY, uint32 AbsoluteTileZ)
{
    tile_chunk_position Result;

    Result.TileChunkX = AbsoluteTileX >> TileMap->ChunkShift;
    Result.TileChunkY = AbsoluteTileY >> TileMap->ChunkShift;
    Result.TileChunkZ = AbsoluteTileZ;
    Result.RelativeTileX = AbsoluteTileX & TileMap->ChunkMask;
    Result.RelativeTileY = AbsoluteTileY & TileMap->ChunkMask;

    return (Result);
}

internal uint32
GetTileValue(tile_map *TileMap, uint32 AbsoluteTileX, uint32 AbsoluteTileY, uint32 AbsoluteTileZ)
{
    tile_chunk_position ChunkPosition = GetChunkPositionFor(TileMap, AbsoluteTileX, AbsoluteTileY, AbsoluteTileZ);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY, ChunkPosition.TileChunkZ);
    uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPosition.RelativeTileX, ChunkPosition.RelativeTileY);

    return (TileChunkValue);
}

internal uint32
GetTileValue(tile_map *TileMap, tile_map_position Position)
{
    bool32 TileChunkValue = GetTileValue(TileMap, Position.AbsoluteTileX, Position.AbsoluteTileY, Position.AbsoluteTileZ);

    return (TileChunkValue);
}

internal bool32
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position Position)
{
    uint32 TileChunkValue = GetTileValue(TileMap, Position);
    bool32 Empty = ((TileChunkValue == 1) || (TileChunkValue == 3) || (TileChunkValue == 4));

    return (Empty);
}

internal void
SetTileValue(memory_arena *Arena, tile_map *TileMap,
             uint32 AbsoluteTileX, uint32 AbsoluteTileY, uint32 AbsoluteTileZ,
             uint32 TileValue)
{
    tile_chunk_position ChunkPosition = GetChunkPositionFor(TileMap, AbsoluteTileX, AbsoluteTileY, AbsoluteTileZ);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY, ChunkPosition.TileChunkZ);

    Assert(TileChunk);
    if (!TileChunk->Tiles)
    {
        uint32 TileCount = TileMap->ChunkDim * TileMap->ChunkDim;
        TileChunk->Tiles = PushArray(Arena, TileCount, uint32);
        for (uint32 TileIndex = 0; TileIndex < TileCount; ++TileIndex)
        {
            TileChunk->Tiles[TileIndex] = 1;
        }
    }

    SetTileValue(TileMap, TileChunk, ChunkPosition.RelativeTileX, ChunkPosition.RelativeTileY, TileValue);
}

//
//
//

inline void
RecanonicalizeCoord(tile_map *TileMap, uint32 *Tile, real32 *TileRelative)
{
    int32 Offset = RoundReal32ToInt32(*TileRelative / TileMap->TileSideInMeters);
    *Tile += Offset;
    *TileRelative -= Offset * TileMap->TileSideInMeters;

    Assert(*TileRelative >= -0.5f * TileMap->TileSideInMeters);
    Assert(*TileRelative <= 0.5f * TileMap->TileSideInMeters);
}

inline tile_map_position
RecanonicalizePosition(tile_map *TileMap, tile_map_position Position)
{
    tile_map_position Result = Position;

    RecanonicalizeCoord(TileMap, &Result.AbsoluteTileX, &Result.OffsetX);
    RecanonicalizeCoord(TileMap, &Result.AbsoluteTileY, &Result.OffsetY);

    return (Result);
}

inline bool32
AreOnSameTile(tile_map_position *A, tile_map_position *B)
{
    bool32 Result = ((A->AbsoluteTileX == B->AbsoluteTileX) &&
                     (A->AbsoluteTileY == B->AbsoluteTileY) &&
                     (A->AbsoluteTileZ == B->AbsoluteTileZ));

    return (Result);
}

inline tile_map_difference
Subtract(tile_map *TileMap, tile_map_position *A, tile_map_position *B)
{
    tile_map_difference Result;

    real32 dTileX = (real32)A->AbsoluteTileX - (real32)B->AbsoluteTileX;
    real32 dTileY = (real32)A->AbsoluteTileY - (real32)B->AbsoluteTileY;
    real32 dTileZ = (real32)A->AbsoluteTileZ - (real32)B->AbsoluteTileZ;

    Result.dX = TileMap->TileSideInMeters * dTileX + (A->OffsetX - B->OffsetX);
    Result.dY = TileMap->TileSideInMeters * dTileY + (A->OffsetY - B->OffsetY); 
    Result.dZ = TileMap->TileSideInMeters * dTileZ;

    return (Result);
}