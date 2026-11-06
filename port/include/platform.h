/* platform.h: the host boundary of the portable build (playbook 80, "The platform layer").

   Adapted from poolrad's src/platform/platform.h (branch track-c-hybrid). The difference that shapes
   it: Pool of Radiance asks the BIOS for keys and ticks, so its hooks are calls the game makes;
   Eye of the Beholder drives its own hardware (whitepaper 2.7): an INT 9 handler reads scancodes
   from port 60h, an INT 33h event handler takes mouse moves and buttons, INT 8 and INT 1Ch handlers
   run its sound drivers and timers, and the AdLib is written at ports 388h and 389h. So the host
   here only produces hardware events and consumes hardware output; the machine layer
   (port/runtime, planned) turns them into the interrupts, port reads and DOS and BIOS services the
   game's code expects, and it is the machine layer that a replay records and plays back.

   The host never sees the arena or the game. Everything below is a plain value.

   Status (2026-11-05): video present and the event pump are implemented by port/platform/sdl_host.c
   (null.c, headless, answers them with nothing); the files by port/platform/files.c for both; time
   and sound are declared for the plan and land with the machine's clock and devices (M3, M7). */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

/* ---- Video ----------------------------------------------------------------------------------- */

/* Show one frame: 320 by 200 indexed pixels (the MCGA driver's page 0, A000:0000) and the 256 DAC
   entries as the game loaded them, six bits a component (port 3C9h, INT 10h AX=1012h). */
void platform_present(const uint8_t pixels[320 * 200], const uint8_t dac[256][3]);

/* ---- Input ----------------------------------------------------------------------------------- */

/* One hardware input event, in the form the original's hardware delivers it. */
enum platform_event_kind {
    PLATFORM_EVENT_NONE = 0,
    PLATFORM_EVENT_SCANCODE,  /* a byte the keyboard controller puts at port 60h (set 1: make
                                 codes, break codes with bit 7, E0h prefixes): one INT 9 each */
    PLATFORM_EVENT_MOUSE,     /* the mouse moved or a button changed: x 0..639 and y 0..199 in the
                                 INT 33h driver's range, buttons bit 0 left, bit 1 right */
    PLATFORM_EVENT_QUIT       /* the window was closed */
};

struct platform_event {
    enum platform_event_kind kind;
    uint8_t scancode;
    uint16_t mouse_x, mouse_y;
    uint8_t buttons;
};

/* poll: the next pending event, or kind NONE. Never blocks. */
struct platform_event platform_poll_event(void);

/* ---- Time ------------------------------------------------------------------------------------ */

/* poll: milliseconds of wall time since start. The machine layer turns it into PIT periods (INT 8
   at the divisor the game programmed, INT 1Ch at the BIOS rate) in a live run; a replay never reads
   it. */
uint32_t platform_ms(void);

/* wait: let the host sleep until about `ms` milliseconds have passed (a live run that is ahead of
   the PIT). */
void platform_sleep_ms(uint32_t ms);

/* ---- Sound (planned) --------------------------------------------------------------------------- */

/* One AdLib (OPL2) register write, register then value, as the game's driver writes ports 388h and
   389h; the host synthesises it (open question 9 of playbook 80: which OPL emulator). */
void platform_adlib_write(uint8_t reg, uint8_t value);

/* The PC speaker: PIT channel 2 divisor and the two low bits of port 61h (E1948.ASM). */
void platform_speaker(uint16_t divisor, uint8_t port61_low2);

/* ---- Files ----------------------------------------------------------------------------------- */

/* The game's files live in one directory (the user's copy, never eob/ itself while the game can write
   to it: AGENTS.md). The machine (port/runtime/dos.c) keeps DOS's view of them as the oracle's VFS
   does: it reads a file once, whole, and keeps every write in memory. Names are DOS 8.3, upper case;
   a host matches them without case. */
int32_t platform_file_size(const char *name);                           /* -1: no such file */
int platform_file_read(const char *name, uint8_t *buf, uint32_t size);  /* the whole file; 0 or -1 */

/* where platform_file_* look (port/platform/files.c; a host with no directory of its own) */
void platform_set_game_dir(const char *dir);

/* ---- Lifetime -------------------------------------------------------------------------------- */

/* Open the window (live runs; a replay or a test runs headless and never calls it). 0, or -1 with a
   message on stderr. */
int platform_init(const char *title, int scale);
void platform_quit(void);

#endif
