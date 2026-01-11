#include "handmade.h"

INTERNAL void
RenderShit(
    game_offscreen_buffer *buffer,
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
GameUpdateAndRender(game_offscreen_buffer *buffer, s32 blueOffset, s32 greenOffset)
{
    RenderShit(buffer, blueOffset, greenOffset);
}
