#include "handmade.h"
#include <math.h>
#include <windows.h>

INTERNAL void
GameOutputSound(
    game_sound_output_buffer *soundBuffer,
    s32 lrBalance,
    s32 toneHz)
{
    LOCAL_PERSIST s32 volume = 750;
    LOCAL_PERSIST f32 tSine = 0;

    s16 *samples = soundBuffer->samples;
    s32 samplesCount = soundBuffer->samplesCount;

    for (s32 i = 0; i < samplesCount; ++i)
    {
        f32 sine = sinf(tSine);
        *samples++ = (s16)(sine * (volume - lrBalance));
        *samples++ = (s16)(sine * (volume + lrBalance));
        tSine += (2.0 * M_PI) * (f32)1.0f / ((f32)soundBuffer->samplesPerSec / toneHz);
    }
}

INTERNAL void
RenderShit(
    game_offscreen_buffer *buffer,
    s32 blueOffset,
    s32 greenOffset)
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
            u8 b = (u8)x + blueOffset;
            u8 g = (u8)y + greenOffset;

            // win format (bbggrraa) : le format (aarrggbb)
            *pixel++ = (255 << 24) | (r << 16) | (g << 8) | b;

            //*pixel++ = 0xFF0000FF;
        }

        row += pitch;
    }
}

INTERNAL void
GameUpdateAndRender(
    game_offscreen_buffer *buffer,
    game_sound_output_buffer *soundBuffer,
    game_input *input)
{
    LOCAL_PERSIST s32 blueOffset = 0;
    LOCAL_PERSIST s32 greenOffset = 0;
    LOCAL_PERSIST s32 toneHz = 512;

    game_controller_input *input0 = &input->controllers[0];

    if (input0->isAnalog)
    {
        toneHz = 256 + (int)(128.0f * (input0->endX));
        blueOffset += (int)(4.0f * (input0->endY));
    }
    else
    {
    }

    if (input0->Triangle.endedDown)
    {
        greenOffset += 1;
    }

    GameOutputSound(soundBuffer, 0, toneHz);
    RenderShit(buffer, blueOffset, greenOffset);
}
