#include <dsound.h>
#include <libloaderapi.h>
#include <math.h>
#include <minwinbase.h>
#include <mmeapi.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#include <winerror.h>
#include <winnt.h>
#include <xinput.h>

#define GLOBAL_VARIABLE static
#define LOCAL_PERSIST   static
#define INTERNAL        static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int16_t s16;
typedef float f32;

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
GLOBAL_VARIABLE char g_pressedLetter = 0;
GLOBAL_VARIABLE IDirectSoundBuffer *g_secondaryBuffer;
GLOBAL_VARIABLE bool g_soundBufferInitialized = false;

typedef DWORD WINAPI
x_input_set_state(DWORD, XINPUT_VIBRATION *);
typedef DWORD WINAPI
x_input_get_state(DWORD, XINPUT_STATE *);

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return (ERROR_DEVICE_NOT_CONNECTED);
}
GLOBAL_VARIABLE x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return (ERROR_DEVICE_NOT_CONNECTED);
}
GLOBAL_VARIABLE x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID lpGUID, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

INTERNAL void
Win32LoadDSound(
    HWND window,
    u32 samplePerSec,
    u32 bufferSize)
{
    HMODULE dSoundLib = LoadLibraryA("dsound.dll");

    if (dSoundLib)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(
            dSoundLib,
            "DirectSoundCreate");

        IDirectSound *directSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
        {
            WAVEFORMATEX waveFormat;
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.nSamplesPerSec = samplePerSec;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
            waveFormat.cbSize = 0;
            printf("[DEBUG] directsound dll loaded\n");
            if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                printf("[DEBUG] directsound set level ok\n");
                DSBUFFERDESC bufferDesc = {};
                bufferDesc.dwSize = sizeof(bufferDesc);
                bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

                IDirectSoundBuffer *primaryBuffer;
                if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, 0)))
                {
                    printf("[DEBUG] directsound primary buffer created\n");
                    if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
                    {
                        printf("[DEBUG] directsound primary buffer format set\n");
                        DSBUFFERDESC secondaryBufferDesc = {};
                        secondaryBufferDesc.dwSize = sizeof(secondaryBufferDesc);
                        secondaryBufferDesc.dwBufferBytes = bufferSize;
                        secondaryBufferDesc.lpwfxFormat = &waveFormat;

                        if (SUCCEEDED(directSound->CreateSoundBuffer(&secondaryBufferDesc, &g_secondaryBuffer, 0)))
                        {
                            printf("[DEBUG] directsound secondary buffer created\n");
                        }
                        else
                        {
                            printf("[WARNING] directsound create sb failed\n");
                        }
                    }
                    else
                    {
                        printf("[WARNING] directsound set pb format failed\n");
                    }
                }
                else
                {
                    printf("[WARNING] directsound create pb failed\n");
                }
            }
            else
            {
                printf("[WARNING] directsound set level failed\n");
            }
        }
        else
        {
            printf("[WARNING] directsound dll load failed\n");
        }
    }
    else
    {
        printf("[WARNING] directsound dll not found\n");
    }
}

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
ShitLetterRender(
    win32_offscreen_buffer *buffer,
    char letter,
    int centerX,
    int centerY,
    int size)
{
    if (letter == 0)
        return;

    // Convert to uppercase for rendering
    char renderLetter = letter;
    if (renderLetter >= 'a' && renderLetter <= 'z')
    {
        renderLetter = renderLetter - 'a' + 'A';
    }

    int halfSize = size / 2;
    int startX = centerX - halfSize;
    int startY = centerY - halfSize;
    int endX = centerX + halfSize;
    int endY = centerY + halfSize;

    int pitch = buffer->bitmapWidth * buffer->bytesPerPixel;
    u8 *baseRow = (u8 *)buffer->bitmapMemory;

    // Simple 8x8 bitmap font pattern for each letter
    // Each letter is represented as 8 rows of 8 bits
    u8 letterPattern[8] = {0};

    // Define simple patterns for letters A-Z
    switch (renderLetter)
    {
    case 'A':
        letterPattern[0] = 0x3C;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x66;
        letterPattern[3] = 0x7E;
        letterPattern[4] = 0x66;
        letterPattern[5] = 0x66;
        letterPattern[6] = 0x66;
        letterPattern[7] = 0x00;
        break;
    case 'B':
        letterPattern[0] = 0x7C;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x66;
        letterPattern[3] = 0x7C;
        letterPattern[4] = 0x66;
        letterPattern[5] = 0x66;
        letterPattern[6] = 0x7C;
        letterPattern[7] = 0x00;
        break;
    case 'C':
        letterPattern[0] = 0x3C;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x60;
        letterPattern[3] = 0x60;
        letterPattern[4] = 0x60;
        letterPattern[5] = 0x66;
        letterPattern[6] = 0x3C;
        letterPattern[7] = 0x00;
        break;
    case 'D':
        letterPattern[0] = 0x78;
        letterPattern[1] = 0x6C;
        letterPattern[2] = 0x66;
        letterPattern[3] = 0x66;
        letterPattern[4] = 0x66;
        letterPattern[5] = 0x6C;
        letterPattern[6] = 0x78;
        letterPattern[7] = 0x00;
        break;
    case 'E':
        letterPattern[0] = 0x7E;
        letterPattern[1] = 0x60;
        letterPattern[2] = 0x60;
        letterPattern[3] = 0x7C;
        letterPattern[4] = 0x60;
        letterPattern[5] = 0x60;
        letterPattern[6] = 0x7E;
        letterPattern[7] = 0x00;
        break;
    case 'F':
        letterPattern[0] = 0x7E;
        letterPattern[1] = 0x60;
        letterPattern[2] = 0x60;
        letterPattern[3] = 0x7C;
        letterPattern[4] = 0x60;
        letterPattern[5] = 0x60;
        letterPattern[6] = 0x60;
        letterPattern[7] = 0x00;
        break;
    case 'G':
        letterPattern[0] = 0x3C;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x60;
        letterPattern[3] = 0x6E;
        letterPattern[4] = 0x66;
        letterPattern[5] = 0x66;
        letterPattern[6] = 0x3C;
        letterPattern[7] = 0x00;
        break;
    case 'H':
        letterPattern[0] = 0x66;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x66;
        letterPattern[3] = 0x7E;
        letterPattern[4] = 0x66;
        letterPattern[5] = 0x66;
        letterPattern[6] = 0x66;
        letterPattern[7] = 0x00;
        break;
    case 'I':
        letterPattern[0] = 0x3C;
        letterPattern[1] = 0x18;
        letterPattern[2] = 0x18;
        letterPattern[3] = 0x18;
        letterPattern[4] = 0x18;
        letterPattern[5] = 0x18;
        letterPattern[6] = 0x3C;
        letterPattern[7] = 0x00;
        break;
    case 'J':
        letterPattern[0] = 0x1E;
        letterPattern[1] = 0x0C;
        letterPattern[2] = 0x0C;
        letterPattern[3] = 0x0C;
        letterPattern[4] = 0x6C;
        letterPattern[5] = 0x6C;
        letterPattern[6] = 0x38;
        letterPattern[7] = 0x00;
        break;
    case 'K':
        letterPattern[0] = 0x66;
        letterPattern[1] = 0x6C;
        letterPattern[2] = 0x78;
        letterPattern[3] = 0x70;
        letterPattern[4] = 0x78;
        letterPattern[5] = 0x6C;
        letterPattern[6] = 0x66;
        letterPattern[7] = 0x00;
        break;
    case 'L':
        letterPattern[0] = 0x60;
        letterPattern[1] = 0x60;
        letterPattern[2] = 0x60;
        letterPattern[3] = 0x60;
        letterPattern[4] = 0x60;
        letterPattern[5] = 0x60;
        letterPattern[6] = 0x7E;
        letterPattern[7] = 0x00;
        break;
    case 'M':
        letterPattern[0] = 0x63;
        letterPattern[1] = 0x77;
        letterPattern[2] = 0x7F;
        letterPattern[3] = 0x6B;
        letterPattern[4] = 0x63;
        letterPattern[5] = 0x63;
        letterPattern[6] = 0x63;
        letterPattern[7] = 0x00;
        break;
    case 'N':
        letterPattern[0] = 0x66;
        letterPattern[1] = 0x76;
        letterPattern[2] = 0x7E;
        letterPattern[3] = 0x7E;
        letterPattern[4] = 0x6E;
        letterPattern[5] = 0x66;
        letterPattern[6] = 0x66;
        letterPattern[7] = 0x00;
        break;
    case 'O':
        letterPattern[0] = 0x3C;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x66;
        letterPattern[3] = 0x66;
        letterPattern[4] = 0x66;
        letterPattern[5] = 0x66;
        letterPattern[6] = 0x3C;
        letterPattern[7] = 0x00;
        break;
    case 'P':
        letterPattern[0] = 0x7C;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x66;
        letterPattern[3] = 0x7C;
        letterPattern[4] = 0x60;
        letterPattern[5] = 0x60;
        letterPattern[6] = 0x60;
        letterPattern[7] = 0x00;
        break;
    case 'Q':
        letterPattern[0] = 0x3C;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x66;
        letterPattern[3] = 0x66;
        letterPattern[4] = 0x66;
        letterPattern[5] = 0x6C;
        letterPattern[6] = 0x3E;
        letterPattern[7] = 0x00;
        break;
    case 'R':
        letterPattern[0] = 0x7C;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x66;
        letterPattern[3] = 0x7C;
        letterPattern[4] = 0x78;
        letterPattern[5] = 0x6C;
        letterPattern[6] = 0x66;
        letterPattern[7] = 0x00;
        break;
    case 'S':
        letterPattern[0] = 0x3C;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x60;
        letterPattern[3] = 0x3C;
        letterPattern[4] = 0x06;
        letterPattern[5] = 0x66;
        letterPattern[6] = 0x3C;
        letterPattern[7] = 0x00;
        break;
    case 'T':
        letterPattern[0] = 0x7E;
        letterPattern[1] = 0x18;
        letterPattern[2] = 0x18;
        letterPattern[3] = 0x18;
        letterPattern[4] = 0x18;
        letterPattern[5] = 0x18;
        letterPattern[6] = 0x18;
        letterPattern[7] = 0x00;
        break;
    case 'U':
        letterPattern[0] = 0x66;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x66;
        letterPattern[3] = 0x66;
        letterPattern[4] = 0x66;
        letterPattern[5] = 0x66;
        letterPattern[6] = 0x3C;
        letterPattern[7] = 0x00;
        break;
    case 'V':
        letterPattern[0] = 0x66;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x66;
        letterPattern[3] = 0x66;
        letterPattern[4] = 0x66;
        letterPattern[5] = 0x3C;
        letterPattern[6] = 0x18;
        letterPattern[7] = 0x00;
        break;
    case 'W':
        letterPattern[0] = 0x63;
        letterPattern[1] = 0x63;
        letterPattern[2] = 0x63;
        letterPattern[3] = 0x6B;
        letterPattern[4] = 0x7F;
        letterPattern[5] = 0x77;
        letterPattern[6] = 0x63;
        letterPattern[7] = 0x00;
        break;
    case 'X':
        letterPattern[0] = 0x66;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x3C;
        letterPattern[3] = 0x18;
        letterPattern[4] = 0x3C;
        letterPattern[5] = 0x66;
        letterPattern[6] = 0x66;
        letterPattern[7] = 0x00;
        break;
    case 'Y':
        letterPattern[0] = 0x66;
        letterPattern[1] = 0x66;
        letterPattern[2] = 0x66;
        letterPattern[3] = 0x3C;
        letterPattern[4] = 0x18;
        letterPattern[5] = 0x18;
        letterPattern[6] = 0x18;
        letterPattern[7] = 0x00;
        break;
    case 'Z':
        letterPattern[0] = 0x7E;
        letterPattern[1] = 0x06;
        letterPattern[2] = 0x0C;
        letterPattern[3] = 0x18;
        letterPattern[4] = 0x30;
        letterPattern[5] = 0x60;
        letterPattern[6] = 0x7E;
        letterPattern[7] = 0x00;
        break;
    default:
        // Draw a box for unknown characters
        for (int i = 0; i < 8; i++)
        {
            letterPattern[i] = (i == 0 || i == 7) ? 0xFF : 0x81;
        }
        break;
    }

    // Render the letter pattern scaled to size
    // Note: Windows bitmaps are bottom-up, so we need to invert Y
    for (int y = startY; y < endY && y < buffer->bitmapHeight; ++y)
    {
        if (y < 0)
            continue;

        u32 *pixel = (u32 *)(baseRow + y * pitch) + startX;

        for (int x = startX; x < endX && x < buffer->bitmapWidth; ++x)
        {
            if (x < 0)
            {
                pixel++;
                continue;
            }

            // Map to 8x8 pattern
            // In bottom-up bitmap: y=0 is bottom of screen, y=height-1 is top
            // So we iterate from bottom to top, and need to invert pattern Y
            int localX = x - startX;
            int localY = y - startY;
            int patternX = (localX * 8) / size;
            int patternY = 7 - ((localY * 8) / size);

            if (patternX >= 0 && patternX < 8 && patternY >= 0 && patternY < 8)
            {
                u8 patternRow = letterPattern[patternY];
                bool bitSet = (patternRow >> (7 - patternX)) & 1;

                if (bitSet)
                {
                    *pixel = (255 << 24) | (255 << 16) | (255 << 8) | 255; // White
                }
            }

            pixel++;
        }
    }
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

    // Render pressed letter in the center at 256x256
    if (g_pressedLetter != 0)
    {
        int centerX = buffer->bitmapWidth / 2;
        int centerY = buffer->bitmapHeight / 2;
        ShitLetterRender(buffer, g_pressedLetter, centerX, centerY, 256);
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
        VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
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

        if (isDown && !wasDown)
        {
            // Track letter keys
            if ((vkcode >= 'A' && vkcode <= 'Z') || (vkcode >= 'a' && vkcode <= 'z'))
            {
                g_pressedLetter = (char)vkcode;
            }
        }
        else if (!isDown && wasDown)
        {
            // Clear letter when key is released
            if ((vkcode >= 'A' && vkcode <= 'Z') || (vkcode >= 'a' && vkcode <= 'z'))
            {
                if (g_pressedLetter == (char)vkcode)
                {
                    g_pressedLetter = 0;
                }
            }
        }

        switch (vkcode)
        {
        case 'Q':
        case VK_ESCAPE:
            g_running = false;
        case 'W':
        case 'S':
        case 'A':
        case 'D':
        case 'E':
        case VK_DOWN:
        case VK_UP:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_SPACE:
        default:
            break;
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
            Win32LoadDSound(window, 48000, 48000 * sizeof(u16) * 2);

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

                        xOffset += stickRX >> 11;
                        yOffset += stickRY >> 11;
                    }
                    else
                    {
                    }
                }

                XINPUT_VIBRATION vibr;
                vibr.wLeftMotorSpeed = 60000;
                vibr.wRightMotorSpeed = 60000;
                // XInputSetState(0, &vibr);

                // Lock and write square wave to sound buffer (once)
                if (g_secondaryBuffer && !g_soundBufferInitialized)
                {
                    void *region1 = 0;
                    void *region2 = 0;
                    DWORD region1Size = 0;
                    DWORD region2Size = 0;

                    DWORD bufferSize = 48000 * sizeof(u16) * 2; // 1 second of audio
                    HRESULT lockResult = g_secondaryBuffer->Lock(
                        0,            // dwOffset
                        bufferSize,   // dwBytes
                        &region1,     // ppvAudioPtr1
                        &region1Size, // pdwAudioBytes1
                        &region2,     // ppvAudioPtr2
                        &region2Size, // pdwAudioBytes2
                        0);           // dwFlags

                    if (SUCCEEDED(lockResult))
                    {
                        // Generate a nice beat pattern
                        u32 sampleRate = 48000;
                        u32 beatLength = sampleRate;         // 1 second beat loop
                        u32 samplesPerBeat = sampleRate / 4; // 4 beats per second (120 BPM)

                        // Write to region 1
                        s16 *sampleOut = (s16 *)region1;
                        DWORD sampleCount = region1Size / (sizeof(s16) * 2); // 2 channels

                        for (DWORD i = 0; i < sampleCount; ++i)
                        {
                            u32 sampleIndex = i;
                            u32 beatPosition = sampleIndex % beatLength;
                            u32 beatInMeasure = beatPosition / samplesPerBeat;
                            u32 positionInBeat = beatPosition % samplesPerBeat;

                            s16 leftSample = 0;
                            s16 rightSample = 0;

                            // Kick drum on beats 0 and 2
                            if (beatInMeasure == 0 || beatInMeasure == 2)
                            {
                                if (positionInBeat < 2000) // Short kick
                                {
                                    f32 t = (f32)positionInBeat / 48000.0f;
                                    f32 envelope = expf(-t * 15.0f);
                                    s16 kick = (s16)(sinf(2.0f * 3.14159f * 60.0f * t) * 12000.0f * envelope);
                                    leftSample += kick;
                                    rightSample += kick;
                                }
                            }

                            // Snare on beats 1 and 3
                            if (beatInMeasure == 1 || beatInMeasure == 3)
                            {
                                if (positionInBeat < 1500) // Short snare
                                {
                                    f32 t = (f32)positionInBeat / 48000.0f;
                                    f32 envelope = expf(-t * 20.0f);
                                    // Snare: mix of noise and tone
                                    s16 snare = (s16)((sinf(2.0f * 3.14159f * 200.0f * t) +
                                                       sinf(2.0f * 3.14159f * 400.0f * t) * 0.5f) *
                                                      8000.0f * envelope);
                                    leftSample += snare;
                                    rightSample += snare;
                                }
                            }

                            // Hi-hat on off-beats
                            if (positionInBeat < 500 && (beatInMeasure == 0 || beatInMeasure == 2))
                            {
                                f32 t = (f32)positionInBeat / 48000.0f;
                                f32 envelope = expf(-t * 30.0f);
                                s16 hihat = (s16)(sinf(2.0f * 3.14159f * 8000.0f * t) * 4000.0f * envelope);
                                leftSample += hihat;
                                rightSample += hihat;
                            }

                            // Clamp to prevent clipping
                            if (leftSample > 32767)
                                leftSample = 32767;
                            if (leftSample < -32768)
                                leftSample = -32768;
                            if (rightSample > 32767)
                                rightSample = 32767;
                            if (rightSample < -32768)
                                rightSample = -32768;

                            *sampleOut++ = leftSample;
                            *sampleOut++ = rightSample;
                        }

                        // Write to region 2 if it exists (wraparound)
                        if (region2)
                        {
                            sampleOut = (s16 *)region2;
                            sampleCount = region2Size / (sizeof(s16) * 2);

                            for (DWORD i = 0; i < sampleCount; ++i)
                            {
                                u32 sampleIndex = region1Size / (sizeof(s16) * 2) + i;
                                u32 beatPosition = sampleIndex % beatLength;
                                u32 beatInMeasure = beatPosition / samplesPerBeat;
                                u32 positionInBeat = beatPosition % samplesPerBeat;

                                s16 leftSample = 0;
                                s16 rightSample = 0;

                                // Kick drum on beats 0 and 2
                                if (beatInMeasure == 0 || beatInMeasure == 2)
                                {
                                    if (positionInBeat < 2000)
                                    {
                                        f32 t = (f32)positionInBeat / 48000.0f;
                                        f32 envelope = expf(-t * 15.0f);
                                        s16 kick = (s16)(sinf(2.0f * 3.14159f * 60.0f * t) * 12000.0f * envelope);
                                        leftSample += kick;
                                        rightSample += kick;
                                    }
                                }

                                // Snare on beats 1 and 3
                                if (beatInMeasure == 1 || beatInMeasure == 3)
                                {
                                    if (positionInBeat < 1500)
                                    {
                                        f32 t = (f32)positionInBeat / 48000.0f;
                                        f32 envelope = expf(-t * 20.0f);
                                        s16 snare = (s16)((sinf(2.0f * 3.14159f * 200.0f * t) +
                                                           sinf(2.0f * 3.14159f * 400.0f * t) * 0.5f) *
                                                          8000.0f * envelope);
                                        leftSample += snare;
                                        rightSample += snare;
                                    }
                                }

                                // Hi-hat on off-beats
                                if (positionInBeat < 500 && (beatInMeasure == 0 || beatInMeasure == 2))
                                {
                                    f32 t = (f32)positionInBeat / 48000.0f;
                                    f32 envelope = expf(-t * 30.0f);
                                    s16 hihat = (s16)(sinf(2.0f * 3.14159f * 8000.0f * t) * 4000.0f * envelope);
                                    leftSample += hihat;
                                    rightSample += hihat;
                                }

                                // Clamp to prevent clipping
                                if (leftSample > 32767)
                                    leftSample = 32767;
                                if (leftSample < -32768)
                                    leftSample = -32768;
                                if (rightSample > 32767)
                                    rightSample = 32767;
                                if (rightSample < -32768)
                                    rightSample = -32768;

                                *sampleOut++ = leftSample;
                                *sampleOut++ = rightSample;
                            }
                        }

                        g_secondaryBuffer->Unlock(region1, region1Size, region2, region2Size);

                        // Start playback
                        g_secondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

                        g_soundBufferInitialized = true;
                        printf("[DEBUG] Beat pattern written to sound buffer and playback started\n");
                    }
                    else
                    {
                        printf("[WARNING] Failed to lock sound buffer\n");
                    }
                }

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
