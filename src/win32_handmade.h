#if !defined WIN32_HANDMADE_H

#include <windows.h>

#include "handmade.h"

typedef struct win32_offscreen_buffer
{
    BITMAPINFO bitmapInfo;
    void *bitmapMemory;
    int bitmapWidth;
    int bitmapHeight;
    int bytesPerPixel;
} win32_offscreen_buffer;

typedef struct win32_game_sound
{
    u16 samplesPerSec;

    u16 toneHz;

    u16 bytesPerSample;
    u32 secondaryBufferSize;

    u32 runningSampleIndex;
    u32 latencySampleCount;
} win32_game_sound;

#define WIN32_HANDMADE_H
#endif
