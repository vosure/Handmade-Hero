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

			uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
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
					DEGUBPlatformFreeFileMemory(Result.Contents);
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
DEGUBPlatformFreeFileMemory(void *Memory)
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
	NewState->EndedDown = (XInputButtonState && ButtonBit) == ButtonBit;
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

LRESULT CALLBACK
MainWindowCallback(HWND   Window,
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
		uint32 KeyCode = WParam;

		bool32 KeyWasDown = ((LParam & (1 << 30)) != 0);
		bool32 KeyIsDown = ((LParam & (1 << 31)) == 0);

		if (KeyIsDown != KeyWasDown)
		{
			if (KeyCode == 'W')
			{
				OutputDebugStringA("W: ");
				if (KeyWasDown)
					OutputDebugStringA("Was Down ");
				if (KeyIsDown)
					OutputDebugStringA("Is Down: ");
				OutputDebugStringA("\n");
			}
			if (KeyCode == 'A')
			{
			}
			if (KeyCode == 'S')
			{
			}
			if (KeyCode == 'D')
			{
			}
			if (KeyCode == 'Q')
			{
			}
			if (KeyCode == 'E')
			{
			}
			if (KeyCode == VK_ESCAPE)
			{
			}
			if (KeyCode == VK_SPACE)
			{
			}
			if (KeyCode == VK_SHIFT)
			{
			}
			if (KeyCode == VK_TAB)
			{
			}
		}

		bool32 AltKeyDown = ((LParam & (1 << 29)) != 0);
		if (KeyCode == VK_F4 && AltKeyDown)
			GlobalRunning = false;

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

int CALLBACK
WinMain(HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR     CommandLine,
	int       ShowCode)
{
	LARGE_INTEGER PerformanceCountFrequencyResult;
	QueryPerformanceFrequency(&PerformanceCountFrequencyResult);
	int64 PerformanceCountFrequency = PerformanceCountFrequencyResult.QuadPart;

	Win32LoadXInput();

	WNDCLASSA WindowClass = {};

	//win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
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

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
			Win32InitDirectSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;

			int16 *Sample = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			game_memory GameMemory = {};

#if HANDMADE_INTERNAL
			LPVOID BaseAddress = Terabytes(1);
#else 
			LPVOID BaseAddress = 0;
#endif

			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes((uint64)6);

			uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			GameMemory.PermamentStrorage = VirtualAlloc(BaseAddress, TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			GameMemory.TransientStrorage = ((uint8 *)GameMemory.PermamentStrorage + GameMemory.PermanentStorageSize);

			if (Sample && GameMemory.PermamentStrorage)
			{
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[0];

				LARGE_INTEGER LastCounter;
				QueryPerformanceCounter(&LastCounter);

				int64 LastCycleCount = __rdtsc();
				while (GlobalRunning)
				{
					MSG Message;

					while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
					{
						if (Message.message == WM_QUIT)
							GlobalRunning = false;

						TranslateMessage(&Message);
						DispatchMessage(&Message);
					}

					int MaxControllerCount = XUSER_MAX_COUNT;
					if (MaxControllerCount > ArrayCount(NewInput->Controllers))
					{
						MaxControllerCount = ArrayCount(NewInput->Controllers);
					}

					for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
					{
						game_controller_input *OldController = &OldInput->Controllers[ControllerIndex];
						game_controller_input *NewController = &NewInput->Controllers[ControllerIndex];

						XINPUT_STATE ControllerState;
						if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
						{
							// Controller is Plugged in
							XINPUT_GAMEPAD *Gamepad = &ControllerState.Gamepad;

							bool32 UP = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
							bool32 DOWN = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
							bool32 LEFT = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
							bool32 RIGHT = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

							NewController->IsAnalog = true;
							NewController->StartX = OldController->EndX;
							NewController->StartY = OldController->EndY;

							real32 X;
							if (Gamepad->sThumbLX < 0)
							{
								X = (real32)Gamepad->sThumbLX / 32768.0f;
							}
							else
							{
								X = (real32)Gamepad->sThumbLX / 32767.0f;
							}
							NewController->MinX = NewController->MaxX = NewController->EndX = X;

							real32 Y;
							if (Gamepad->sThumbLY < 0)
							{
								Y = (real32)Gamepad->sThumbLY / 32768.0f;
							}
							else
							{
								Y = (real32)Gamepad->sThumbLY / 32767.0f;
							}
							NewController->MinY = NewController->MaxY = NewController->EndY = Y;

							Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->DOWN, &NewController->DOWN, XINPUT_GAMEPAD_A);
							Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->RIGHT, &NewController->RIGHT, XINPUT_GAMEPAD_B);
							Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->LEFT, &NewController->LEFT, XINPUT_GAMEPAD_X);
							Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->UP, &NewController->UP, XINPUT_GAMEPAD_Y);
							Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->LEFT_SHOULDER, &NewController->LEFT_SHOULDER, XINPUT_GAMEPAD_LEFT_SHOULDER);
							Win32ProcessXInputDigitalButton(Gamepad->wButtons, &OldController->RIGHT_SHOULDER, &NewController->RIGHT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER);

							//bool32 START = (Gamepad->wButtons & XINPUT_GAMEPAD_START);
							//bool32 BACK = (Gamepad->wButtons & XINPUT_GAMEPAD_BACK);

							//bool32 LEFT_THUMB = (Gamepad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
							//bool32 RIGHT_THUMB = (Gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);

							//uint16 RIGHT_STICK_X = Gamepad->sThumbRX;
							//uint16 RIGHT_STICK_Y = Gamepad->sThumbRX;


						}
						else
						{
							// Contoller is unavailable
						}
					}

					DWORD BytesToLock;
					DWORD BytesToWrite;
					DWORD TargetCursor;
					DWORD PlayCursor;
					DWORD WriteCursor;
					bool32 SoundIsValid = false;
					if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
					{
						SoundIsValid = true;
						BytesToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);
						TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize;
						if (BytesToLock > TargetCursor)
						{
							BytesToWrite = SoundOutput.SecondaryBufferSize - BytesToLock;
							BytesToWrite += TargetCursor;
						}
						else
						{
							BytesToWrite = TargetCursor - BytesToLock;
						}
					}
					game_sound_output_buffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
					SoundBuffer.Samples = Sample;

					game_offscreen_buffer Buffer;
					Buffer.Memory = GlobalBackBuffer.Memory;
					Buffer.Width = GlobalBackBuffer.Width;
					Buffer.Height = GlobalBackBuffer.Height;
					Buffer.Pitch = GlobalBackBuffer.Pitch;
					GameUpdateAndRender(&GameMemory, &Buffer, &SoundBuffer, NewInput);

					if (SoundIsValid)
					{
						Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
					}
					win32_window_dimension Dimension = Win32GetWindowDimension(Window);
					Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
					ReleaseDC(Window, DeviceContext);

					int64 EndCycleCount = __rdtsc();

					LARGE_INTEGER EndCounter;
					QueryPerformanceCounter(&EndCounter);

					int32 CyclesElapled = EndCycleCount - LastCycleCount;
					int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
					int32 MSPerFrame = (int32)(((1000 * CounterElapsed) / PerformanceCountFrequency));
					int32 FPS = PerformanceCountFrequency / CounterElapsed;
					int32 MCPF = (int32)(CyclesElapled / (1000 * 1000));

					char InformationString[256];
					wsprintf(InformationString, "%dms/f, %df/s, %dmc/f\n", MSPerFrame, FPS, MCPF);
					OutputDebugStringA(InformationString);

					LastCounter = EndCounter;
					LastCycleCount = EndCycleCount;

					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = NewInput;

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

