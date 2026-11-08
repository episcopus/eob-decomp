/* sdl_host.c: SDL3 implementation of platform.h, the game you play (playbook 80, "Platform separation").

   Adapted from poolrad's c/platform/sdl_host.c: one window, one
   streaming texture, scaled; SDL key events become what the hardware would deliver. Unlike poolrad's
   host, which queues BIOS key codes (scan code and ASCII, INT 16h), this one queues set 1 scancodes,
   make and break, because the game reads port 60h itself (E1AAD.ASM). Skeleton status: video,
   keyboard, mouse and time; sound and files are the machine layer's next step. */
#include <stdio.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "platform.h"

#define FB_W 320
#define FB_H 200

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;
static uint32_t rgba[FB_W * FB_H];

#define EVQ 256u
static struct platform_event evq[EVQ];
static unsigned evq_head, evq_tail;

/* SDL scancode (USB HID usage) to the set 1 make code. 0: no key of the original's keyboard.
   Extended keys (the arrow block, keypad Enter, right Ctrl and Alt) carry E0h first; they are
   marked with 0x100 here. */
static uint16_t set1(SDL_Scancode sc)
{
    static const uint8_t letters[26] = {
        0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
        0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C,
    };
    static const uint8_t digits[10] = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};
    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z) {
        return letters[sc - SDL_SCANCODE_A];
    }
    if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_0) {
        return digits[sc - SDL_SCANCODE_1];
    }
    if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F10) {
        return (uint16_t)(0x3B + (sc - SDL_SCANCODE_F1));
    }
    switch (sc) {
    case SDL_SCANCODE_RETURN: return 0x1C;
    case SDL_SCANCODE_ESCAPE: return 0x01;
    case SDL_SCANCODE_BACKSPACE: return 0x0E;
    case SDL_SCANCODE_TAB: return 0x0F;
    case SDL_SCANCODE_SPACE: return 0x39;
    case SDL_SCANCODE_MINUS: return 0x0C;
    case SDL_SCANCODE_EQUALS: return 0x0D;
    case SDL_SCANCODE_LEFTBRACKET: return 0x1A;
    case SDL_SCANCODE_RIGHTBRACKET: return 0x1B;
    case SDL_SCANCODE_BACKSLASH: return 0x2B;
    case SDL_SCANCODE_SEMICOLON: return 0x27;
    case SDL_SCANCODE_APOSTROPHE: return 0x28;
    case SDL_SCANCODE_GRAVE: return 0x29;
    case SDL_SCANCODE_COMMA: return 0x33;
    case SDL_SCANCODE_PERIOD: return 0x34;
    case SDL_SCANCODE_SLASH: return 0x35;
    case SDL_SCANCODE_CAPSLOCK: return 0x3A;
    case SDL_SCANCODE_LSHIFT: return 0x2A;
    case SDL_SCANCODE_RSHIFT: return 0x36;
    case SDL_SCANCODE_LCTRL: return 0x1D;
    case SDL_SCANCODE_LALT: return 0x38;
    case SDL_SCANCODE_RCTRL: return 0x11D;
    case SDL_SCANCODE_RALT: return 0x138;
    case SDL_SCANCODE_UP: return 0x148;
    case SDL_SCANCODE_DOWN: return 0x150;
    case SDL_SCANCODE_LEFT: return 0x14B;
    case SDL_SCANCODE_RIGHT: return 0x14D;
    case SDL_SCANCODE_HOME: return 0x147;
    case SDL_SCANCODE_END: return 0x14F;
    case SDL_SCANCODE_PAGEUP: return 0x149;
    case SDL_SCANCODE_PAGEDOWN: return 0x151;
    case SDL_SCANCODE_INSERT: return 0x152;
    case SDL_SCANCODE_DELETE: return 0x153;
    case SDL_SCANCODE_KP_7: return 0x47;
    case SDL_SCANCODE_KP_8: return 0x48;
    case SDL_SCANCODE_KP_9: return 0x49;
    case SDL_SCANCODE_KP_4: return 0x4B;
    case SDL_SCANCODE_KP_5: return 0x4C;
    case SDL_SCANCODE_KP_6: return 0x4D;
    case SDL_SCANCODE_KP_1: return 0x4F;
    case SDL_SCANCODE_KP_2: return 0x50;
    case SDL_SCANCODE_KP_3: return 0x51;
    case SDL_SCANCODE_KP_ENTER: return 0x11C;
    default: return 0;
    }
}

/* With the game on its own thread (platform_run): the event queue and the frame are shared with it. */
static SDL_Mutex *lock;
static int threaded;
static volatile int game_done;
static int game_result;
static uint8_t frame_pixels[FB_W * FB_H];
static uint8_t frame_dac[256][3];
static int frame_dirty;

static void push(struct platform_event e)
{
    if (lock) SDL_LockMutex(lock);
    if (((evq_tail + 1u) % EVQ) != evq_head) {
        evq[evq_tail] = e;
        evq_tail = (evq_tail + 1u) % EVQ;
    }
    if (lock) SDL_UnlockMutex(lock);
}

static void push_scancode(uint8_t code)
{
    struct platform_event e = {PLATFORM_EVENT_SCANCODE, code, 0, 0, 0};
    push(e);
}

static void pump(void)
{
    SDL_Event ev;
    float fx, fy;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_EVENT_QUIT) {
            struct platform_event e = {PLATFORM_EVENT_QUIT, 0, 0, 0, 0};
            push(e);
        } else if ((ev.type == SDL_EVENT_KEY_DOWN || ev.type == SDL_EVENT_KEY_UP) && !ev.key.repeat) {
            uint16_t code = set1(ev.key.scancode);
            if (code == 0) {
                continue;
            }
            if (code & 0x100) {
                push_scancode(0xE0);
            }
            push_scancode((uint8_t)((code & 0x7F) | (ev.type == SDL_EVENT_KEY_UP ? 0x80 : 0)));
        } else if (ev.type == SDL_EVENT_MOUSE_MOTION || ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                   ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            struct platform_event e = {PLATFORM_EVENT_MOUSE, 0, 0, 0, 0};
            SDL_MouseButtonFlags b = SDL_GetMouseState(&fx, &fy);
            SDL_RenderCoordinatesFromWindow(ren, fx, fy, &fx, &fy);
            fx = fx < 0 ? 0 : fx > FB_W - 1 ? FB_W - 1 : fx;
            fy = fy < 0 ? 0 : fy > FB_H - 1 ? FB_H - 1 : fy;
            e.mouse_x = (uint16_t)(fx * 2);   /* the INT 33h driver's 640-wide range */
            e.mouse_y = (uint16_t)fy;
            e.buttons = (uint8_t)(((b & SDL_BUTTON_LMASK) ? 1 : 0) | ((b & SDL_BUTTON_RMASK) ? 2 : 0));
            push(e);
        }
    }
}

int platform_init(const char *title, int scale)
{
    if (win != NULL) {
        return 0;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "platform: SDL_Init: %s\n", SDL_GetError());
        return -1;
    }
    if (!SDL_CreateWindowAndRenderer(title, FB_W * scale, FB_H * scale * 6 / 5, SDL_WINDOW_RESIZABLE, &win, &ren)) {
        fprintf(stderr, "platform: window: %s\n", SDL_GetError());
        return -1;
    }
    /* 320x200 on a 4:3 screen: the pixels are 1.2 times taller than wide */
    SDL_SetRenderLogicalPresentation(ren, FB_W, FB_H, SDL_LOGICAL_PRESENTATION_STRETCH);
    /* the game and the window share this thread: a present that waits for the display stops the game */
    SDL_SetRenderVSync(ren, SDL_RENDERER_VSYNC_DISABLED);
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, FB_W, FB_H);
    SDL_HideCursor();                   /* the game draws its own pointer (src/E1BBE.ASM) */
    SDL_RaiseWindow(win);               /* opened from a terminal, it would stay behind it on macOS */
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    return tex != NULL ? 0 : -1;
}

void platform_quit(void)
{
    if (threaded) {
        return;                         /* the game's thread does not touch the window: the process's end closes it */
    }
    if (win != NULL) {
        SDL_DestroyTexture(tex);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        win = NULL;
    }
}

static void draw(const uint8_t *pixels, const uint8_t dac[256][3])
{
    unsigned i;
    for (i = 0; i < FB_W * FB_H; i++) {
        const uint8_t *c = dac[pixels[i]];
        /* six-bit DAC components to eight bits, as VGA monitors show them */
        uint32_t r = (uint32_t)((c[0] & 63) * 255 / 63), g = (uint32_t)((c[1] & 63) * 255 / 63),
                 b = (uint32_t)((c[2] & 63) * 255 / 63);
        rgba[i] = (r << 16) | (g << 8) | b;
    }
    SDL_UpdateTexture(tex, NULL, rgba, FB_W * 4);
    SDL_RenderClear(ren);
    SDL_RenderTexture(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
}

void platform_present(const uint8_t pixels[320 * 200], const uint8_t dac[256][3])
{
    if (win == NULL) {
        return;
    }
    if (!threaded) {
        draw(pixels, dac);
        return;
    }
    SDL_LockMutex(lock);
    if (memcmp(frame_pixels, pixels, sizeof frame_pixels) != 0 || memcmp(frame_dac, dac, sizeof frame_dac) != 0) {
        memcpy(frame_pixels, pixels, sizeof frame_pixels);
        memcpy(frame_dac, dac, sizeof frame_dac);
        frame_dirty = 1;
    }
    SDL_UnlockMutex(lock);
}

struct platform_event platform_poll_event(void)
{
    struct platform_event e = {PLATFORM_EVENT_NONE, 0, 0, 0, 0};
    if (win != NULL && !threaded) {
        pump();
    }
    if (lock) SDL_LockMutex(lock);
    if (evq_head != evq_tail) {
        e = evq[evq_head];
        evq_head = (evq_head + 1u) % EVQ;
    }
    if (lock) SDL_UnlockMutex(lock);
    return e;
}

struct game_call {
    int (*fn)(void *);
    void *arg;
};

static int SDLCALL game_thread(void *p)
{
    struct game_call *c = p;
    game_result = c->fn(c->arg);
    game_done = 1;
    return 0;
}

int platform_run(int (*fn)(void *), void *arg)
{
    static struct game_call call;
    SDL_PropertiesID props;
    SDL_Thread *t;
    static uint8_t pixels[FB_W * FB_H];
    static uint8_t dac[256][3];
    if (win == NULL) {
        return fn(arg);                 /* no window (a run with no display): nothing to keep apart */
    }
    lock = SDL_CreateMutex();
    threaded = 1;
    call.fn = fn;
    call.arg = arg;
    props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_THREAD_CREATE_ENTRY_FUNCTION_POINTER, (void *)game_thread);
    SDL_SetStringProperty(props, SDL_PROP_THREAD_CREATE_NAME_STRING, "game");
    SDL_SetPointerProperty(props, SDL_PROP_THREAD_CREATE_USERDATA_POINTER, &call);
    SDL_SetNumberProperty(props, SDL_PROP_THREAD_CREATE_STACKSIZE_NUMBER, 16 * 1024 * 1024);
    t = SDL_CreateThreadWithProperties(props);
    SDL_DestroyProperties(props);
    if (t == NULL) {
        fprintf(stderr, "platform: thread: %s\n", SDL_GetError());
        threaded = 0;
        return fn(arg);
    }
    while (!game_done) {
        int dirty;
        pump();
        SDL_LockMutex(lock);
        dirty = frame_dirty;
        if (dirty) {
            memcpy(pixels, frame_pixels, sizeof pixels);
            memcpy(dac, frame_dac, sizeof dac);
            frame_dirty = 0;
        }
        SDL_UnlockMutex(lock);
        if (dirty) {
            draw(pixels, (const uint8_t (*)[3])dac);    /* it may block here, on this thread only */
        } else {
            SDL_Delay(1);
        }
    }
    SDL_WaitThread(t, NULL);
    return game_result;
}

uint32_t platform_ms(void)
{
    return (uint32_t)SDL_GetTicks();
}

uint32_t platform_poll_mask(void)
{
    return 0x3Fu;
}

void platform_busy(void)
{
}

void platform_sleep_ms(uint32_t ms)
{
    SDL_Delay(ms);
}

void platform_adlib_write(uint8_t reg, uint8_t value)
{
    (void)reg;
    (void)value;
}

void platform_speaker(uint16_t divisor, uint8_t port61_low2)
{
    (void)divisor;
    (void)port61_low2;
}
