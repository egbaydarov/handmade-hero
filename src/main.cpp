#include <libloaderapi.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#include <winerror.h>
#include <xinput.h>

#define GLOBAL_VARIABLE static
#define LOCAL_PERSIST   static
#define INTERNAL        static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

struct win32_offscreen_buffer
{
    BITMAPINFO bitmapInfo;
    void *bitmapMemory;
    int bitmapWidth;
    int bitmapHeight;
    int bytesPerPixel;
};

struct win32_window_dimension
{
    int width;
    int height;
};

GLOBAL_VARIABLE int xOffset = 0;
GLOBAL_VARIABLE int yOffset = 0;
GLOBAL_VARIABLE bool g_running;
GLOBAL_VARIABLE win32_offscreen_buffer g_backBuffer;

typedef DWORD WINAPI
x_input_set_state(DWORD, XINPUT_VIBRATION *);
typedef DWORD WINAPI
x_input_get_state(DWORD, XINPUT_STATE *);

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return 0;
}
GLOBAL_VARIABLE x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return 0;
}
GLOBAL_VARIABLE x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

INTERNAL void
Win32LoadXinput(void)
{
    HMODULE xinputLib = LoadLibraryA("Xinput9_1_0.dll");
    if (!xinputLib)
    {
        printf("[DEBUG] fallback into 1.4 xinput\n");
        xinputLib = LoadLibraryA("Xinput1_4.dll");
    }
    if (!xinputLib)
    {
        printf("[DEBUG] fallback into 1.3 xinput\n");
        xinputLib = LoadLibraryA("Xinput1_3.dll");
    }

    if (xinputLib)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(xinputLib, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(xinputLib, "XInputSetState");
        printf("[DEBUG] xinput dll loaded\n");
    }
}

INTERNAL win32_window_dimension
Win32GetWindowDimension(
    HWND window)
{
    win32_window_dimension result;
    RECT rect;
    BOOL ok = GetClientRect(window, &rect);

    // TODO(byda): any case to fail?
    (void)ok;

    result.width = rect.right - rect.left;
    result.height = rect.bottom - rect.top;

    return (result);
}

INTERNAL void
RenderShit(
    win32_offscreen_buffer *buffer,
    int xOffset,
    int yOffset)
{
    int pitch = buffer->bitmapWidth * buffer->bytesPerPixel;
    u8 *row = (u8 *)buffer->bitmapMemory;

    for (int y = 0; y < buffer->bitmapHeight; ++y)
    {
        u32 *pixel = (u32 *)row;

        // for (int x = 255 - (u8)xOffset; x < g_bitmapWidth; ++x)
        for (int x = 0; x < buffer->bitmapWidth; ++x)
        {
            // u8 r = (u8)(x * y + xOffset + yOffset);
            u8 r = 0;
            u8 b = (u8)x + xOffset;
            u8 g = (u8)y + yOffset;

            // win format (bbggrraa) : le format (aarrggbb)
            *pixel++ = (255 << 24) | (r << 16) | (g << 8) | b;

            //*pixel++ = 0xFF0000FF;
        }

        row += pitch;
    }
}

INTERNAL void
Win32ResizeDIBSection(
    win32_offscreen_buffer *buffer,
    int width,
    int height)
{
    if (buffer->bitmapMemory)
    {
        VirtualFree(buffer->bitmapMemory, 0, MEM_RELEASE);
    }

    buffer->bitmapWidth = width;
    buffer->bitmapHeight = height;
    buffer->bytesPerPixel = 4;

    buffer->bitmapInfo.bmiHeader.biSize = sizeof(buffer->bitmapInfo.bmiHeader);
    buffer->bitmapInfo.bmiHeader.biWidth = buffer->bitmapWidth;
    buffer->bitmapInfo.bmiHeader.biHeight = buffer->bitmapHeight;
    buffer->bitmapInfo.bmiHeader.biPlanes = 1;
    buffer->bitmapInfo.bmiHeader.biBitCount = 32;
    buffer->bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = buffer->bytesPerPixel * buffer->bitmapWidth *
                           buffer->bitmapHeight;
    buffer->bitmapMemory =
        VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    printf("%dx%d p=0x%p\n", width, height, buffer->bitmapMemory);
}

INTERNAL void
Win32UpdateWindow(
    win32_offscreen_buffer *buffer,
    HDC hdc,
    int windowWidth,
    int windowHeight)
{
    StretchDIBits(
        hdc,
        0,
        0,
        windowWidth,
        windowHeight,
        0,
        0,
        buffer->bitmapWidth,
        buffer->bitmapHeight,
        buffer->bitmapMemory,
        &buffer->bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
    case WM_SIZE:
    {
    }
    break;

    case WM_ACTIVATEAPP:
    {
        printf("WM_ACTIVATEAPP\n");
    }
    break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYUP:
    case WM_KEYDOWN:
    {
        u32 vkcode = wParam;
        bool wasDown = ((lParam & (1 << 30)) != 0);
        bool isDown = ((lParam & (1 << 31)) == 0);

        if (vkcode == 'W')
        {
            printf("w");
        }
        else if (vkcode == 'S')
        {
            printf("w");
        }
        else if (vkcode == 'A')
        {
            printf("w");
        }
        else if (vkcode == 'D')
        {
            printf("w");
        }
        else if (vkcode == 'Q')
        {
            printf("w");
        }
        else if (vkcode == 'E')
        {
            printf("w");
        }
        else if (vkcode == VK_DOWN)
        {
            printf("w");
        }
        else if (vkcode == VK_UP)
        {
            printf("w");
        }
        else if (vkcode == VK_LEFT)
        {
            printf("w");
        }
        else if (vkcode == VK_RIGHT)
        {
            printf("w");
        }
        else if (vkcode == VK_SPACE)
        {
            printf("w");
        }
        else if (vkcode == VK_ESCAPE)
        {
            printf("w");
        }
    }
    break;
    case WM_DESTROY:
    {
        g_running = false;
    }
    break;

    case WM_CLOSE:
    {
        g_running = false;
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC hdc = BeginPaint(window, &paint);

        // TODO(byda): workaround

        win32_window_dimension windowDim = Win32GetWindowDimension(window);
        Win32UpdateWindow(
            &g_backBuffer,
            hdc,
            windowDim.width,
            windowDim.height);

        BOOL endPaintOk = EndPaint(window, &paint);
        (void)endPaintOk;
        ++xOffset;
        ++xOffset;
        --yOffset;
    }
    break;

    default:
    {
        result = DefWindowProc(window, message, wParam, lParam);
    }
    break;
    }

    return result;
}

int WINAPI
WinMain(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR cmdLine,
    int showCmd)
{
    (void)prevInstance;
    (void)cmdLine;
    (void)showCmd;

    Win32LoadXinput();
    Win32ResizeDIBSection(&g_backBuffer, 2560, 1280);

    WNDCLASSA windowClass = {0};
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClassA(&windowClass))
    {
        HWND window = CreateWindowExA(
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

        if (window)
        {
            // NOTE(byda): depends on window creation flag (should I create a
            // hdc each iteration or not)
            HDC hdc = GetDC(window);
            g_running = true;

            size_t j = 0;
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

                for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
                {
                    XINPUT_STATE state;
                    DWORD xinputRes = XInputGetState(i, &state);

                    if (xinputRes == ERROR_SUCCESS)
                    {
                        XINPUT_GAMEPAD *gamepad = &state.Gamepad;

                        bool dpadUp = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        bool dpadDown = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        bool dpadLeft = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        bool dpadRight = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        bool start = gamepad->wButtons & XINPUT_GAMEPAD_START;
                        bool back = gamepad->wButtons & XINPUT_GAMEPAD_BACK;
                        bool a = gamepad->wButtons & XINPUT_GAMEPAD_A;
                        bool b = gamepad->wButtons & XINPUT_GAMEPAD_B;
                        bool x = gamepad->wButtons & XINPUT_GAMEPAD_X;
                        bool y = gamepad->wButtons & XINPUT_GAMEPAD_Y;
                        bool lShoulder = gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        bool rShoulder = gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        bool lThumb = gamepad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
                        bool rThumb = gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
                        SHORT stickLX = gamepad->sThumbLX;
                        SHORT stickLY = gamepad->sThumbLY;
                        SHORT stickRX = gamepad->sThumbRX;
                        SHORT stickRY = gamepad->sThumbRY;

                        if (b)
                        {
                            xOffset += 10;
                        }
                        if (a)
                        {
                            yOffset -= 10;
                        }
                        if (y)
                        {
                            yOffset += 10;
                        }
                        if (x)
                        {
                            xOffset -= 10;
                        }
                    }
                    else
                    {
                    }
                }

                XINPUT_VIBRATION vibr;
                vibr.wLeftMotorSpeed = 60000;
                vibr.wRightMotorSpeed = 60000;
                // XInputSetState(0, &vibr);

                RenderShit(&g_backBuffer, xOffset, yOffset);

                win32_window_dimension windowDim = Win32GetWindowDimension(
                    window);
                Win32UpdateWindow(
                    &g_backBuffer,
                    hdc,
                    windowDim.width,
                    windowDim.height);

                ReleaseDC(window, hdc);
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
