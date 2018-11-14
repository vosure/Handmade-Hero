#include <windows.h>
#include <stdint.h>

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

global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;

global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;


internal void 
RenderWierdGradent(int XOffset, int YOffset)
{
	int Width = BitmapWidth;
	int Height = BitmapHeight;

	int Pitch = Width * BytesPerPixel;
	uint8 *Row = (uint8 *)BitmapMemory;
	for (int Y = 0; Y < BitmapHeight; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < BitmapWidth; ++X)
		{
			uint8 Blue = X + XOffset;
			uint8 Red = Y + YOffset;

			*Pixel++ = (Red << 16 | Blue);
		}
		Row += Pitch;
	}
}

internal void
Win32ResizeDIBSection(int Width, int Height)
{
	if (BitmapMemory)
	{
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader) ;
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapInfo.bmiHeader.biHeight = Height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	
	BitmapWidth = Width;
	BitmapHeight = Height;

	int BitmapMemorySize = (Width*Height)*BytesPerPixel;
	BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect, int X, int Y, int Width, int Height)
{
	int WindowWidth = ClientRect->right - ClientRect->left;
	int WindowHeight = ClientRect->bottom - ClientRect->top;

	StretchDIBits(DeviceContext,
				  0, 0, BitmapWidth, BitmapHeight,
				  0, 0, WindowWidth, WindowHeight,
				  BitmapMemory,
				  &BitmapInfo,
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
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			Win32ResizeDIBSection(Width, Height);
		}break;

		case WM_CLOSE:
		{
			Running = false;
		}break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVEAPP\n");
		}break;

		case WM_DESTROY:
		{
			Running = false;
		}break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;

			RECT ClientRect;
			GetClientRect(Window, &ClientRect);

			HDC DeviceContext = BeginPaint(Window, &Paint);
			
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

			Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
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

	WNDCLASS WindowClass = {};
	
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

			Running = true;
			while(Running)
			{
				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
						Running = false;

					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				RenderWierdGradent(XOffset, YOffset);
				HDC DeviceContext = GetDC(Window);
				RECT ClientRect;
				GetClientRect(Window, &ClientRect);
				int WindowWidth = ClientRect.right - ClientRect.left;
				int WindowHeight = ClientRect.bottom - ClientRect.top;
				Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
				ReleaseDC(Window, DeviceContext);
				++XOffset;
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

