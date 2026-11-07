/* null.c: the headless implementation of platform.h (playbook 80, "Platform separation").

   No window, no input, no sound: a run that needs none of them (the self-test, and later the
   per-function tests) links this instead of sdl_host.c, so the game, the runtime and the machine
   are built and run without SDL. It is poolrad's c/platform/null.c for this port. Time is a counter
   that platform_sleep_ms advances, so a run never depends on the wall clock.

   Input for a test of live play (M7): TC_NULL_INPUT lists scancodes (set 1, hexadecimal) each at a
   time in milliseconds of that clock, "40000:50,40100:D0,41000:1C,41100:9C" (keypad 2 pressed and
   released, then Enter); each comes from platform_poll_event once the clock has reached its time. */
#include <stdio.h>
#include <stdlib.h>

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
    static const char *next;
    static int started;
    struct platform_event e = {PLATFORM_EVENT_NONE, 0, 0, 0, 0};
    char *end;
    unsigned long at, code;
    if (!started) {
        started = 1;
        next = getenv("TC_NULL_INPUT");
    }
    if (next == NULL || *next == 0) {
        return e;
    }
    at = strtoul(next, &end, 10);
    if (*end != ':' || at > now_ms) {
        return e;
    }
    code = strtoul(end + 1, &end, 16);
    next = *end == ',' ? end + 1 : end;
    e.kind = PLATFORM_EVENT_SCANCODE;
    e.scancode = (uint8_t)code;
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
