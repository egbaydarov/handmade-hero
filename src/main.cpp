#include <stdio.h>
#include <windows.h>

LRESULT MainWindowCallback(
  HWND Wnd,
  UINT Msg,
  WPARAM Wparam,
  LPARAM Lparam)
{
  LRESULT Result = 0;
  switch (Msg) {
    case WM_SIZE:
    {
      printf("WM_SIZE");
    } break;
    case WM_ACTIVATEAPP:
    {
      printf("WM_ActivateApp");
    } break;
    case WM_DESTROY:
    {
      PostQuitMessage(0);
    } break;
    /*
    case WM_CLOSE:
    {
      printf("WM_CLOSE");
    } break;
    */
    case WM_PAINT:
    {
      PAINTSTRUCT paint;
      HDC deviceContext = BeginPaint(Wnd, &paint);
      LONG x = paint.rcPaint.left;
      LONG y = paint.rcPaint.top;
      LONG height = paint.rcPaint.bottom - paint.rcPaint.top;
      LONG width = paint.rcPaint.right - paint.rcPaint.left;
      static DWORD operation = WHITENESS;
      PatBlt(deviceContext, x, y, width, height, operation);
      if (operation == WHITENESS)
      {
        operation = BLACKNESS;
      }
      else
      {
        operation = WHITENESS;
      }
      BOOL EndPaintResult = EndPaint(Wnd, &paint);
    } break;
    default:
    {
      Result = DefWindowProc(Wnd, Msg, Wparam, Lparam);
      //printf("WM_SIZE");
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
  windowClass.lpfnWndProc = MainWindowCallback; // WNDPROC
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
      for(;;)
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
