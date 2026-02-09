#include "handmade.h"
#include <math.h>

INTERNAL void
GameOutputSound(
    game_sound_output_buffer *soundBuffer,
    game_input *gameInput)
{
    LOCAL_PERSIST s32 volume = 750;
    LOCAL_PERSIST f32 tSine = 0;

    s16 *samples = soundBuffer->samples;
    s32 samplesCount = soundBuffer->samplesCount;

    for (s32 i = 0; i < samplesCount; ++i)
    {
        f32 sine = sinf(tSine);
        *samples++ = (s16)(sine * (volume - (s16)(1000.f * ((f32)gameInput->lxOffset / 32768.0f))));
        *samples++ = (s16)(sine * (volume + (s16)(1000.f * ((f32)gameInput->lxOffset / 32768.0f))));
        tSine += (2.0 * M_PI) * (f32)1.0f / ((f32)soundBuffer->samplesPerSec / 512 + (s32)(256.f * ((f32)gameInput->lyOffset / 32768.0f)));
    }
}

INTERNAL void
RenderShit(
    game_offscreen_buffer *buffer,
    game_input *gameInput)
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
            u8 b = (u8)x + gameInput->rxOffset;
            u8 g = (u8)y + gameInput->ryOffset;

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
    game_input *gameInput)
{
    // TODO(byda): allow sample offset here for more robust platform options
    GameOutputSound(soundBuffer, gameInput);
    RenderShit(buffer, gameInput);
}
