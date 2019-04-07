#include <windows.h>
#include <malloc.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>
#include <stdio.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint32_t uint64;

typedef float real32;
typedef double real64;

#include "win32_handmade.h"

global_variable bool32 GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerformanceCountFrequency;

#include "handmade.cpp"

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
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// Set state
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex,  XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal debug_read_file_result
DEBUGPlatformReadEntireFile(char *FileName)
{
	debug_read_file_result Result = {};
	HANDLE FileHandle = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
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
					DEBUGPlatformFreeFileMemory(Result.Contents);
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
	return(Result);
}

internal void
DEBUGPlatformFreeFileMemory(void *Memory)
{
	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

internal bool32
DEBUGPlatformWriteEntireFile(char *FileName, uint32 MemorySize, void *Memory)
{
	bool32 Result = false;
	HANDLE FileHandle = CreateFile(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
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
	return(Result);
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
		direct_sound_create *DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
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

	return(Dimension);
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
	Buffer->Info.bmiHeader.biHeight = Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	Buffer->Width = Width;
	Buffer->Height = Height;

	int BitmapMemorySize = (Width*Height)*Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Width * Buffer->BytesPerPixel;
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext,
	int WindowWidth, int WindowHeight)
{
	StretchDIBits(DeviceContext,
		0, 0, WindowWidth, WindowHeight,
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
		uint8 *DestSample = (uint8*)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}
		DestSample = (uint8*)Region2;
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
		int16 *DestSample = (int16*)Region1;
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
	Assert(NewState->EndedDown != IsDown);
	NewState->EndedDown = IsDown;
	++NewState->HalfTransitionCount;
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
	return(Result);
}

internal void
Win32ProcessPendingMessages(game_controller_input *KeyboardController)
{
	MSG Message;
	while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch (Message.message)
		{
		case WM_QUIT:
		{
			GlobalRunning = false;
		} break;

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
			}

			bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
			if ((VKCode == VK_F4) && AltKeyWasDown)
			{
				GlobalRunning = false;
			}
		} break;

		default:
		{
			TranslateMessage(&Message);
			DispatchMessageA(&Message);
		} break;
		}
	}
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND   Window,
	UINT   Message,
	WPARAM WParam,
	LPARAM LParam
)
{
	LRESULT Result = 0;

	switch (Message)
	{
	case WM_SIZE:
	{
	}break;

	case WM_CLOSE:
	{
		GlobalRunning = false;
	}break;

	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVEAPP\n");
	}break;

	case WM_DESTROY:
	{
		GlobalRunning = false;
	}break;

	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	{
		Assert(!"Keyboard input came in through a non-dispatch message!");
	}break;

	case WM_PAINT:
	{
		PAINTSTRUCT Paint;

		HDC DeviceContext = BeginPaint(Window, &Paint);

		win32_window_dimension Dimension = Win32GetWindowDimension(Window);
		Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

		EndPaint(Window, &Paint);
	}break;

	default:
	{
		//	OutputDebugStringA("default\n");
		Result = DefWindowProc(Window, Message, WParam, LParam);
	}break;
	}

	return(Result);
}

inline LARGE_INTEGER
Win32GetWallClock()
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return(Result);
}

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	return (real32)((End.QuadPart - Start.QuadPart) / (real32)GlobalPerformanceCountFrequency);
}

internal void
Win32DebugDrawVertical(win32_offscreen_buffer *GlobalBackBuffer,
	int32 X, int32 Top, int32 Bottom, uint32 Color)
{
	uint8 *Pixel = ((uint8 *)GlobalBackBuffer->Memory + X * GlobalBackBuffer->BytesPerPixel + Top * GlobalBackBuffer->Pitch);
	for (int32 Y = Top; Y < Bottom; ++Y)
	{
		*(uint32 *)Pixel = Color;
		Pixel += GlobalBackBuffer->Pitch;
	}
}

inline void
Win32DrawSoundBufferMarker(win32_offscreen_buffer *BackBuffer,
	win32_sound_output *SoundOutput, real32 C, int32 PadX, int32 Top, int32 Bottom, DWORD Value, uint32 Color)
{
	Assert(Value < SoundOutput->SecondaryBufferSize);
	real32 XReal32 = (C * (real32)Value);
	int32 X = PadX + (int32)XReal32;
	Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);
}

internal void
Win32DebugSyncDisplay(win32_offscreen_buffer *BackBuffer,
	int32 MarkerCount, win32_debug_time_marker *Markers,
	win32_sound_output *SoundOutput, real32 TargetSecondsElapsed)
{
	int32 PadX = 16;
	int32 PadY = 16;

	int32 Top = PadY;
	int32 Bottom = BackBuffer->Height - PadY;

	real32 C = (real32)(BackBuffer->Width - 2 * PadX) / (real32)SoundOutput->SecondaryBufferSize;

	for (int32 MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex)
	{
		win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
		Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->PlayCursor, 0xFFFFFFFF);
		Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->WriteCursor, 0xFFFF0000);
	}
}

int CALLBACK
WinMain(HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR     CommandLine,
	int       ShowCode)
{
	LARGE_INTEGER PerformanceCountFrequencyResult;
	QueryPerformanceFrequency(&PerformanceCountFrequencyResult);
	GlobalPerformanceCountFrequency = PerformanceCountFrequencyResult.QuadPart;

	UINT DesiredSchedulerMs = 1;
	bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMs) == TIMERR_NOERROR);

	Win32LoadXInput();

	WNDCLASSA WindowClass = {};

	//win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	//	WindowClass.hIcon = ;
	//	WindowClass.lpszMenuName = ;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

#define FramesOfAudionLatency 3
#define MonitorRefreshHz  60
#define GameUpdateHz (MonitorRefreshHz / 2)

	real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

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

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = FramesOfAudionLatency * (SoundOutput.SamplesPerSecond / GameUpdateHz);
			SoundOutput.SafetyBytes = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample / GameUpdateHz) / 3;
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

			uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

			if (Sample && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				LARGE_INTEGER LastCounter = Win32GetWallClock();

				int32 DebugTimeMarkerIndex = 0;
				win32_debug_time_marker DebugTimeMarkers[GameUpdateHz / 2] = { 0 };

				DWORD AudioLatencyBytes = 0;
				real32 AudioLatencySeconds = 0;
				bool32 SoundIsValid = false;

				uint64 LastCycleCount = (uint64)__rdtsc();
				while (GlobalRunning)
				{
					game_controller_input *NewKeyboardController = GetController(NewInput, 0);
					game_controller_input *OldKeyboardController = GetController(OldInput, 0);
					*NewKeyboardController = {};
					NewKeyboardController->IsConnected = true;

					for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex)
					{
						NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
					}

					Win32ProcessPendingMessages(NewKeyboardController);

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
					DWORD BytesToWrite = 0;
					DWORD TargetCursor = 0;

					game_offscreen_buffer Buffer;
					Buffer.Memory = GlobalBackBuffer.Memory;
					Buffer.Width = GlobalBackBuffer.Width;
					Buffer.Height = GlobalBackBuffer.Height;
					Buffer.Pitch = GlobalBackBuffer.Pitch;
					GameUpdateAndRender(&GameMemory, &Buffer, NewInput);

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

						DWORD ExpectedSoundBytesPerFrame = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz;
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
						GameGetSoundSamples(&GameMemory, &SoundBuffer);
#if HANDMADE_INTERNAL
						DWORD PlayCursor;
						DWORD WriteCursor;
						GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

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
#if HANDMADE_INTERNAL
					Win32DebugSyncDisplay(&GlobalBackBuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkers,
						&SoundOutput, TargetSecondsPerFrame);
#endif
					Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

#if HANDMADE_INTERNAL
					{
						DWORD PlayCursor;
						DWORD WriteCursor;
						if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
						{
							Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
							win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex++];
							if (DebugTimeMarkerIndex >= ArrayCount(DebugTimeMarkers))
							{
								DebugTimeMarkerIndex = 0;
							}
							Marker->PlayCursor = PlayCursor;
							Marker->WriteCursor = WriteCursor;
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

	return(0);
}

