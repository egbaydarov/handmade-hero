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

typedef struct game_input
{
    u16 rxOffset;
    u16 ryOffset;
    u16 lxOffset;
    u16 lyOffset;
} game_input;

typedef struct game_offscreen_buffer
{
    void *bitmapMemory;
    int bitmapWidth;
    int bitmapHeight;
    int bytesPerPixel;
} game_offscreen_buffer;

typedef struct game_sound_output_buffer
{
    s16 *samples;
    s32 samplesCount;
    u16 samplesPerSec;
} game_sound_output_buffer;

typedef struct game_window_dimension
{
    int width;
    int height;
} game_window_dimension;

INTERNAL void
GameUpdateAndRender(
    game_offscreen_buffer *buffer,
    game_sound_output_buffer *soundBuffer,
    game_input *gameInput);

#define HANDMADE_H
#endif
