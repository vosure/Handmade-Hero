#include "handmade.h"

#include <windows.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

#include "win32_handmade.h"

global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerformanceCountFrequency;

/*
typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState);
global_variable x_input_get_state *XInputGetState_;
#define XInputGetState XInputGetState_;
*/

// Get state
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// Set state
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void
CatStrings(size_t SourceACount, char *SourceA,
		   size_t SourceBCount, char *SourceB,
		   size_t DestCount, char *Dest)
{
	// TODO(casey): Dest bounds checking!

	for (int Index = 0;
		 Index < SourceACount;
		 ++Index)
	{
		*Dest++ = *SourceA++;
	}

	for (int Index = 0;
		 Index < SourceBCount;
		 ++Index)
	{
		*Dest++ = *SourceB++;
	}

	*Dest++ = 0;
}

internal void
Win32GetEXEFileName(win32_state *State)
{
	// NOTE(casey): Never use MAX_PATH in code that is user-facing, because it
	// can be dangerous and lead to bad results.
	DWORD SizeOfFilename = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
	State->OnePastLastEXEFileNameSlash = State->EXEFileName;
	for (char *Scan = State->EXEFileName;
		 *Scan;
		 ++Scan)
	{
		if (*Scan == '\\')
		{
			State->OnePastLastEXEFileNameSlash = Scan + 1;
		}
	}
}

internal int
StringLength(char *String)
{
	int Count = 0;
	while (*String++)
	{
		++Count;
	}
	return (Count);
}

internal void
Win32BuildEXEPathFileName(win32_state *State, char *FileName,
						  int DestCount, char *Dest)
{
	CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName, State->EXEFileName,
			   StringLength(FileName), FileName,
			   DestCount, Dest);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
	debug_read_file_result Result = {};
	HANDLE FileHandle = CreateFile(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{

			uint32 FileSize32 = SafeTruncateUInt64((uint64)FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead))
				{
					// File read successfully
					Result.ContentSize = FileSize32;
				}
				else
				{
					// Logging
					DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{
				// Logging
			}
		}
		else
		{
			// Logging
		}
		CloseHandle(FileHandle);
	}
	else
	{
		// Logging
	}
	return (Result);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
	bool32 Result = false;
	HANDLE FileHandle = CreateFile(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
		{
			// Successfull write
			Result = (BytesWritten == MemorySize);
		}
		else
		{
			// Logging
		}
		CloseHandle(FileHandle);
	}
	else
	{
		// Logging
	}
	return (Result);
}

inline FILETIME
Win32GetLastWriteTime(char *FileName)
{
	FILETIME LastWriteTime = {};

	WIN32_FILE_ATTRIBUTE_DATA Data;
	if (GetFileAttributesEx(FileName, GetFileExInfoStandard, &Data))
	{
		LastWriteTime = Data.ftLastWriteTime;
	}

	return (LastWriteTime);
}

internal win32_game_code
Win32LoadGameCode(char *SourceDLLName, char *TempDLLName)
{
	win32_game_code Result = {};

	Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

	CopyFile(SourceDLLName, TempDLLName, FALSE);
	Result.GameCodeDLL = LoadLibraryA(TempDLLName);
	if (Result.GameCodeDLL)
	{
		Result.UpdateAndRender = (game_update_and_render *)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
		Result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");
		Result.IsValid = Result.UpdateAndRender && Result.GetSoundSamples;
	}

	if (!Result.IsValid)
	{
		Result.UpdateAndRender = 0;
		Result.GetSoundSamples = 0;
	}
	return (Result);
}

internal void
Win32UnloadGameCode(win32_game_code *GameCode)
{
	if (GameCode->GameCodeDLL)
	{
		FreeLibrary(GameCode->GameCodeDLL);
	}
	GameCode->IsValid = false;
	GameCode->UpdateAndRender = 0;
	GameCode->GetSoundSamples = 0;
}

internal void
Win32LoadXInput()
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");

	if (!XInputLibrary)
		LoadLibraryA("xinput1_3.dll");

	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

internal void
Win32InitDirectSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	HMODULE DSoundLibrary = LoadLibrary("dsound.dll");

	if (DSoundLibrary)
	{
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.wBitsPerSample * WaveFormat.nChannels) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nBlockAlign * WaveFormat.nSamplesPerSec;
			WaveFormat.cbSize = 0;
			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						OutputDebugStringA("\nPrimary buffer format was set\n");
					}
					else
					{
					}
				}
				else
				{
				}
			}

			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
			{
				OutputDebugStringA("\nSecondary buffer created succesfully\n");
			}
		}
		else
		{
			// Diagnostic
		}
	}
	else
	{
	}
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Dimension;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Dimension.Width = ClientRect.right - ClientRect.left;
	Dimension.Height = ClientRect.bottom - ClientRect.top;

	return (Dimension);
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Width;
	Buffer->Info.bmiHeader.biHeight = -Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	Buffer->Width = Width;
	Buffer->Height = Height;

	int BitmapMemorySize = (Width * Height) * Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Width * Buffer->BytesPerPixel;
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext,
						   int WindowWidth, int WindowHeight)
{
	int32 OffsetY = 10;
	int32 OffsetX = 10;

	PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
	PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
	PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
	PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);

	StretchDIBits(DeviceContext,
				  OffsetX, OffsetY, Buffer->Width, Buffer->Height,
				  0, 0, Buffer->Width, Buffer->Height,
				  Buffer->Memory,
				  &Buffer->Info,
				  DIB_RGB_COLORS, SRCCOPY);
}

internal void
Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		uint8 *DestSample = (uint8 *)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}
		DestSample = (uint8 *)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD BytesToLock, DWORD BytesToWrite, game_sound_output_buffer *SourceBuffer)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(BytesToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		int16 *DestSample = (int16 *)Region1;
		int16 *SourseSample = SourceBuffer->Samples;
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;

		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourseSample++;
			*DestSample++ = *SourseSample++;
			++SoundOutput->RunningSampleIndex;
		}

		DestSample = (int16 *)Region2;
		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourseSample++;
			*DestSample++ = *SourseSample++;
			++SoundOutput->RunningSampleIndex;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, game_button_state *NewState, DWORD ButtonBit)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
	if (NewState->EndedDown != IsDown)
	{
		NewState->EndedDown = IsDown;
		++NewState->HalfTransitionCount;
	}
}

internal real32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshhold)
{
	real32 Result = 0;
	if (Value < -DeadZoneThreshhold)
	{
		Result = (real32)Value / 32768.0f;
	}
	else if (Value > DeadZoneThreshhold)
	{
		Result = (real32)Value / 32767.0f;
	}
	return (Result);
}

internal void
Win32GetInputFileLocation(win32_state *State, bool32 InputStream, int32 SlotIndex, int32 DestCount, char *Dest)
{
	char Temp[64];
	wsprintf(Temp, "loop_edit_%d_%s.hmi", SlotIndex, InputStream ? "input" : "state");
	Win32BuildEXEPathFileName(State, Temp, DestCount, Dest);
}

internal win32_replay_buffer *
Win32GetReplayBuffer(win32_state *State, int unsigned Index)
{
	Assert(Index < ArrayCount(State->ReplayBuffers));
	win32_replay_buffer *Result = &State->ReplayBuffers[Index];
	return (Result);
}

internal void
Win32BeginRecordingInput(win32_state *State, int InputRecordingIndex)
{
	win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputRecordingIndex);
	if (ReplayBuffer->MemoryBlock)
	{
		State->InputRecordingIndex = InputRecordingIndex;

		char FileName[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, true, InputRecordingIndex, sizeof(FileName), FileName);
		State->RecordingHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

#if 0
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx(State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
#endif
		CopyMemory(ReplayBuffer->MemoryBlock, State->GameMemoryBlock, State->TotalSize);
	}
}

internal void
Win32EndRecordingInput(win32_state *State)
{
	CloseHandle(State->RecordingHandle);
	State->InputRecordingIndex = 0;
}

internal void
Win32BeginPlayBackInput(win32_state *State, int InputPlayingIndex)
{
	win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputPlayingIndex);
	if (ReplayBuffer->MemoryBlock)
	{
		State->InputPlayingIndex = InputPlayingIndex;

		char FileName[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, true, InputPlayingIndex, sizeof(FileName), FileName);
		State->PlayBackHandle = CreateFileA(FileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);

#if 0
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx(State->PlayBackHandle, FilePosition, 0, FILE_BEGIN);
#endif
		CopyMemory(State->GameMemoryBlock, ReplayBuffer->MemoryBlock, State->TotalSize);
	}
}

internal void
Win32EndPlayBackInput(win32_state *State)
{
	CloseHandle(State->PlayBackHandle);
	State->InputPlayingIndex = 0;
}

internal void
Win32RecordInput(win32_state *State, game_input *NewInput)
{
	DWORD BytesWritten;
	WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}

internal void
Win32PlayBackInput(win32_state *State, game_input *NewInput)
{
	DWORD BytesRead = 0;
	if (ReadFile(State->PlayBackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0))
	{
		if (BytesRead == 0)
		{
			int32 PlayingIndex = State->InputPlayingIndex;
			Win32EndPlayBackInput(State);
			Win32BeginPlayBackInput(State, PlayingIndex);
			ReadFile(State->PlayBackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
		}
	}
}

internal void
Win32ProcessPendingMessages(win32_state *State, game_controller_input *KeyboardController)
{
	MSG Message;
	while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch (Message.message)
		{
		case WM_QUIT:
		{
			GlobalRunning = false;
		}
		break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32 VKCode = (uint32)Message.wParam;
			bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
			bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
			if (WasDown != IsDown)
			{
				if (VKCode == 'W')
				{
					Win32ProcessKeyboardMessage(&KeyboardController->MOVE_UP, IsDown);
				}
				else if (VKCode == 'A')
				{
					Win32ProcessKeyboardMessage(&KeyboardController->MOVE_LEFT, IsDown);
				}
				else if (VKCode == 'S')
				{
					Win32ProcessKeyboardMessage(&KeyboardController->MOVE_DOWN, IsDown);
				}
				else if (VKCode == 'D')
				{
					Win32ProcessKeyboardMessage(&KeyboardController->MOVE_RIGHT, IsDown);
				}
				else if (VKCode == 'Q')
				{
					Win32ProcessKeyboardMessage(&KeyboardController->LEFT_SHOULDER, IsDown);
				}
				else if (VKCode == 'E')
				{
					Win32ProcessKeyboardMessage(&KeyboardController->RIGHT_SHOULDER, IsDown);
				}
				else if (VKCode == VK_UP)
				{
					Win32ProcessKeyboardMessage(&KeyboardController->ACTION_UP, IsDown);
				}
				else if (VKCode == VK_LEFT)
				{
					Win32ProcessKeyboardMessage(&KeyboardController->ACTION_LEFT, IsDown);
				}
				else if (VKCode == VK_DOWN)
				{
					Win32ProcessKeyboardMessage(&KeyboardController->ACTION_DOWN, IsDown);
				}
				else if (VKCode == VK_RIGHT)
				{
					Win32ProcessKeyboardMessage(&KeyboardController->ACTION_RIGHT, IsDown);
				}
				else if (VKCode == VK_ESCAPE)
				{
					Win32ProcessKeyboardMessage(&KeyboardController->START, IsDown);
				}
				else if (VKCode == VK_SPACE)
				{
					Win32ProcessKeyboardMessage(&KeyboardController->BACK, IsDown);
				}

#if HANDMADE_INTERNAL
				else if (VKCode == 'P')
				{
					if (IsDown)
					{
						GlobalPause = !GlobalPause;
					}
				}
				else if (VKCode == 'L')
				{
					if (IsDown)
					{
						if (State->InputPlayingIndex == 0)
						{
							if (State->InputRecordingIndex == 0)
							{
								Win32BeginRecordingInput(State, 1);
							}
							else
							{
								Win32EndRecordingInput(State);
								Win32BeginPlayBackInput(State, 1);
							}
						}
						else
						{
							Win32EndPlayBackInput(State);
						}
					}
				}
#endif
			}

			bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
			if ((VKCode == VK_F4) && AltKeyWasDown)
			{
				GlobalRunning = false;
			}
		}
		break;

		default:
		{
			TranslateMessage(&Message);
			DispatchMessageA(&Message);
		}
		break;
		}
	}
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
						UINT Message,
						WPARAM WParam,
						LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message)
	{
	case WM_SIZE:
	{
	}
	break;

	case WM_CLOSE:
	{
		GlobalRunning = false;
	}
	break;

	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVEAPP\n");
	}
	break;

	case WM_DESTROY:
	{
		GlobalRunning = false;
	}
	break;

	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	{
		Assert(!"Keyboard input came in through a non-dispatch message!");
	}
	break;

	case WM_PAINT:
	{
		PAINTSTRUCT Paint;

		HDC DeviceContext = BeginPaint(Window, &Paint);

		win32_window_dimension Dimension = Win32GetWindowDimension(Window);
		Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

		EndPaint(Window, &Paint);
	}
	break;

	default:
	{
		//	OutputDebugStringA("default\n");
		Result = DefWindowProc(Window, Message, WParam, LParam);
	}
	break;
	}

	return (Result);
}

inline LARGE_INTEGER
Win32GetWallClock()
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return (Result);
}

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	return (real32)((End.QuadPart - Start.QuadPart) / (real32)GlobalPerformanceCountFrequency);
}

internal void
Win32DebugDrawVertical(win32_offscreen_buffer *BackBuffer,
					   int32 X, int32 Top, int32 Bottom, uint32 Color)
{
	if (Top <= 0)
	{
		Top = 0;
	}

	if (Bottom >= BackBuffer->Height)
	{
		Bottom = BackBuffer->Height;
	}
	if (X >= 0 && X < BackBuffer->Width)
	{
		uint8 *Pixel = ((uint8 *)BackBuffer->Memory + X * BackBuffer->BytesPerPixel + Top * BackBuffer->Pitch);
		for (int32 Y = Top; Y < Bottom; ++Y)
		{
			*(uint32 *)Pixel = Color;
			Pixel += BackBuffer->Pitch;
		}
	}
}

inline void
Win32DrawSoundBufferMarker(win32_offscreen_buffer *BackBuffer,
						   win32_sound_output *SoundOutput, real32 C, int32 PadX, int32 Top, int32 Bottom, DWORD Value, uint32 Color)
{
	real32 XReal32 = (C * (real32)Value);
	int32 X = PadX + (int32)XReal32;
	Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);
}

internal void
Win32DebugSyncDisplay(win32_offscreen_buffer *BackBuffer,
					  int32 MarkerCount, win32_debug_time_marker *Markers,
					  int32 CurrentMarkerIndex,
					  win32_sound_output *SoundOutput)
{
	int32 PadX = 16;
	int32 PadY = 16;

	int32 LineHeight = 64;

	real32 C = (real32)(BackBuffer->Width - 2 * PadX) / (real32)SoundOutput->SecondaryBufferSize;

	for (int32 MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex)
	{
		win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];

		Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);

		DWORD PlayColor = 0xFFFFFFFF;
		DWORD WriteColor = 0xFFFF0000;
		DWORD ExpectedFlipColor = 0xFFFFFF00;

		int32 Top = PadY;
		int32 Bottom = PadY + LineHeight;
		if (MarkerIndex == CurrentMarkerIndex)
		{
			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;

			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor, PlayColor);
			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);
			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);

			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;

			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation, PlayColor);
			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);

			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;
		}

		Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor, PlayColor);
		Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
	}
}

int CALLBACK
WinMain(HINSTANCE Instance,
		HINSTANCE PrevInstance,
		LPSTR CommandLine,
		int ShowCode)
{
	win32_state Win32State = {};

	LARGE_INTEGER PerformanceCountFrequencyResult;
	QueryPerformanceFrequency(&PerformanceCountFrequencyResult);
	GlobalPerformanceCountFrequency = PerformanceCountFrequencyResult.QuadPart;

	Win32GetEXEFileName(&Win32State);

	char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, "handmade.dll",
							  sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

	char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, "handmade_temp.dll",
							  sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);

	UINT DesiredSchedulerMs = 1;
	bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMs) == TIMERR_NOERROR);

	Win32LoadXInput();

	WNDCLASSA WindowClass = {};

	//win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	Win32ResizeDIBSection(&GlobalBackBuffer, 960, 540);

	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	//	WindowClass.hIcon = ;
	//	WindowClass.lpszMenuName = ;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&WindowClass))
	{
		HWND Window = CreateWindowExA(
			0,
			WindowClass.lpszClassName,
			"Handmade Hero",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0, 0, Instance, 0);
		if (Window)
		{
			HDC DeviceContext = GetDC(Window);

			win32_sound_output SoundOutput = {};

			int32 MonitorRefreshHz = 60;
			HDC RefreshDC = GetDC(Window);
			int32 Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
			ReleaseDC(Window, RefreshDC);
			if (Win32RefreshRate > 1)
			{
				MonitorRefreshHz = Win32RefreshRate;
			}
			real32 GameUpdateHz(MonitorRefreshHz / 2.0f);
			real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.SafetyBytes = (int32)(((real32)SoundOutput.SamplesPerSecond * (real32)SoundOutput.BytesPerSample / GameUpdateHz) / 3.0f);
			Win32InitDirectSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;

			int16 *Sample = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
			LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
			LPVOID BaseAddress = 0;
#endif

			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes(1);
			GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
			GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
			GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

			Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, Win32State.TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
			GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

			for (int32 ReplayIndex = 0; ReplayIndex < ArrayCount(Win32State.ReplayBuffers); ++ReplayIndex)
			{
				win32_replay_buffer *ReplayBuffer = &Win32State.ReplayBuffers[ReplayIndex];

				Win32GetInputFileLocation(&Win32State, false, ReplayIndex, sizeof(ReplayBuffer->FileName), ReplayBuffer->FileName);

				ReplayBuffer->FileHandle = CreateFileA(ReplayBuffer->FileName, GENERIC_WRITE | GENERIC_READ, 0, 0, CREATE_ALWAYS, 0, 0);

				LARGE_INTEGER MaxSize;
				MaxSize.QuadPart = Win32State.TotalSize;
				ReplayBuffer->MemoryMap = CreateFileMapping(
					ReplayBuffer->FileHandle, 0, PAGE_READWRITE,
					MaxSize.HighPart, MaxSize.LowPart, 0);
				ReplayBuffer->MemoryBlock = MapViewOfFile(ReplayBuffer->MemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, Win32State.TotalSize);

				if (ReplayBuffer->MemoryBlock)
				{
				}
				else
				{
					// TODO(vosure): Logging
				}
			}

			if (Sample && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				LARGE_INTEGER LastCounter = Win32GetWallClock();
				LARGE_INTEGER FlipWallClock = Win32GetWallClock();

				int32 DebugTimeMarkerIndex = 0;
				win32_debug_time_marker DebugTimeMarkers[32] = {0};

				DWORD AudioLatencyBytes = 0;
				real32 AudioLatencySeconds = 0;
				bool32 SoundIsValid = false;

				win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
				uint32 LoadCounter = 0;

				uint64 LastCycleCount = __rdtsc();
				while (GlobalRunning)
				{
					NewInput->dtForFrame = TargetSecondsPerFrame;
					FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
					if (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) == 1 || CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) == -1)
					//if(LoadCounter ++ >120)
					{
						Sleep(155);
						Win32UnloadGameCode(&Game);
						Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
						LoadCounter = 0;
					}

					game_controller_input *NewKeyboardController = GetController(NewInput, 0);
					game_controller_input *OldKeyboardController = GetController(OldInput, 0);
					*NewKeyboardController = {};
					NewKeyboardController->IsConnected = true;

					for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex)
					{
						NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
					}

					Win32ProcessPendingMessages(&Win32State, NewKeyboardController);

					if (!GlobalPause)
					{
						POINT MouseP;
						GetCursorPos(&MouseP);
						ScreenToClient(Window, &MouseP);
						NewInput->MouseX = MouseP.x;
						NewInput->MouseY = MouseP.y;
						NewInput->MouseZ = 0;
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[2], GetKeyState(VK_RBUTTON) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[3], GetKeyState(VK_XBUTTON1) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButtons[4], GetKeyState(VK_XBUTTON2) & (1 << 15));

						DWORD MaxControllerCount = XUSER_MAX_COUNT;
						if (MaxControllerCount > ArrayCount(NewInput->Controllers) - 1)
						{
							MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
						}

						for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
						{
							DWORD OurControllerIndex = ControllerIndex + 1;
							game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
							game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

							XINPUT_STATE ControllerState;
							if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
							{
								NewController->IsConnected = true;
								NewController->IsAnalog = OldController->IsAnalog;
								// Controller is Plugged in
								XINPUT_GAMEPAD *Gamepad = &ControllerState.Gamepad;

								NewController->IsAnalog = true;
								NewController->StickAverageX = Win32ProcessXInputStickValue(Gamepad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
								NewController->StickAverageY = Win32ProcessXInputStickValue(Gamepad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

								if (NewController->StickAverageX != 0.0f || NewController->StickAverageY != 0.0f)
								{
									NewController->IsAnalog = true;
								}

								if (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
								{
									NewController->StickAverageY = 1.0f;
									NewController->IsAnalog = false;
								}
								if (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
								{
									NewController->StickAverageY = -1.0f;
									NewController->IsAnalog = false;
								}
								if (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
								{
									NewController->StickAverageX = -1.0f;
									NewController->IsAnalog = false;
								}
								if (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
								{
									NewController->StickAverageX = 1.0f;
									NewController->IsAnalog = false;
								}
								real32 Threshold = 0.5f;
								Win32ProcessXInputDigitalButton((NewController->StickAverageX < -Threshold) ? 1 : 0, &OldController->MOVE_LEFT, &NewController->MOVE_LEFT, 1);
								Win32ProcessXInputDigitalButton((NewController->StickAverageX > Threshold) ? 1 : 0, &OldController->MOVE_RIGHT, &NewController->MOVE_RIGHT, 1);
								Win32ProcessXInputDigitalButton((NewController->StickAverageY < -Threshold) ? 1 : 0, &OldController->MOVE_DOWN, &NewController->MOVE_DOWN, 1);
								Win32ProcessXInputDigitalButton((NewController->StickAverageY > Threshold) ? 1 : 0, &OldController->MOVE_UP, &NewController->MOVE_UP, 1);

								Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->ACTION_DOWN, &NewController->ACTION_DOWN, XINPUT_GAMEPAD_A);
								Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->ACTION_RIGHT, &NewController->ACTION_RIGHT, XINPUT_GAMEPAD_B);
								Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->ACTION_LEFT, &NewController->ACTION_LEFT, XINPUT_GAMEPAD_X);
								Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->ACTION_UP, &NewController->ACTION_UP, XINPUT_GAMEPAD_Y);

								Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->LEFT_SHOULDER, &NewController->LEFT_SHOULDER, XINPUT_GAMEPAD_LEFT_SHOULDER);
								Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->RIGHT_SHOULDER, &NewController->RIGHT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER);

								Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->START, &NewController->START, XINPUT_GAMEPAD_START);
								Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->BACK, &NewController->BACK, XINPUT_GAMEPAD_BACK);

								//NOTE(vosure) Gamepad Things
								//bool32 LEFT_THUMB = (Gamepad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
								//bool32 RIGHT_THUMB = (Gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);

								//uint16 RIGHT_STICK_X = Gamepad->sThumbRX;
								//uint16 RIGHT_STICK_Y = Gamepad->sThumbRX;
							}
							else
							{
								NewController->IsConnected = true;
							}
						}

						DWORD BytesToLock = 0;

						thread_context Thread = {};

						game_offscreen_buffer Buffer = {};
						Buffer.Memory = GlobalBackBuffer.Memory;
						Buffer.Width = GlobalBackBuffer.Width;
						Buffer.Height = GlobalBackBuffer.Height;
						Buffer.Pitch = GlobalBackBuffer.Pitch;
						Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;

						if (Win32State.InputRecordingIndex)
						{
							Win32RecordInput(&Win32State, NewInput);
						}
						if (Win32State.InputPlayingIndex)
						{
							Win32PlayBackInput(&Win32State, NewInput);
						}

						if (Game.UpdateAndRender)
						{
							Game.UpdateAndRender(&Thread, &GameMemory, &Buffer, NewInput);
						}

						DWORD PlayCursor;
						DWORD WriteCursor;
						if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
						{
							if (!SoundIsValid)
							{
								SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
								SoundIsValid = true;
							}

							BytesToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);

							DWORD ExpectedSoundBytesPerFrame = (int32)(((real32)SoundOutput.SamplesPerSecond * (real32)SoundOutput.BytesPerSample) / GameUpdateHz);
							DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesPerFrame;

							DWORD SafeWriteCursor = WriteCursor;
							if (SafeWriteCursor < PlayCursor)
							{
								SafeWriteCursor += SoundOutput.SecondaryBufferSize;
							}

							Assert(SafeWriteCursor >= PlayCursor);
							SafeWriteCursor += SoundOutput.SafetyBytes;

							bool32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

							DWORD TargetCursor = 0;
							if (AudioCardIsLowLatency)
							{
								TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
							}
							else
							{
								TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
							}

							TargetCursor = (TargetCursor % SoundOutput.SecondaryBufferSize);

							DWORD BytesToWrite = 0;
							if (BytesToLock > TargetCursor)
							{
								BytesToWrite = SoundOutput.SecondaryBufferSize - BytesToLock;
								BytesToWrite += TargetCursor;
							}
							else
							{
								BytesToWrite = TargetCursor - BytesToLock;
							}

							game_sound_output_buffer SoundBuffer = {};
							SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
							SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
							SoundBuffer.Samples = Sample;

							if (Game.GetSoundSamples)
							{
								Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);
							}

#if HANDMADE_INTERNAL
							win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];

							Marker->OutputPlayCursor = PlayCursor;
							Marker->OutputWriteCursor = WriteCursor;
							Marker->OutputLocation = BytesToLock;
							Marker->OutputByteCount = BytesToWrite;
							Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

							DWORD UnwrapedWriteCursor = WriteCursor;
							if (UnwrapedWriteCursor < PlayCursor)
							{
								UnwrapedWriteCursor += SoundOutput.SecondaryBufferSize;
							}
							AudioLatencyBytes = UnwrapedWriteCursor - PlayCursor;
							AudioLatencySeconds = (((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample) / (real32)SoundOutput.SamplesPerSecond);

							char InformationString[256];
							_snprintf_s(InformationString, sizeof(InformationString),
										"BTL:%u TC:%u BTW:%u - PC:%u WC:%u DELTA:%u (%fs)\n",
										BytesToLock, TargetCursor, BytesToWrite,
										PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
							OutputDebugStringA(InformationString);
#endif
							Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
						}
						else
						{
							SoundIsValid = false;
						}

						ReleaseDC(Window, DeviceContext);

						LARGE_INTEGER WorkCounter = Win32GetWallClock();
						real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

						real32 SecondsElapsedforFrame = WorkSecondsElapsed;
						if (SecondsElapsedforFrame < TargetSecondsPerFrame)
						{
							while (SecondsElapsedforFrame < TargetSecondsPerFrame)
							{
								if (SleepIsGranular)
								{
									DWORD SleepMs = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedforFrame));
									if (SleepMs > 0)
									{
										Sleep(SleepMs);
									}
								}
								real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());

								if (TestSecondsElapsedForFrame < TargetSecondsPerFrame)
								{
									//Logging
								}
								while (SecondsElapsedforFrame < TargetSecondsPerFrame)
								{
									SecondsElapsedforFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
								}
							}
						}
						else
						{
						}

						LARGE_INTEGER EndCounter = Win32GetWallClock();
						real32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
						LastCounter = EndCounter;

						win32_window_dimension Dimension = Win32GetWindowDimension(Window);

						Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

						FlipWallClock = Win32GetWallClock();

#if HANDMADE_INTERNAL
						{
							if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
							{
								Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
								win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
								Marker->FlipPlayCursor = PlayCursor;
								Marker->FlipWriteCursor = WriteCursor;
							}
						}
#endif

						game_input *Temp = NewInput;
						NewInput = OldInput;
						OldInput = Temp;

						uint64 EndCycleCount = (uint64)__rdtsc();
						uint64 CyclesElapled = EndCycleCount - LastCycleCount;
						LastCycleCount = EndCycleCount;

						real64 FPS = 0;
						real64 MCPF = (real64)(CyclesElapled / (1000.0f * 1000.0f));

						char InformationString[256];
						_snprintf_s(InformationString, sizeof(InformationString), "%.02fms/f, %.02ff/s, %.02fmc/f\n", MSPerFrame, FPS, MCPF);
						OutputDebugStringA(InformationString);
#if HANDMADE_INTERNAL
						++DebugTimeMarkerIndex;
						if (DebugTimeMarkerIndex >= ArrayCount(DebugTimeMarkers))
						{
							DebugTimeMarkerIndex = 0;
						}
#endif
					}
				}
			}
			else
			{
				// Logging
			}
		}
		else
		{
			// Logging
		}
	}
	else
	{
		// Logging
	}

	return (0);
}
