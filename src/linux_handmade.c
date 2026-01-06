#include <inttypes.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

// Try to include generated protocol header, fallback to manual definitions
#ifdef __has_include
#if __has_include("xdg-shell-client-protocol.h")
#include "xdg-shell-client-protocol.h"
#endif
#endif

#ifndef XDG_WM_BASE_INTERFACE
// Manual XDG shell definitions if header not available
struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;

struct xdg_wm_base_listener
{
    void (*ping)(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);
};

struct xdg_surface_listener
{
    void (*configure)(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
};

struct xdg_toplevel_listener
{
    void (*configure)(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states);
    void (*close)(void *data, struct xdg_toplevel *xdg_toplevel);
};

extern const struct wl_interface xdg_wm_base_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_toplevel_interface;

struct xdg_surface *
xdg_wm_base_get_xdg_surface(struct xdg_wm_base *xdg_wm_base, struct wl_surface *surface);
struct xdg_toplevel *
xdg_surface_get_toplevel(struct xdg_surface *xdg_surface);
void
xdg_surface_add_listener(struct xdg_surface *xdg_surface, const struct xdg_surface_listener *listener, void *data);
void
xdg_surface_ack_configure(struct xdg_surface *xdg_surface, uint32_t serial);
void
xdg_toplevel_add_listener(struct xdg_toplevel *xdg_toplevel, const struct xdg_toplevel_listener *listener, void *data);
void
xdg_toplevel_set_title(struct xdg_toplevel *xdg_toplevel, const char *title);
void
xdg_toplevel_destroy(struct xdg_toplevel *xdg_toplevel);
void
xdg_surface_destroy(struct xdg_surface *xdg_surface);
void
xdg_wm_base_destroy(struct xdg_wm_base *xdg_wm_base);
#endif
#include <alsa/asoundlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <linux/uinput.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <x86intrin.h>

#define GLOBAL_VARIABLE static
#define LOCAL_PERSIST   static
#define INTERNAL        static

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

struct linux_offscreen_buffer
{
    void *bitmapMemory;
    int bitmapWidth;
    int bitmapHeight;
    int bytesPerPixel;
    int pitch;
    struct wl_buffer *buffer;
    int fd;
    // Logical render dimensions (what we render to)
    int renderWidth;
    int renderHeight;
};

struct linux_window_dimension
{
    int width;
    int height;
};

struct linux_sound_output
{
    u16 samplesPerSec;
    u16 toneHz;
    f32 tSine;

    u16 volume;
    s16 lrBalance;
    u16 bytesPerSample;
    u32 secondaryBufferSize;

    u32 runningSampleIndex;
    u32 latencySampleCount;

    snd_pcm_t *pcmHandle;
    s16 *soundBuffer;
};

struct linux_gamepad_state
{
    s16 stickLX;
    s16 stickLY;
    s16 stickRX;
    s16 stickRY;
    b32 buttonA;
    b32 buttonB;
    b32 buttonX;
    b32 buttonY;
    b32 buttonStart;
    b32 buttonBack;
    b32 buttonLShoulder;
    b32 buttonRShoulder;
    b32 dpadUp;
    b32 dpadDown;
    b32 dpadLeft;
    b32 dpadRight;
};

struct wayland_state
{
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_surface *surface;
    struct wl_shm *shm;
    struct xdg_wm_base *wm_base;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *toplevel;
    struct wl_seat *seat;
    struct wl_keyboard *keyboard;
    b32 configured;
    int width;
    int height;
};

GLOBAL_VARIABLE b32 g_running;
GLOBAL_VARIABLE struct linux_offscreen_buffer g_backBuffer;
GLOBAL_VARIABLE struct wayland_state g_wl;
GLOBAL_VARIABLE int g_joystickFd;
GLOBAL_VARIABLE int g_evdevFd;
GLOBAL_VARIABLE struct linux_gamepad_state g_gamepadState;

// Wayland registry listener
static void
registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    struct wayland_state *wl = (struct wayland_state *)data;

    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        wl->compositor = (struct wl_compositor *)wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        wl->shm = (struct wl_shm *)wl_registry_bind(registry, name, &wl_shm_interface, 1);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        wl->wm_base = (struct xdg_wm_base *)wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        wl->seat = (struct wl_seat *)wl_registry_bind(registry, name, &wl_seat_interface, 1);
    }
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

// XDG surface listener
static void
xdg_surface_handle_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
    struct wayland_state *wl = (struct wayland_state *)data;
    xdg_surface_ack_configure(xdg_surface, serial);
    wl->configured = 1;
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_handle_configure,
};

// XDG toplevel listener
static void
xdg_toplevel_handle_configure(void *data, struct xdg_toplevel *toplevel, int32_t width, int32_t height, struct wl_array *states)
{
    struct wayland_state *wl = (struct wayland_state *)data;
    (void)toplevel;
    (void)states;

    if (width > 0 && height > 0)
    {
        wl->width = width;
        wl->height = height;
    }
}

static void
xdg_toplevel_handle_close(void *data, struct xdg_toplevel *toplevel)
{
    (void)data;
    (void)toplevel;
    g_running = 0;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_handle_configure,
    .close = xdg_toplevel_handle_close,
};

// Keyboard listener
static void
keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size)
{
    (void)data;
    (void)keyboard;
    (void)format;
    (void)fd;
    (void)size;
    // No cleanup - OS will handle it
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
    (void)data;
    (void)keyboard;
    (void)serial;
    (void)surface;
    (void)keys;
}

static void
keyboard_handle_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface)
{
    (void)data;
    (void)keyboard;
    (void)serial;
    (void)surface;
}

static void
keyboard_handle_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    (void)data;
    (void)keyboard;
    (void)serial;
    (void)time;

    if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
    {
        // Escape or Q to quit
        if (key == 1 || key == 24) // ESC or Q
        {
            g_running = 0;
        }
    }
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
    (void)data;
    (void)keyboard;
    (void)serial;
    (void)mods_depressed;
    (void)mods_latched;
    (void)mods_locked;
    (void)group;
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_handle_keymap,
    .enter = keyboard_handle_enter,
    .leave = keyboard_handle_leave,
    .key = keyboard_handle_key,
    .modifiers = keyboard_handle_modifiers,
};

static void
seat_handle_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities)
{
    struct wayland_state *wl = (struct wayland_state *)data;

    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        wl->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(wl->keyboard, &keyboard_listener, wl);
    }
    else if (wl->keyboard)
    {
        // No cleanup - OS will handle it
        wl->keyboard = NULL;
    }
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_handle_capabilities,
};

INTERNAL void
LinuxLoadJoystick(void)
{
    const char *joystickPaths[] = {
        "/dev/input/js0",
        "/dev/input/js1",
        "/dev/input/js2",
        "/dev/input/js3",
    };

    const char *evdevPaths[] = {
        "/dev/input/event0",
        "/dev/input/event1",
        "/dev/input/event2",
        "/dev/input/event3",
        "/dev/input/event4",
        "/dev/input/event5",
        "/dev/input/event6",
        "/dev/input/event7",
    };

    for (int i = 0; i < 4; ++i)
    {
        g_joystickFd = open(joystickPaths[i], O_RDONLY | O_NONBLOCK);
        if (g_joystickFd >= 0)
        {
            printf("[DEBUG] joystick opened: %s\n", joystickPaths[i]);

            // Try to open corresponding evdev for force feedback
            // Usually event device number matches or is close
            for (int j = 0; j < 8; ++j)
            {
                g_evdevFd = open(evdevPaths[j], O_RDWR);
                if (g_evdevFd >= 0)
                {
                    // Check if it supports force feedback
                    unsigned long features[4];
                    if (ioctl(g_evdevFd, EVIOCGBIT(EV_FF, sizeof(features)), features) >= 0)
                    {
                        if (features[0] & (1 << FF_RUMBLE))
                        {
                            printf("[DEBUG] evdev opened for vibration: %s\n", evdevPaths[j]);
                            return;
                        }
                    }
                    // Not the right device, try next
                    // No cleanup - just reset pointer
                    g_evdevFd = -1;
                }
            }

            // If we found joystick but no evdev, that's okay
            g_evdevFd = -1;
            return;
        }
    }

    printf("[WARNING] no joystick found\n");
    g_joystickFd = -1;
    g_evdevFd = -1;
}

INTERNAL void
LinuxUpdateGamepadState(void)
{
    if (g_joystickFd < 0)
    {
        return;
    }

    struct js_event event;
    while (read(g_joystickFd, &event, sizeof(event)) == sizeof(event))
    {
        event.type &= ~JS_EVENT_INIT;

        switch (event.type)
        {
        case JS_EVENT_BUTTON:
        {
            switch (event.number)
            {
            case 0:
                g_gamepadState.buttonA = event.value;
                break;
            case 1:
                g_gamepadState.buttonB = event.value;
                break;
            case 2:
                g_gamepadState.buttonX = event.value;
                break;
            case 3:
                g_gamepadState.buttonY = event.value;
                break;
            case 4:
                g_gamepadState.buttonLShoulder = event.value;
                break;
            case 5:
                g_gamepadState.buttonRShoulder = event.value;
                break;
            case 6:
                g_gamepadState.buttonBack = event.value;
                break;
            case 7:
                g_gamepadState.buttonStart = event.value;
                break;
            default:
                // Handle additional buttons if needed
                break;
            }
        }
        break;

        case JS_EVENT_AXIS:
        {
            // Apply deadzone
            s16 deadzone = 8000;
            s16 value = (s16)event.value;
            s16 absValue = (value < 0) ? -value : value;

            switch (event.number)
            {
            case 0: // Left stick X
                g_gamepadState.stickLX = (absValue > deadzone) ? value : 0;
                break;
            case 1: // Left stick Y (inverted in Linux)
                g_gamepadState.stickLY = (absValue > deadzone) ? -value : 0;
                break;
            case 2: // Right stick X (or sometimes trigger on some gamepads)
                // Check if this looks like a trigger (only positive values) or stick (both directions)
                if (value >= 0 && absValue > 10000)
                {
                    // Likely a trigger, ignore
                    break;
                }
                g_gamepadState.stickRX = (absValue > deadzone) ? value : 0;
                break;
            case 3: // Right stick Y (inverted in Linux) (or sometimes trigger)
                if (value >= 0 && absValue > 10000)
                {
                    // Likely a trigger, ignore
                    break;
                }
                g_gamepadState.stickRY = (absValue > deadzone) ? -value : 0;
                break;
            case 4: // Left trigger (usually 0 to 32767)
            case 5: // Right trigger (usually 0 to 32767)
                // Triggers not used in current code, but don't interfere with sticks
                break;
            case 6: // D-pad X
                g_gamepadState.dpadLeft = (value < -10000);
                g_gamepadState.dpadRight = (value > 10000);
                break;
            case 7: // D-pad Y
                g_gamepadState.dpadUp = (value < -10000);
                g_gamepadState.dpadDown = (value > 10000);
                break;
            default:
                // Try to map additional axes - some gamepads have right stick on 4/5
                if (event.number == 4 && value < 0)
                {
                    // Might be right stick X on some gamepads
                    g_gamepadState.stickRX = (absValue > deadzone) ? value : 0;
                }
                else if (event.number == 5 && value < 0)
                {
                    // Might be right stick Y on some gamepads
                    g_gamepadState.stickRY = (absValue > deadzone) ? -value : 0;
                }
                break;
            }
        }
        break;
        }
    }
}

INTERNAL b32
LinuxLoadALSA(
    struct linux_sound_output *soundOutput,
    u32 samplesPerSec,
    u32 bufferSize)
{
    int err;

    if ((err = snd_pcm_open(&soundOutput->pcmHandle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        printf("[WARNING] alsa open failed: %s\n", snd_strerror(err));
        return 0;
    }

    snd_pcm_hw_params_t *hwParams;
    snd_pcm_hw_params_alloca(&hwParams);

    if ((err = snd_pcm_hw_params_any(soundOutput->pcmHandle, hwParams)) < 0)
    {
        printf("[WARNING] alsa hw_params_any failed: %s\n", snd_strerror(err));
        return 0;
    }

    if ((err = snd_pcm_hw_params_set_access(soundOutput->pcmHandle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        printf("[WARNING] alsa set_access failed: %s\n", snd_strerror(err));
        return 0;
    }

    if ((err = snd_pcm_hw_params_set_format(soundOutput->pcmHandle, hwParams, SND_PCM_FORMAT_S16_LE)) < 0)
    {
        printf("[WARNING] alsa set_format failed: %s\n", snd_strerror(err));
        return 0;
    }

    if ((err = snd_pcm_hw_params_set_channels(soundOutput->pcmHandle, hwParams, 2)) < 0)
    {
        printf("[WARNING] alsa set_channels failed: %s\n", snd_strerror(err));
        return 0;
    }

    unsigned int rate = samplesPerSec;
    if ((err = snd_pcm_hw_params_set_rate_near(soundOutput->pcmHandle, hwParams, &rate, 0)) < 0)
    {
        printf("[WARNING] alsa set_rate failed: %s\n", snd_strerror(err));
        return 0;
    }

    snd_pcm_uframes_t frames = bufferSize / (sizeof(s16) * 2);
    if ((err = snd_pcm_hw_params_set_buffer_size_near(soundOutput->pcmHandle, hwParams, &frames)) < 0)
    {
        printf("[WARNING] alsa set_buffer_size failed: %s\n", snd_strerror(err));
        return 0;
    }

    if ((err = snd_pcm_hw_params(soundOutput->pcmHandle, hwParams)) < 0)
    {
        printf("[WARNING] alsa hw_params failed: %s\n", snd_strerror(err));
        return 0;
    }

    soundOutput->soundBuffer = (s16 *)malloc(bufferSize);
    if (!soundOutput->soundBuffer)
    {
        printf("[WARNING] failed to allocate sound buffer\n");
        return 0;
    }

    printf("[DEBUG] alsa initialized\n");
    return 1;
}

INTERNAL struct linux_window_dimension
LinuxGetWindowDimension(void)
{
    struct linux_window_dimension result;
    result.width = g_wl.width;
    result.height = g_wl.height;
    return result;
}

INTERNAL void
RenderShit(
    struct linux_offscreen_buffer *buffer,
    int xOffset,
    int yOffset)
{
    int pitch = buffer->bitmapWidth * buffer->bytesPerPixel;
    u8 *row = (u8 *)buffer->bitmapMemory;

    // Scale coordinates from logical render size to actual buffer size
    f32 scaleX = (f32)buffer->renderWidth / (f32)buffer->bitmapWidth;
    f32 scaleY = (f32)buffer->renderHeight / (f32)buffer->bitmapHeight;

    for (int y = 0; y < buffer->bitmapHeight; ++y)
    {
        u32 *pixel = (u32 *)row;

        for (int x = 0; x < buffer->bitmapWidth; ++x)
        {
            // Map buffer coordinates to logical render coordinates
            int logicalX = (int)((f32)x * scaleX);
            int logicalY = (int)((f32)y * scaleY);

            u8 r = 0;
            u8 b = (u8)(logicalX + xOffset);
            u8 g = (u8)(logicalY + yOffset);

            // ARGB format (0xaarrggbb)
            *pixel++ = (255 << 24) | (r << 16) | (g << 8) | b;
        }

        row += pitch;
    }
}

INTERNAL void
LinuxResizeDIBSection(
    struct linux_offscreen_buffer *buffer,
    int width,
    int height)
{
    // Store logical render dimensions (what we render to)
    buffer->renderWidth = width;
    buffer->renderHeight = height;

    // Use actual window size for buffer (or keep existing if window not configured yet)
    int bufferWidth = (g_wl.width > 0) ? g_wl.width : width;
    int bufferHeight = (g_wl.height > 0) ? g_wl.height : height;

    // Only recreate if size actually changed
    if (buffer->bitmapWidth == bufferWidth && buffer->bitmapHeight == bufferHeight && buffer->buffer)
    {
        return;
    }

    // No cleanup - just overwrite pointers
    buffer->buffer = NULL;
    buffer->bitmapMemory = NULL;
    buffer->fd = -1;

    buffer->bitmapWidth = bufferWidth;
    buffer->bitmapHeight = bufferHeight;
    buffer->bytesPerPixel = 4;
    buffer->pitch = buffer->bitmapWidth * buffer->bytesPerPixel;

    int bitmapMemorySize = buffer->bytesPerPixel * buffer->bitmapWidth * buffer->bitmapHeight;

    // Create shared memory file
    char name[] = "/tmp/handmade-shm-XXXXXX";
    buffer->fd = mkstemp(name);
    if (buffer->fd < 0)
    {
        printf("[ERROR] failed to create shm file\n");
        return;
    }

    unlink(name);
    ftruncate(buffer->fd, bitmapMemorySize);

    buffer->bitmapMemory = mmap(NULL, bitmapMemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, buffer->fd, 0);
    if (buffer->bitmapMemory == MAP_FAILED)
    {
        printf("[ERROR] failed to mmap shm\n");
        return;
    }

    memset(buffer->bitmapMemory, 0, bitmapMemorySize);

    // Create wl_buffer from shared memory
    struct wl_shm_pool *pool = wl_shm_create_pool(g_wl.shm, buffer->fd, bitmapMemorySize);
    buffer->buffer = wl_shm_pool_create_buffer(pool, 0, bufferWidth, bufferHeight, buffer->pitch, WL_SHM_FORMAT_XRGB8888);
    // No cleanup - pool will be freed by compositor

    printf("Buffer: %dx%d, Render: %dx%d p=0x%p\n", bufferWidth, bufferHeight, width, height, buffer->bitmapMemory);
}

INTERNAL void
LinuxUpdateWindow(
    struct linux_offscreen_buffer *buffer,
    int windowWidth,
    int windowHeight)
{
    if (buffer->buffer && g_wl.surface)
    {
        wl_surface_attach(g_wl.surface, buffer->buffer, 0, 0);
        wl_surface_damage(g_wl.surface, 0, 0, windowWidth, windowHeight);
        wl_surface_commit(g_wl.surface);
    }
}

INTERNAL void
LinuxFillSoundBuffer(
    struct linux_sound_output *soundOutput,
    u32 samplesToWrite)
{
    if (!soundOutput->soundBuffer || !soundOutput->pcmHandle)
    {
        return;
    }

    s16 *out = soundOutput->soundBuffer;
    for (u32 i = 0; i < samplesToWrite; ++i)
    {
        f32 sine = sinf(soundOutput->tSine);
        *out++ = (s16)(sine * (soundOutput->volume - soundOutput->lrBalance));
        *out++ = (s16)(sine * (soundOutput->volume + soundOutput->lrBalance));
        soundOutput->tSine += (2.0 * M_PI) * (f32)1.0f / ((f32)soundOutput->samplesPerSec / soundOutput->toneHz);
        ++soundOutput->runningSampleIndex;
    }

    snd_pcm_sframes_t framesWritten = snd_pcm_writei(
        soundOutput->pcmHandle,
        soundOutput->soundBuffer,
        samplesToWrite);

    if (framesWritten < 0)
    {
        framesWritten = snd_pcm_recover(soundOutput->pcmHandle, framesWritten, 0);
        if (framesWritten < 0)
        {
            printf("[WARNING] alsa write failed: %s\n", snd_strerror(framesWritten));
        }
    }
}

int
main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    memset(&g_wl, 0, sizeof(g_wl));
    g_wl.width = 1280;
    g_wl.height = 720;
    memset(&g_backBuffer, 0, sizeof(g_backBuffer));
    g_backBuffer.fd = -1;
    g_evdevFd = -1;

    g_wl.display = wl_display_connect(NULL);
    if (!g_wl.display)
    {
        printf("Failed to connect to Wayland display.\n");
        return 1;
    }

    g_wl.registry = wl_display_get_registry(g_wl.display);
    wl_registry_add_listener(g_wl.registry, &registry_listener, &g_wl);
    wl_display_roundtrip(g_wl.display);

    if (!g_wl.compositor || !g_wl.shm || !g_wl.wm_base)
    {
        printf("Failed to get required Wayland interfaces.\n");
        wl_display_disconnect(g_wl.display);
        return 1;
    }

    g_wl.surface = wl_compositor_create_surface(g_wl.compositor);
    g_wl.xdg_surface = xdg_wm_base_get_xdg_surface(g_wl.wm_base, g_wl.surface);
    xdg_surface_add_listener(g_wl.xdg_surface, &xdg_surface_listener, &g_wl);
    g_wl.toplevel = xdg_surface_get_toplevel(g_wl.xdg_surface);
    xdg_toplevel_add_listener(g_wl.toplevel, &xdg_toplevel_listener, &g_wl);
    xdg_toplevel_set_title(g_wl.toplevel, "Handmade Hero");
    wl_surface_commit(g_wl.surface);

    if (g_wl.seat)
    {
        wl_seat_add_listener(g_wl.seat, &seat_listener, &g_wl);
    }

    wl_display_roundtrip(g_wl.display);

    LinuxLoadJoystick();
    // Initialize with logical render size, actual buffer will be created when window is configured
    g_backBuffer.renderWidth = 1920;
    g_backBuffer.renderHeight = 1080;

    int xOffset = 0;
    int yOffset = 0;

    struct linux_sound_output soundOutput = {};
    soundOutput.samplesPerSec = 48000;
    soundOutput.toneHz = 512;
    soundOutput.tSine = 0;
    soundOutput.volume = 1000;
    soundOutput.bytesPerSample = sizeof(u16) * 2;
    soundOutput.secondaryBufferSize = soundOutput.samplesPerSec * soundOutput.bytesPerSample;
    soundOutput.latencySampleCount = soundOutput.samplesPerSec / 20;

    if (!LinuxLoadALSA(&soundOutput, soundOutput.samplesPerSec, soundOutput.secondaryBufferSize))
    {
        printf("[WARNING] sound initialization failed, continuing without sound\n");
    }
    else
    {
        LinuxFillSoundBuffer(&soundOutput, soundOutput.latencySampleCount);
    }

    g_running = 1;

    struct timespec startTime;
    clock_gettime(CLOCK_MONOTONIC, &startTime);
    u64 lastCounter = startTime.tv_sec * 1000000000ULL + startTime.tv_nsec;
    u64 lastCounterCycles = __rdtsc();

    while (g_running)
    {
        // Dispatch Wayland events
        while (wl_display_prepare_read(g_wl.display) != 0)
        {
            wl_display_dispatch_pending(g_wl.display);
        }
        wl_display_flush(g_wl.display);

        // Set up poll for joystick
        struct pollfd fds[2];
        fds[0].fd = wl_display_get_fd(g_wl.display);
        fds[0].events = POLLIN;
        fds[1].fd = g_joystickFd;
        fds[1].events = POLLIN;

        int pollCount = (g_joystickFd >= 0) ? 2 : 1;
        poll(fds, pollCount, 0);

        if (fds[0].revents & POLLIN)
        {
            wl_display_read_events(g_wl.display);
            wl_display_dispatch_pending(g_wl.display);
        }
        else
        {
            wl_display_cancel_read(g_wl.display);
        }

        LinuxUpdateGamepadState();

        if (g_joystickFd >= 0)
        {
            xOffset += g_gamepadState.stickRX / 5000;
            yOffset += g_gamepadState.stickRY / 5000;

            soundOutput.toneHz = 512 + (s32)(256.f * ((f32)g_gamepadState.stickLY / 32768.0f));
            soundOutput.lrBalance = (s16)(1000.f * ((f32)g_gamepadState.stickLX / 32768.0f));

            if (g_gamepadState.buttonB)
            {
                // Trigger vibration using force feedback
                if (g_evdevFd >= 0)
                {
                    struct ff_effect effect;
                    effect.type = FF_RUMBLE;
                    effect.id = -1;
                    effect.u.rumble.strong_magnitude = 0x5000;
                    effect.u.rumble.weak_magnitude = 0x5000;
                    effect.replay.length = 200; // 200ms
                    effect.replay.delay = 0;

                    if (ioctl(g_evdevFd, EVIOCSFF, &effect) >= 0)
                    {
                        struct input_event play;
                        play.type = EV_FF;
                        play.code = effect.id;
                        play.value = 1;
                        write(g_evdevFd, &play, sizeof(play));

                        // Stop after duration
                        usleep(200000); // 200ms
                        play.value = 0;
                        write(g_evdevFd, &play, sizeof(play));
                    }
                }
            }
        }

        if (soundOutput.pcmHandle)
        {
            snd_pcm_sframes_t avail = snd_pcm_avail(soundOutput.pcmHandle);
            if (avail > 0)
            {
                u32 samplesToWrite = (avail < soundOutput.latencySampleCount) ? (u32)avail : soundOutput.latencySampleCount;
                LinuxFillSoundBuffer(&soundOutput, samplesToWrite);
            }
        }

        if (g_wl.configured)
        {
            struct linux_window_dimension windowDim = LinuxGetWindowDimension();

            // Recreate buffer if window size changed
            if (windowDim.width > 0 && windowDim.height > 0)
            {
                LinuxResizeDIBSection(&g_backBuffer, 1920, 1080); // Logical render size
            }

            RenderShit(&g_backBuffer, xOffset, yOffset);
            LinuxUpdateWindow(&g_backBuffer, windowDim.width, windowDim.height);
        }

        struct timespec endTime;
        clock_gettime(CLOCK_MONOTONIC, &endTime);
        u64 endCounter = endTime.tv_sec * 1000000000ULL + endTime.tv_nsec;
        u64 endCounterCycles = __rdtsc();

        u64 counterElapsed = endCounter - lastCounter;
        u64 perfMillis = counterElapsed / 1000000;
        u64 cyclesElapsed = endCounterCycles - lastCounterCycles;

        if (perfMillis > 0)
        {
            printf(
                "\r%4" PRIu64 " f/s, %2" PRIu64 " ms/f, %3" PRIu64 " mc/f",
                1000 / perfMillis,
                perfMillis,
                cyclesElapsed / (1000 * 1000));
            fflush(stdout);
        }

        lastCounterCycles = endCounterCycles;
        lastCounter = endCounter;
    }

    // No cleanup - OS will handle it when process exits
    return 0;
}
