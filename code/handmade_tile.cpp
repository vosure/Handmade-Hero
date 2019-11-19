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

    RecanonicalizeCoord(TileMap, &Result.AbsoluteTileX, &Result.TileRelativeX);
    RecanonicalizeCoord(TileMap, &Result.AbsoluteTileY, &Result.TileRelativeY);

    return (Result);
}

inline tile_chunk *
GetTileChunk(tile_map *TileMap, int32 TileChunkX, int32 TileChunkY)
{
    tile_chunk *TileChunk = 0;

    if ((TileChunkX >= 0) && (TileChunkX < TileMap->TileChunkCountX) &&
        (TileChunkY >= 0) && (TileChunkY < TileMap->TileChunkCountY))
    {
        TileChunk = &TileMap->TileChunks[TileChunkY * TileMap->TileChunkCountX + TileChunkX];
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

internal bool32
GetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY)
{
    uint32 TileChunkValue = 0;

    if (TileChunk)
    {
        TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
    }

    return (TileChunkValue);
}

inline tile_chunk_position
GetChunkPositionFor(tile_map *TileMap, uint32 AbsoluteTileX, uint32 AbsoluteTileY)
{
    tile_chunk_position Result;

    Result.TileChunkX = AbsoluteTileX >> TileMap->ChunkShift;
    Result.TileChunkY = AbsoluteTileY >> TileMap->ChunkShift;
    Result.RelativeTileX = AbsoluteTileX & TileMap->ChunkMask;
    Result.RelativeTileY = AbsoluteTileY & TileMap->ChunkMask;

    return (Result);
}

internal uint32
GetTileValue(tile_map *TileMap, uint32 AbsoluteTileX, uint32 AbsoluteTileY)
{
    bool32 Empty = false;

    tile_chunk_position ChunkPosition = GetChunkPositionFor(TileMap, AbsoluteTileX, AbsoluteTileY);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY);
    uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPosition.RelativeTileX, ChunkPosition.RelativeTileY);

    return (TileChunkValue);
}

internal bool32
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position CanonicalPosition)
{
    uint32 TileChunkValue = GetTileValue(TileMap, CanonicalPosition.AbsoluteTileX, CanonicalPosition.AbsoluteTileY);
    bool32 Empty = (TileChunkValue == 0);

    return (Empty);
}