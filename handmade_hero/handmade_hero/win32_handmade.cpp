#include <windows.h>
#include <stdint.h>
#include <xinput.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint32_t uint64;

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;

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
	return(0);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// Set state
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex,  XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return(0);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
Win32LoadXInput()
{
	HMODULE XInputLibrary =  LoadLibraryA("xinput1_4.dll");

	if (XInputLibrary)
	{ 
	XInputGetState = (x_input_get_state *) GetProcAddress(XInputLibrary, "XInputGetState");
	XInputSetState = (x_input_set_state *) GetProcAddress(XInputLibrary, "XInputSetState");
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
RenderWierdGradent(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
	uint8 *Row = (uint8 *)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer->Width; ++X)
		{
			uint8 Blue = X + XOffset;
			uint8 Red = Y + YOffset;

			*Pixel++ = (Red << 16 | Blue);
		}
		Row += Buffer->Pitch;
	}
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader) ;
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

			bool KeyWasDown = ((LParam & (1 << 30)) != 0);
			bool KeyIsDown = ((LParam & (1 << 31)) == 0);
			
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
	Win32LoadXInput();

	WNDCLASSA WindowClass = {};

	//win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
	
	WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
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
							WS_OVERLAPPEDWINDOW|WS_VISIBLE,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT, 
							0, 0, Instance, 0);
		if (Window)
		{
			int XOffset = 0;
			int YOffset = 0;

			GlobalRunning = true;
			while(GlobalRunning)
			{
				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
						GlobalRunning = false;

					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
				{
					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						// Controller is Plugged in
						XINPUT_GAMEPAD *Gamepad = &ControllerState.Gamepad;

						bool UP = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool DOWN = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool LEFT = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool RIGHT = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool START = (Gamepad->wButtons & XINPUT_GAMEPAD_START);
						bool BACK = (Gamepad->wButtons & XINPUT_GAMEPAD_BACK);
						bool LEFT_THUMB = (Gamepad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
						bool RIGHT_THUMB = (Gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
						bool LEFT_SHOULDER = (Gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool A_BUTTON = (Gamepad->wButtons & XINPUT_GAMEPAD_A);
						bool B_BUTTON = (Gamepad->wButtons & XINPUT_GAMEPAD_B);
						bool X_BUTTON = (Gamepad->wButtons & XINPUT_GAMEPAD_X);
						bool Y_BUTTON = (Gamepad->wButtons & XINPUT_GAMEPAD_Y);

						uint16 LEFT_STICK_X = Gamepad->sThumbLX;
						uint16 LEFT_STICK_Y = Gamepad->sThumbLX;
						uint16 RIGHT_STICK_X = Gamepad->sThumbRX;
						uint16 RIGHT_STICK_Y = Gamepad->sThumbRX;

					}
					else
					{
						// Contoller is unavailable
					}
				}

				RenderWierdGradent(&GlobalBackBuffer, XOffset, YOffset);
				HDC DeviceContext = GetDC(Window);
				
				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
				ReleaseDC(Window, DeviceContext);
				++XOffset;
				--YOffset;
			}
		}
		else
		{
			//LATER
		}

	}
	else
	{
		// LATER
	}

	return (0);
}

