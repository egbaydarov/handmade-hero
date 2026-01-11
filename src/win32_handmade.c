#include "handmade.c"

#include <dsound.h>
#include <float.h>
#include <math.h>
#include <profileapi.h>
#include <stdio.h>
#include <windows.h>
#include <winnt.h>
#include <xinput.h>

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

struct win32_sound_output
{
    u16 samplesPerSec;
    u16 toneHz;
    f32 tSine;

    u16 volume;
    s16 lrBalance;
    u16 bytesPerSample;
    u16 secondaryBufferSize;

    u32 runningSampleIndex;
    u32 latencySampleCount;
};

GLOBAL_VARIABLE b32 g_running;
GLOBAL_VARIABLE struct win32_offscreen_buffer g_backBuffer;
GLOBAL_VARIABLE IDirectSoundBuffer *g_secondaryBuffer;

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
Win32LoadDirectSound(
    HWND window,
    u32 samplesPerSec,
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
            waveFormat.nSamplesPerSec = samplesPerSec;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
            waveFormat.cbSize = 0;

            printf("[DEBUG dll loaded\n");
            if (SUCCEEDED(directSound->lpVtbl->SetCooperativeLevel(directSound, window, DSSCL_PRIORITY)))
            {
                printf("[DEBUG] dsound set level ok\n");
                DSBUFFERDESC bufferDesc = {};
                bufferDesc.dwSize = sizeof(bufferDesc);
                bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

                IDirectSoundBuffer *primaryBuffer;
                if (SUCCEEDED(directSound->lpVtbl->CreateSoundBuffer(directSound, &bufferDesc, &primaryBuffer, 0)))
                {
                    if (SUCCEEDED(primaryBuffer->lpVtbl->SetFormat(primaryBuffer, &waveFormat)))
                    {
                        printf("[DEBUG] dsound primary buffer created\n");
                        DSBUFFERDESC secondaryBufferDesc = {};
                        secondaryBufferDesc.dwSize = sizeof(secondaryBufferDesc);
                        secondaryBufferDesc.dwBufferBytes = bufferSize;
                        secondaryBufferDesc.lpwfxFormat = &waveFormat;

                        if (SUCCEEDED(directSound->lpVtbl->CreateSoundBuffer(directSound, &secondaryBufferDesc, &g_secondaryBuffer, 0)))
                        {
                            printf("[DEBUG] dsound secondary buffer created\n");
                        }
                        else
                        {
                            printf("[WARNING] dsound create sb failed\n");
                        }
                    }
                    else
                    {
                        printf("[WARNING] dsound set pb format failed\n");
                    }
                }
                else
                {
                    printf("[WARNING] dsound create pb failed\n");
                }
            }
            else
            {
                printf("[WARNING] dsound set level failed\n");
            }
        }
        else
        {
            printf("[WARNING] dsound dll load failed\n");
        }
    }
    else
    {
        printf("[WARNING] dsound dll not found\n");
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

INTERNAL struct win32_window_dimension
Win32GetWindowDimension(
    HWND window)
{
    struct win32_window_dimension result;
    RECT rect;
    BOOL ok = GetClientRect(window, &rect);

    // TODO(byda): any case to fail?
    (void)ok;

    result.width = rect.right - rect.left;
    result.height = rect.bottom - rect.top;

    return (result);
}

INTERNAL void
Win32ResizeDIBSection(
    struct win32_offscreen_buffer *buffer,
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
    struct win32_offscreen_buffer *buffer,
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
        b32 wasDown = ((lParam & (1 << 30)) != 0);
        b32 isDown = ((lParam & (1 << 31)) == 0);

        if (isDown && !wasDown)
        {
            // Track letter keys
        }
        else if (!isDown && wasDown)
        {
            // Clear letter when key is released
        }

        switch (vkcode)
        {
        case 'Q':
        case VK_ESCAPE:
            g_running = 0;
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
        g_running = 0;
    }
    break;

    case WM_CLOSE:
    {
        g_running = 0;
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC hdc = BeginPaint(window, &paint);

        // TODO(byda): workaround

        struct win32_window_dimension windowDim = Win32GetWindowDimension(window);
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
        result = DefWindowProcA(window, message, wParam, lParam);
    }
    break;
    }

    return result;
}

INTERNAL VOID
Win32FillSoundBuffer(
    struct win32_sound_output *soundOutput,
    DWORD byteToLock,
    DWORD bytesToWrite)
{
    void *region1 = 0;
    void *region2 = 0;
    DWORD region1Size = 0;
    DWORD region2Size = 0;

    HRESULT lockResult = g_secondaryBuffer->lpVtbl->Lock(
        g_secondaryBuffer,
        byteToLock,
        bytesToWrite,
        &region1,
        &region1Size,
        &region2,
        &region2Size,
        0);

    if (SUCCEEDED(lockResult))
    {
        s16 *out = (s16 *)region1;
        DWORD samples = region1Size / (sizeof(s16) * 2);
        for (DWORD i = 0; i < samples; ++i)
        {
            f32 sine = sinf(soundOutput->tSine);
            *out++ = (s16)(sine * (soundOutput->volume - soundOutput->lrBalance));
            *out++ = (s16)(sine * (soundOutput->volume + soundOutput->lrBalance));
            soundOutput->tSine += (2.0 * M_PI) * (f32)1.0f / ((f32)soundOutput->samplesPerSec / soundOutput->toneHz);
            ++soundOutput->runningSampleIndex;
        }
        out = (s16 *)region2;
        samples = region2Size / (sizeof(s16) * 2);
        for (DWORD i = 0; i < samples; ++i)
        {
            f32 sine = sinf(soundOutput->tSine);
            *out++ = (s16)(sine * (soundOutput->volume - soundOutput->lrBalance));
            *out++ = (s16)(sine * (soundOutput->volume + soundOutput->lrBalance));
            soundOutput->tSine += (2.0 * M_PI) * (f32)1.0f / ((f32)soundOutput->samplesPerSec / soundOutput->toneHz);
            ++soundOutput->runningSampleIndex;
        }
        g_secondaryBuffer->lpVtbl->Unlock(g_secondaryBuffer, region1, region1Size, region2, region2Size);
    }
    else
    {
        printf("[WARNING] Failed to lock sound buffer\n");
    }
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
    Win32ResizeDIBSection(&g_backBuffer, 1920, 1080);

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
            s32 xOffset = 0;
            s32 yOffset = 0;

            struct win32_sound_output soundOutput = {};
            soundOutput.samplesPerSec = 48000;
            soundOutput.toneHz = 512;
            soundOutput.tSine = 0;
            soundOutput.volume = 1000;
            soundOutput.bytesPerSample = sizeof(u16) * 2;
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSec * soundOutput.bytesPerSample;
            soundOutput.latencySampleCount = soundOutput.samplesPerSec / 20;

            Win32LoadDirectSound(
                window,
                soundOutput.samplesPerSec,
                soundOutput.secondaryBufferSize);

            Win32FillSoundBuffer(&soundOutput, 0, soundOutput.latencySampleCount * soundOutput.bytesPerSample);
            g_secondaryBuffer->lpVtbl->Play(g_secondaryBuffer, 0, 0, DSBPLAY_LOOPING);

            // NOTE(byda): depends on window creation flag (should I create a
            // hdc each iteration or not)
            HDC hdc = GetDC(window);
            g_running = 1;

            LARGE_INTEGER perfCounterFreqWin;
            QueryPerformanceFrequency(&perfCounterFreqWin);
            u64 perfCounterFreq = perfCounterFreqWin.QuadPart;

            LARGE_INTEGER lastCounter;
            QueryPerformanceCounter(&lastCounter);

            u64 lastCounterCycles = __rdtsc();

            while (g_running)
            {
                MSG msg;
                while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                    {
                        g_running = 0;
                    }

                    TranslateMessage(&msg);
                    DispatchMessageA(&msg);
                }

                for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
                {
                    XINPUT_STATE state;

                    if (SUCCEEDED(XInputGetState(i, &state)))
                    {
                        XINPUT_GAMEPAD *gamepad = &state.Gamepad;

                        b32 dpadUp = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        b32 dpadDown = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        b32 dpadLeft = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        b32 dpadRight = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        b32 start = gamepad->wButtons & XINPUT_GAMEPAD_START;
                        b32 back = gamepad->wButtons & XINPUT_GAMEPAD_BACK;
                        b32 a = gamepad->wButtons & XINPUT_GAMEPAD_A;
                        b32 b = gamepad->wButtons & XINPUT_GAMEPAD_B;
                        b32 x = gamepad->wButtons & XINPUT_GAMEPAD_X;
                        b32 y = gamepad->wButtons & XINPUT_GAMEPAD_Y;
                        b32 lShoulder = gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        b32 rShoulder = gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        b32 lThumb = gamepad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
                        b32 rThumb = gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
                        SHORT stickLX = gamepad->sThumbLX;
                        SHORT stickLY = gamepad->sThumbLY;
                        SHORT stickRX = gamepad->sThumbRX;
                        SHORT stickRY = gamepad->sThumbRY;

                        xOffset += stickRX / 5000;
                        yOffset += stickRY / 5000;

                        soundOutput.toneHz = 512 + (s32)(256.f * ((f32)stickLY / 32768.0f));
                        soundOutput.lrBalance = (s16)(1000.f * ((f32)stickLX / 32768.0f));
                        if (b)
                        {
                            XINPUT_VIBRATION vibr;
                            vibr.wLeftMotorSpeed = 5000;
                            vibr.wRightMotorSpeed = 5000;
                            HRESULT setResult = XInputSetState(i, &vibr);
                            printf("%ld", setResult);
                        }
                    }
                    else
                    {
                    }
                }

                if (g_secondaryBuffer)
                {
                    DWORD playCursor;
                    DWORD writeCursor;
                    DWORD status;
                    if (
                        SUCCEEDED(g_secondaryBuffer->lpVtbl->GetCurrentPosition(g_secondaryBuffer, &playCursor, &writeCursor)) &&
                        SUCCEEDED(g_secondaryBuffer->lpVtbl->GetStatus(g_secondaryBuffer, &status)))
                    {
                        DWORD byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;
                        DWORD bytesToWrite;
                        DWORD targetCursor = (playCursor + (soundOutput.latencySampleCount * soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize;

                        if (byteToLock > targetCursor)
                        {
                            bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
                            bytesToWrite += targetCursor;
                        }
                        else
                        {
                            bytesToWrite = targetCursor - byteToLock;
                        }

                        Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
                    }
                    else
                    {
                        printf("[WARNING] Failed to get sound buffer info\n");
                    }
                }

                game_offscreen_buffer buffer = {};
                buffer.bitmapMemory = g_backBuffer.bitmapMemory;
                buffer.bitmapWidth = g_backBuffer.bitmapWidth;
                buffer.bitmapHeight = g_backBuffer.bitmapHeight;
                buffer.bytesPerPixel = g_backBuffer.bytesPerPixel;

                GameUpdateAndRender(&buffer, xOffset, yOffset);

                struct win32_window_dimension windowDim = Win32GetWindowDimension(
                    window);
                Win32UpdateWindow(
                    &g_backBuffer,
                    hdc,
                    windowDim.width,
                    windowDim.height);

                LARGE_INTEGER endCounter;
                QueryPerformanceCounter(&endCounter);

                u64 endCounterCycles = __rdtsc();
                u64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
                u64 perfMillis = counterElapsed / (perfCounterFreq / 1000);
                u64 cyclesElapsed = endCounterCycles - lastCounterCycles;
#if 0
                printf(
                    "\r%4llu f/s, %2llu ms/f, %3llu mc/f",
                    1000 / perfMillis,
                    perfMillis,
                    cyclesElapsed / (1000 * 1000));
#endif

                lastCounterCycles = endCounterCycles;
                lastCounter = endCounter;
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
