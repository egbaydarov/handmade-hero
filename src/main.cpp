#include <stdio.h>
#include <windows.h>

#define global_variable static
#define local_persist static
#define internal static

global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

internal void
Win32ResizeDIBSection(
    int width,
    int height)
{
  if (BitmapHandle)
  {
    DeleteObject(BitmapHandle);
  }

  if (!BitmapDeviceContext)
  {
    BitmapDeviceContext = CreateCompatibleDC(0);
  }

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = width;
  BitmapInfo.bmiHeader.biHeight = height;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  HBITMAP bitmapHandle = CreateDIBSection(
    BitmapDeviceContext,
    &BitmapInfo,
    DIB_RGB_COLORS,
    &BitmapMemory,
    0,
    0
  );
}

internal void
Win32UpdateWindow(
    HDC hdc,
    int x,
    int y,
    int width,
    int height)
{
  StretchDIBits(
    hdc,
    x, y, width, height,
    x, y, width, height,
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
      Win32UpdateWindow(deviceContext, x, y, width, height);
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
      while(Running)
      {
        BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
        if (MessageResult > 0)
        {
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }
        else
        {
          break;
        }
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
