#include <stdio.h>
#include <stdint.h>
#include <windows.h>

#define global_variable static
#define local_persist static
#define internal static

global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4; 

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

internal void
RenderGradient(int xOffset, int yOffset)
{
  int pitch = BitmapWidth * BytesPerPixel;
  uint8 *row = (uint8*)BitmapMemory;
  for (int y = 0; y < BitmapHeight; ++y)
  {
    uint32 *pixel = (uint32 *)row;
    for (int x = 255 - (uint8)xOffset; x < BitmapWidth; ++x)
    {
      //blue
      uint8 b = x;
      uint8 g = y;
      uint8 r = x * y + xOffset + yOffset;
      *pixel++ = (b << 24 | g << 16 | r << 8);
    }

    row += pitch;
  }
}

internal void
Win32ResizeDIBSection(
    int width,
    int height)
{
  if (BitmapMemory)
  {
    VirtualFree(BitmapMemory, 0, MEM_RELEASE);
  }

  BitmapWidth = width;
  BitmapHeight = height;
  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = BitmapWidth;
  BitmapInfo.bmiHeader.biHeight = BitmapHeight;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  int bitmapMemorySize = BytesPerPixel * BitmapWidth * BitmapHeight;
  BitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}


internal void
Win32UpdateWindow(
    HDC hdc,
    RECT *clientRect,
    int x,
    int y,
    int height,
    int width)
{
  int windowWidth = clientRect->right - clientRect->left;
  int windowHeight = clientRect->bottom - clientRect->top;
  StretchDIBits(
    hdc,
    0, 0, BitmapWidth, BitmapHeight,
    0, 0, windowWidth, windowHeight,
    BitmapMemory,
    &BitmapInfo,
    DIB_RGB_COLORS,
    SRCCOPY);
}

LRESULT Win32MainWindowCallback(
  HWND Wnd,
  UINT Msg,
  WPARAM Wparam,
  LPARAM Lparam)
{
  LRESULT Result = 0;
  switch (Msg) {
    case WM_SIZE:
    {
      RECT rect;
      BOOL ok = GetClientRect(Wnd, &rect);
      LONG height = rect.bottom - rect.top;
      LONG width = rect.right - rect.left;
      Win32ResizeDIBSection(width, height);
    } break;
    case WM_ACTIVATEAPP:
    {
      printf("WM_ActivateApp");
    } break;
    case WM_DESTROY:
    {
      Running = false;
    } break;
    case WM_CLOSE:
    {
      Running = false;
    } break;
    case WM_PAINT:
    {
      PAINTSTRUCT paint;
      HDC deviceContext = BeginPaint(Wnd, &paint);
      LONG x = paint.rcPaint.left;
      LONG y = paint.rcPaint.top;
      LONG height = paint.rcPaint.bottom - paint.rcPaint.top;
      LONG width = paint.rcPaint.right - paint.rcPaint.left;
      RECT rect;
      BOOL ok = GetClientRect(Wnd, &rect);
      Win32UpdateWindow(deviceContext, &rect, x, y, width, height);
      BOOL EndPaintResult = EndPaint(Wnd, &paint);
    } break;
    default:
    {
      Result = DefWindowProc(Wnd, Msg, Wparam, Lparam);
      //printf("");
    } break;
  }

  return (Result);
}

int WINAPI WinMain(
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR CmdLine,
  int ShowCmd)
{
  WNDCLASS windowClass = {0};
  windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW; // UINT
  windowClass.lpfnWndProc = Win32MainWindowCallback; // WNDPROC
  //windowClass.cbClsExtra = 0; // int
  //windowClass.cbWndExtra = 0; // int
  windowClass.hInstance = Instance; // HINSTANCE
  //windowClass.hIcon; // HICON
  //windowClass.hCursor; // HCURSOR
  //windowClass.hbrBackground; // HBRUSH
  //windowClass.lpszMenuName; // LPCWSTR
  windowClass.lpszClassName = "HandmadeHeroWindowClass"; // LPCWSTR

  if (RegisterClass(&windowClass))
  {
    HWND windowHandle = CreateWindowEx(
      0,                                     //DWORD
      windowClass.lpszClassName,             //LPCWSTR
      "Handmade Hero",                       //LPCWSTR   lpWindowName
      WS_OVERLAPPEDWINDOW|WS_VISIBLE,        //DWORD     dwStyle
      CW_USEDEFAULT,                         //int       X,
      CW_USEDEFAULT,                         //int       Y,
      CW_USEDEFAULT,                         //int       nWidth,
      CW_USEDEFAULT,                         //int       nHeight,
      0,                                     //HWND      hWndParent,
      0,                                     //HMENU     hMenu,
      Instance,                             //HINSTANCE hInstance,
      0                                     //LPVOID    lpParam
    );

    if (windowHandle)
    {
      MSG Message;
      Running = true;
      int xOffset = 0;
      int yOffset = 0;
      while(Running)
      {
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
          if (message.message == WM_QUIT)
          {
            Running = false;
          }
          TranslateMessage(&message);
          DispatchMessage(&message);
        }
        RECT rect;
        BOOL ok = GetClientRect(windowHandle, &rect);
        HDC deviceContext = GetDC(windowHandle);
        RenderGradient(xOffset, yOffset);
        Win32UpdateWindow(deviceContext, &rect, 0, 0, 0, 0);
        ReleaseDC(windowHandle, deviceContext);
        ++xOffset;
        ++xOffset;
        --yOffset;
      }
    }
    else
    {
      printf("Failed to create a window.");
    }
  }
  else
  {
      printf("Failed to register window class");
  }

  return 0;
}
