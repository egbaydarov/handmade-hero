#include <stdio.h>
#include <stdint.h>
#include <windows.h>

#define GLOBAL_VARIABLE static
#define LOCAL_PERSIST   static
#define INTERNAL        static

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

GLOBAL_VARIABLE bool       g_running;
GLOBAL_VARIABLE BITMAPINFO g_bitmapInfo;
GLOBAL_VARIABLE void      *g_bitmapMemory;
GLOBAL_VARIABLE HBITMAP    g_bitmapHandle;
GLOBAL_VARIABLE int        g_bitmapWidth;
GLOBAL_VARIABLE int        g_bitmapHeight;
GLOBAL_VARIABLE int        g_bytesPerPixel = 4;

INTERNAL void
RenderGradient(int xOffset, int yOffset)
{
    int pitch = g_bitmapWidth * g_bytesPerPixel;
    u8 *row = (u8 *)g_bitmapMemory;

    for (int y = 0; y < g_bitmapHeight; ++y)
    {
        u32 *pixel = (u32 *)row;

        for (int x = 255 - (u8)xOffset; x < g_bitmapWidth; ++x)
        {
            // blue
            u8 b = (u8)x;
            u8 g = (u8)y;
            u8 r = (u8)(x * y + xOffset + yOffset);

            *pixel++ = (b << 24) | (g << 16) | (r << 8);
        }

        row += pitch;
    }
}

INTERNAL void
Win32ResizeDIBSection(int width, int height)
{
    if (g_bitmapMemory)
    {
        VirtualFree(g_bitmapMemory, 0, MEM_RELEASE);
    }

    g_bitmapWidth  = width;
    g_bitmapHeight = height;

    g_bitmapInfo.bmiHeader.biSize        = sizeof(g_bitmapInfo.bmiHeader);
    g_bitmapInfo.bmiHeader.biWidth       = g_bitmapWidth;
    g_bitmapInfo.bmiHeader.biHeight      = g_bitmapHeight;
    g_bitmapInfo.bmiHeader.biPlanes      = 1;
    g_bitmapInfo.bmiHeader.biBitCount    = 32;
    g_bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = g_bytesPerPixel * g_bitmapWidth * g_bitmapHeight;
    g_bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

INTERNAL void
Win32UpdateWindow(
    HDC hdc,
    RECT *clientRect,
    int x,
    int y,
    int width,
    int height)
{
    int windowWidth  = clientRect->right - clientRect->left;
    int windowHeight = clientRect->bottom - clientRect->top;

    StretchDIBits(
        hdc,
        0, 0, g_bitmapWidth, g_bitmapHeight,
        0, 0, windowWidth, windowHeight,
        g_bitmapMemory,
        &g_bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_SIZE:
        {
            RECT rect;
            BOOL ok = GetClientRect(window, &rect);
            (void)ok;

            LONG width  = rect.right - rect.left;
            LONG height = rect.bottom - rect.top;

            Win32ResizeDIBSection((int)width, (int)height);
        } break;

        case WM_ACTIVATEAPP:
        {
            printf("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            g_running = false;
        } break;

        case WM_CLOSE:
        {
            g_running = false;
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC hdc = BeginPaint(window, &paint);

            LONG x      = paint.rcPaint.left;
            LONG y      = paint.rcPaint.top;
            LONG width  = paint.rcPaint.right - paint.rcPaint.left;
            LONG height = paint.rcPaint.bottom - paint.rcPaint.top;

            RECT rect;
            BOOL ok = GetClientRect(window, &rect);
            (void)ok;

            Win32UpdateWindow(hdc, &rect, (int)x, (int)y, (int)width, (int)height);

            BOOL endPaintOk = EndPaint(window, &paint);
            (void)endPaintOk;
        } break;

        default:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        } break;
    }

    return result;
}

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    (void)prevInstance;
    (void)cmdLine;
    (void)showCmd;

    WNDCLASSA windowClass = {0};
    windowClass.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc   = Win32MainWindowCallback;
    windowClass.hInstance     = instance;
    windowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClassA(&windowClass))
    {
        HWND windowHandle = CreateWindowExA(
            0,
            windowClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            instance,
            0);

        if (windowHandle)
        {
            g_running = true;
            int xOffset = 0;
            int yOffset = 0;

            while (g_running)
            {
                MSG msg;
                while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                    {
                        g_running = false;
                    }

                    TranslateMessage(&msg);
                    DispatchMessageA(&msg);
                }

                RECT rect;
                BOOL ok = GetClientRect(windowHandle, &rect);
                (void)ok;

                HDC hdc = GetDC(windowHandle);

                RenderGradient(xOffset, yOffset);
                Win32UpdateWindow(hdc, &rect, 0, 0, 0, 0);

                ReleaseDC(windowHandle, hdc);

                ++xOffset;
                ++xOffset;
                --yOffset;
            }
        }
        else
        {
            printf("Failed to create a window.\n");
        }
    }
    else
    {
        printf("Failed to register window class.\n");
    }

    return 0;
}

