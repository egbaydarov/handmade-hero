#if !defined HANDMADE_H

#include <stdint.h>
#define INTERNAL        static
#define GLOBAL_VARIABLE static
#define LOCAL_PERSIST   static
typedef int32_t b32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;
typedef double f64;

typedef struct game_offscreen_buffer
{
    void *bitmapMemory;
    int bitmapWidth;
    int bitmapHeight;
    int bytesPerPixel;
} game_offscreen_buffer;

INTERNAL void
GameUpdateAndRender(game_offscreen_buffer *buffer, s32 blueOffset, s32 greenOffset);

#define HANDMADE_H
#endif
