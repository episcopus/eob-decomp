/* null.c: the headless implementation of platform.h (playbook 80, "Platform separation").

   No window, no input, no sound: a run that needs none of them (the self-test, and later the
   per-function tests) links this instead of sdl_host.c, so the game, the runtime and the machine
   are built and run without SDL. It is poolrad's c/platform/null.c for this port. Time is a counter
   that platform_sleep_ms advances, so a run never depends on the wall clock. The replay platform
   (M5) is the third implementation: it answers from a recording of the original. */
#include <stdio.h>

#include "platform.h"

static uint32_t now_ms;

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

uint32_t platform_ms(void)
{
    return now_ms;
}

void platform_sleep_ms(uint32_t ms)
{
    now_ms += ms;
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
