#include "handmade.h"
#include "handmade_tile.cpp"
#include "handmade_random.h"

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
DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, real32 RealX, real32 RealY)
{
    int32 MinX = RoundReal32ToInt32(RealX);
    int32 MinY = RoundReal32ToInt32(RealY);
    int32 MaxX = RoundReal32ToInt32(RealX + (real32)Bitmap->Width);
    int32 MaxY = RoundReal32ToInt32(RealY + (real32)Bitmap->Height);

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

    uint32 *SourceRow = Bitmap->Pixels + Bitmap->Width * (Bitmap->Height - 1);
    uint8 *DestRow = ((uint8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);

    for (int32 Y = MinY; Y < MaxY; ++Y)
    {
        uint32 *Dest = (uint32 *)DestRow;
        uint32 *Source = SourceRow;
        for (int32 X = MinX; X < MaxX; ++X)
        {   
            real32 A = (real32)((*Source >> 24) & 0xFF) / 255.0f;

            real32 SR = (real32)((*Source >> 16) & 0xFF);
            real32 SG = (real32)((*Source >> 8) & 0xFF);
            real32 SB = (real32)((*Source >> 0) & 0xFF);

            real32 DR = (real32)((*Dest >> 16) & 0xFF);
            real32 DG = (real32)((*Dest >> 8) & 0xFF);
            real32 DB = (real32)((*Dest >> 0) & 0xFF);

            real32 R = (1.0f - A) * DR + A * SR;
            real32 G = (1.0f - A) * DG + A * SG;
            real32 B = (1.0f - A) * DB + A * SB;

            *Dest = ((uint32)(R + 0.5f) << 16) |
                    ((uint32)(G + 0.5f) << 8) | 
                    ((uint32)(B + 0.5f) << 0);  

            ++Dest;
            ++Source;
        }
        DestRow += Buffer->Pitch;
        SourceRow -= Bitmap->Width;
    }
}

#pragma pack(push, 1)
struct bitmap_header
{
    uint16 FileType;
    uint32 FileSize;
    uint16 Reserved1;
    uint16 Reserved2;
    uint32 BitmapOffset;
    uint32 Size;
    int32 Width;
    int32 Height;
    uint16 Planes;
    uint16 BitsPerPixel;
    uint32 Compression;
    uint32 SizeOfBitap;
    int32 HorzResolution;
    int32 VertResolution; 
    uint32 ColorsUsed;
    uint32 ColorsImportant;

    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
};
#pragma pack(pop)

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntirefile, char *FileName)
{
    loaded_bitmap Result = {};
    debug_read_file_result ReadResult = ReadEntirefile(Thread, FileName);
    if (ReadResult.ContentSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
        Result.Pixels = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;

        uint32 RedMask = Header->RedMask;
        uint32 GreenMask = Header->GreenMask;
        uint32 BlueMask = Header->BlueMask;
        uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

        bit_scan_result RedShift = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenShift = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueShift = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaShift = FindLeastSignificantSetBit(AlphaMask);

        Assert(RedShift.Found);
        Assert(GreenShift.Found);
        Assert(BlueShift.Found);
        Assert(AlphaShift.Found);

        uint32 *SourceDest = Pixels;
        for (int32 Y = 0; Y < Header->Height; ++Y)
        {
            for (int32 X = 0; X < Header->Width; ++X)
            {
                uint32 C = *SourceDest;
                *SourceDest++ = (((C >> AlphaShift.Index) & 0xFF) << 24) |
                                (((C >> RedShift.Index) & 0xFF) << 16) |
                                (((C >> GreenShift.Index) & 0xFF) << 8) |
                                (((C >> BlueShift.Index) & 0xFF) << 0);
            }
        }
    }

    return (Result);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    real32 PlayerHeight = 1.4f;
    real32 PlayerWidth = 0.5f * PlayerHeight;

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        GameState->Backdrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");
        GameState->HeroHead = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_head.bmp");
        GameState->HeroCape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_cape.bmp");
        GameState->HeroTorso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_torso.bmp");

        GameState->PlayerPosition.AbsoluteTileX = 1;
        GameState->PlayerPosition.AbsoluteTileY = 3;
        GameState->PlayerPosition.OffsetX = 5.0f;
        GameState->PlayerPosition.OffsetY = 5.0f;

        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), (uint8 *)Memory->PermanentStorage + sizeof(game_state));

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        World->TileMap = PushStruct(&GameState->WorldArena, tile_map);

        tile_map *TileMap = World->TileMap;

        TileMap->ChunkShift = 4;
        TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
        TileMap->ChunkDim = (1 << TileMap->ChunkShift);

        TileMap->TileChunkCountX = 128;
        TileMap->TileChunkCountY = 128;
        TileMap->TileChunkCountZ = 2;
        TileMap->TileChunks = PushArray(&GameState->WorldArena,
                                        TileMap->TileChunkCountX *
                                            TileMap->TileChunkCountY *
                                            TileMap->TileChunkCountZ,
                                        tile_chunk);

        TileMap->TileSideInMeters = 1.4f;

        uint32 RandomNumberIndex = 0;
        uint32 TilesPerWidth = 17;
        uint32 TilesPerHeight = 9;
        uint32 ScreenX = 0;
        uint32 ScreenY = 0;
        uint32 AbsoluteTileZ = 0;

        bool32 DoorLeft = false;
        bool32 DoorRight = false;
        bool32 DoorTop = false;
        bool32 DoorBottom = false;

        bool32 DoorUp = false;
        bool32 DoorDown = false;
        for (uint32 ScreenIndex = 0; ScreenIndex < 100; ++ScreenIndex)
        {
            Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));

            uint32 RandomChoice;
            if (DoorUp || DoorDown)
            {
                RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
            }
            else
            {
                RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
            }

            bool32 CreatedZDoor = false;
            if (RandomChoice == 2)
            {
                CreatedZDoor = true;
                if (AbsoluteTileZ == 0)
                {
                    DoorUp = true;
                }
                else
                {
                    DoorDown = true;
                }
            }
            else if (RandomChoice == 1)
            {
                DoorRight = true;
            }
            else
            {
                DoorTop = true;
            }

            for (uint32 TileY = 0; TileY < TilesPerHeight; TileY++)
            {
                for (uint32 TileX = 0; TileX < TilesPerWidth; TileX++)
                {
                    uint32 AbsoluteTileX = ScreenX * TilesPerWidth + TileX;
                    uint32 AbsoluteTileY = ScreenY * TilesPerHeight + TileY;

                    uint32 TileValue = 1;
                    if ((TileX == 0) && (!DoorLeft || (TileY != TilesPerHeight / 2)))
                    {
                        TileValue = 2;
                    }
                    if ((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != TilesPerHeight / 2)))
                    {
                        TileValue = 2;
                    }

                    if ((TileY == 0) && (!DoorBottom || (TileX != TilesPerWidth / 2)))
                    {
                        TileValue = 2;
                    }
                    if ((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != TilesPerWidth / 2)))
                    {
                        TileValue = 2;
                    }

                    if ((TileX == 10) && (TileY == 5))
                    {
                        if (DoorUp)
                        {
                            TileValue = 3;
                        }
                        if (DoorDown)
                        {
                            TileValue = 4;
                        }
                    }

                    SetTileValue(&GameState->WorldArena, World->TileMap, AbsoluteTileX, AbsoluteTileY, AbsoluteTileZ, TileValue);
                }
            }

            DoorLeft = DoorRight;
            DoorBottom = DoorTop;

            if (CreatedZDoor)
            {
                DoorDown = !DoorDown;
                DoorUp = !DoorUp;
            }
            else
            {
                DoorUp = false;
                DoorDown = false;
            }

            DoorRight = false;
            DoorTop = false;

            if (RandomChoice == 2)
            {
                if (AbsoluteTileZ == 0)
                {
                    AbsoluteTileZ = 1;
                }
                else
                {
                    AbsoluteTileZ = 0;
                }
            }
            else if (RandomChoice == 1)
            {
                ScreenX++;
            }
            else
            {
                ScreenY++;
            }
        }

        Memory->IsInitialized = true;
    }

    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;

    uint32 TileSideInPixels = 60;
    real32 MetersToPixels = (real32)(TileSideInPixels) / (real32)(TileMap->TileSideInMeters);

    real32 LowerLeftX = -(real32)TileSideInPixels / 2;
    real32 LowerLeftY = (real32)Buffer->Height;

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
                PlayerSpeed = 15.0f;
            }

            dPlayerX *= PlayerSpeed;
            dPlayerY *= PlayerSpeed;

            tile_map_position NewPlayerPosition = GameState->PlayerPosition;
            NewPlayerPosition.OffsetX += Input->dtForFrame * dPlayerX;
            NewPlayerPosition.OffsetY += Input->dtForFrame * dPlayerY;
            NewPlayerPosition = RecanonicalizePosition(TileMap, NewPlayerPosition);

            tile_map_position PlayerLeft = NewPlayerPosition;
            PlayerLeft.OffsetX -= 0.5f * PlayerWidth;
            PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);

            tile_map_position PlayerRight = NewPlayerPosition;
            PlayerRight.OffsetX += 0.5f * PlayerWidth;
            PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);

            if (IsTileMapPointEmpty(TileMap, NewPlayerPosition) &&
                IsTileMapPointEmpty(TileMap, PlayerLeft) &&
                IsTileMapPointEmpty(TileMap, PlayerRight))
            {
                if (!AreOnSameTile(&GameState->PlayerPosition, &NewPlayerPosition))
                {
                    uint32 NewTileValue = GetTileValue(TileMap, NewPlayerPosition);

                    if (NewTileValue == 3)
                    {
                        ++NewPlayerPosition.AbsoluteTileZ;
                    }
                    else if (NewTileValue == 4)
                    {
                        --NewPlayerPosition.AbsoluteTileZ;
                    }
                }

                GameState->PlayerPosition = NewPlayerPosition;
            }
        }
    }

    DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);

    real32 ScreenCenterX = (real32)Buffer->Width * 0.5f;
    real32 ScreenCenterY = (real32)Buffer->Height * 0.5f;

    for (int32 RelativeRow = -10; RelativeRow < 10; RelativeRow++)
    {
        for (int32 RelativeColumn = -20; RelativeColumn < 20; RelativeColumn++)
        {
            uint32 Column = RelativeColumn + GameState->PlayerPosition.AbsoluteTileX;
            uint32 Row = RelativeRow + GameState->PlayerPosition.AbsoluteTileY;

            uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->PlayerPosition.AbsoluteTileZ);
            if (TileID > 1)
            {
                real32 Gray = 0.5f;
                if (TileID == 2)
                {
                    Gray = 1.0f;
                }
                if (TileID > 2)
                {
                    Gray = 0.30f;
                }
                if ((Column == GameState->PlayerPosition.AbsoluteTileX) &&
                    (Row == GameState->PlayerPosition.AbsoluteTileY))
                {
                    Gray = 0.0f;
                }

                real32 CenterX = ScreenCenterX - MetersToPixels * GameState->PlayerPosition.OffsetX + ((real32)RelativeColumn) * TileSideInPixels;
                real32 CenterY = ScreenCenterY + MetersToPixels * GameState->PlayerPosition.OffsetY - ((real32)RelativeRow) * TileSideInPixels;
                real32 MinX = CenterX - 0.5f * TileSideInPixels;
                real32 MinY = CenterY - 0.5f * TileSideInPixels;
                real32 MaxX = CenterX + 0.5f * TileSideInPixels;
                real32 MaxY = CenterY + 0.5f * TileSideInPixels;
                DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
            }
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;

    real32 PlayerLeft = ScreenCenterX - 0.5f * PlayerWidth * MetersToPixels;
    real32 PlayerTop = ScreenCenterY - PlayerHeight * MetersToPixels;
    DrawRectangle(Buffer, PlayerLeft, PlayerTop,
                  PlayerLeft + PlayerWidth * MetersToPixels,
                  PlayerTop + PlayerHeight * MetersToPixels,
                  PlayerR, PlayerG, PlayerB);
    DrawBitmap(Buffer, &GameState->HeroHead, 0, 0);

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
