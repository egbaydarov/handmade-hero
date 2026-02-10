#if !defined HANDMADE_H

#include <stdint.h>
#include <stdio.h>
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

#if HANDMADE_INTERNAL
#else
#endif

#if HANDMADE_SLOW
#define ASSERT(x)                                                                   \
    do                                                                              \
    {                                                                               \
        if (!(x))                                                                   \
        {                                                                           \
            fprintf(stderr, "ASSERT failed: %s (%s:%d)\n", #x, __FILE__, __LINE__); \
            abort();                                                                \
        }                                                                           \
    } while (0)
#else
#define ASSERT(x)
#endif
#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))
#define KILOBYTES(x)   (x) * 1024
#define MEGABYTES(x)   (x) * 1024 * 1024
#define GIGABYTES(x)   ((u64)(x) * 1024ULL * 1024ULL * 1024ULL)
#define TERABYTES(x)   ((u64)(x) * 1024ULL * 1024ULL * 1024ULL * 1024ULL)

typedef struct game_memory
{
    u64 permanentStorageSize;
    void *permanentStorage; // NOTE: required to be zeroed by platform

    u64 transientStorageSize;
    void *transientStorage; // NOTE: required to be zeroed by platform

    b32 isInitialized;
} game_memory;

typedef struct game_button_state
{
    s32 halfTransitionsCount;
    b32 endedDown;
} game_button_state;

typedef struct game_controller_input
{
    b32 isAnalog;
    f32 startX;
    f32 startY;

    f32 minX;
    f32 minY;

    f32 maxX;
    f32 maxY;

    f32 endX;
    f32 endY;

    union
    {
        game_button_state buttons[6];
        struct
        {
            game_button_state Triangle;
            game_button_state Square;
            game_button_state Cross;
            game_button_state Circle;
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
        };
    };
} game_controller_input;

typedef struct game_input
{
    game_controller_input controllers[4];
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
    game_memory *memory,
    game_offscreen_buffer *buffer,
    game_sound_output_buffer *soundBuffer,
    game_input *gameInput);

typedef struct game_clocks
{
    f64 secondsElapsed;
} game_clocks;

typedef struct game_state
{
    s32 blueOffset;
    s32 greenOffset;
    s32 toneHz;
} game_state;

#define HANDMADE_H
#endif
