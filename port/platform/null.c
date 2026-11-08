/* null.c: the headless implementation of platform.h (playbook 80, "Platform separation").

   No window, no input, no sound: a run that needs none of them (the self-test, and later the
   per-function tests) links this instead of sdl_host.c, so the game, the runtime and the machine
   are built and run without SDL. It is poolrad's c/platform/null.c for this port. Time is a counter
   that platform_sleep_ms and platform_busy advance, so a run never depends on the wall clock.

   With TC_NULL_REALTIME set the clock is the wall clock and the program sleeps for real, as in a window: what
   `make port-playtest` uses to check the game's pace. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "platform.h"

static uint32_t now_ms;

static uint32_t platform_window_mask(void) { return 0x3Fu; }   /* sdl_host.c's */

int platform_init(const char *title, int scale)
{
    (void)title;
    (void)scale;
    fprintf(stderr, "platform: this build has no window (null platform); build eob-port with SDL3 for one\n");
    return -1;
}

void platform_quit(void)
{
}

int platform_run(int (*fn)(void *), void *arg)
{
    return fn(arg);
}

void platform_present(const uint8_t pixels[320 * 200], const uint8_t dac[256][3])
{
    (void)pixels;
    (void)dac;
}

struct platform_event platform_poll_event(void)
{
    struct platform_event e = {PLATFORM_EVENT_NONE, 0, 0, 0, 0};
    return e;
}

static int realtime(void)
{
    static int r = -1;
    if (r < 0) {
        r = getenv("TC_NULL_REALTIME") != NULL;
    }
    return r;
}

static uint32_t wall_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint32_t)((uint64_t)t.tv_sec * 1000u + (uint64_t)t.tv_nsec / 1000000u);
}

uint32_t platform_ms(void)
{
    return realtime() ? wall_ms() : now_ms;
}

/* a program that computes without sleeping still sees time pass, as in a window */
uint32_t platform_poll_mask(void)
{
    return realtime() ? platform_window_mask() : 0x3FFu;
}

void platform_busy(void)
{
    now_ms += realtime() ? 0u : 4u;
}

void platform_sleep_ms(uint32_t ms)
{
    if (realtime()) {
        struct timespec t = {0, (long)ms * 1000000L};
        nanosleep(&t, NULL);
    } else {
        now_ms += ms;
    }
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
