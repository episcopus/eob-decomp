/* live.c: the game played live in a window (playbook 80, M7, "Live play").

   The same program as the replay (port/start.c, then Main), with the interrupts made as they come
   instead of read from a recording: the machine asks for them where it would deliver one (before an
   access or a file call, at a wait: machine_more, irq.c), and this pushes what has happened since:

   - IRQ 0 at the rate the game programmed into the PIT (channel 0's divisor, machine_irq0_period on
     the machine's clock of 1,000,000 instructions a second), from the wall clock; the machine's clock
     (machine_time.insns, which the PIT counter, the retrace bit and the time of day read) is the wall
     clock's microseconds. At most one timer interrupt waits at a time, as on the PC's interrupt
     controller: a tick that comes while one is waiting is lost, and a backlog of more than a tenth of a
     second is dropped;
   - IRQ 1 for each set 1 scancode the host gives;
   - the INT 33h driver's event for a mouse move or button change, when the game has a handler for it
     (the driver's state too, which function 3 reads);
   - and every 16 ms the screen: A000:0000 through the DAC (platform_present).

   At a wait with nothing due it sleeps until the next tick. TC_PLAY_SNAPSHOT=MS:PATH[;MS:PATH...], in
   order of time, writes the screen (64,000 bytes) and the DAC (768) to each PATH once its MS
   milliseconds have passed, and TC_PLAY_END=MS ends the run then (exit 0): on the headless program,
   whose clock counts its own sleeps and looks, a live run is the same every time (the smoke test and
   `make port-playtest`). The window's close ends the program, as the game's own exit does (DOS 4Ch). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tcport.h"
#include "machine.h"
#include "ported.h"
#include "start.h"
#include "platform.h"

static u32 start_ms, last_present_ms, next_irq0_us;
static u8 irq0_waiting;          /* a timer interrupt pushed and not delivered yet */
static u32 pushed, irq0_at;      /* the events pushed; the ordinal of the waiting IRQ 0 */

static u32 snapshot_ms = 0xFFFFFFFFu;     /* TC_PLAY_SNAPSHOT=ms:path;...: the screen and DAC at those times */
static const char *snapshot_list;
static char snapshot_path[1024];
static u32 end_ms = 0xFFFFFFFFu;          /* TC_PLAY_END=ms: the run ends then */

/* the next entry of the list, if any */
static void snapshot_next(void)
{
    const char *colon, *semi;
    size_t n;
    snapshot_ms = 0xFFFFFFFFu;
    if (snapshot_list == NULL || *snapshot_list == 0 || (colon = strchr(snapshot_list, ':')) == NULL) {
        return;
    }
    snapshot_ms = (u32)strtoul(snapshot_list, NULL, 10);
    semi = strchr(colon, ';');
    n = semi ? (size_t)(semi - colon - 1) : strlen(colon + 1);
    if (n >= sizeof snapshot_path) {
        n = sizeof snapshot_path - 1;
    }
    memcpy(snapshot_path, colon + 1, n);
    snapshot_path[n] = 0;
    snapshot_list = semi ? semi + 1 : "";
}

static void snapshot(void)
{
    FILE *f = fopen(snapshot_path, "wb");
    if (f != NULL) {
        fwrite(tc_mem + 0xA0000u, 1, 64000u, f);
        fwrite(video.dac, 1, 768u, f);
        fclose(f);
    }
    snapshot_next();
}

static void present(void)
{
    platform_present(tc_mem + 0xA0000u, (const u8 (*)[3])video.dac);
}

static void push(machine_event *e)
{
    machine_event_push(e);
    pushed++;
}

static void mouse_event(const struct platform_event *p)
{
    u16 cond = 0, x = p->mouse_x, y = p->mouse_y;
    machine_event e;
    if (x < mouse.range_x0) x = mouse.range_x0;
    if (mouse.range_x1 && x > mouse.range_x1) x = mouse.range_x1;
    if (y < mouse.range_y0) y = mouse.range_y0;
    if (mouse.range_y1 && y > mouse.range_y1) y = mouse.range_y1;
    if (x != mouse.x || y != mouse.y) {
        cond |= 0x01u;
    }
    if ((p->buttons & 1u) && !(mouse.buttons & 1u)) cond |= 0x02u;
    if (!(p->buttons & 1u) && (mouse.buttons & 1u)) cond |= 0x04u;
    if ((p->buttons & 2u) && !(mouse.buttons & 2u)) cond |= 0x08u;
    if (!(p->buttons & 2u) && (mouse.buttons & 2u)) cond |= 0x10u;
    mouse.x = x;
    mouse.y = y;
    mouse.buttons = p->buttons;
    if (cond == 0 || !mouse.has_handler || !(cond & mouse.handler_mask)) {
        return;
    }
    memset(&e, 0, sizeof e);
    e.kind = MACHINE_MOUSE;
    e.cond = cond;
    e.buttons = mouse.buttons;
    e.x = x;
    e.y = y;
    push(&e);
}

static void more(int waiting)
{
    for (;;) {
        u32 now_ms = platform_ms() - start_ms, now_us = now_ms * 1000u;
        struct platform_event p;
        machine_time.insns = now_us;
        while ((p = platform_poll_event()).kind != PLATFORM_EVENT_NONE) {
            machine_event e;
            memset(&e, 0, sizeof e);
            if (p.kind == PLATFORM_EVENT_QUIT) {
                platform_quit();
                fprintf(stderr, "eob-port: the window was closed\n");
                exit(0);
            }
            if (p.kind == PLATFORM_EVENT_SCANCODE) {
                e.kind = MACHINE_IRQ1;
                e.scancode = p.scancode;
                push(&e);
            } else if (p.kind == PLATFORM_EVENT_MOUSE) {
                mouse_event(&p);
            }
        }
        if (irq0_waiting && machine_events_delivered() >= irq0_at) {
            irq0_waiting = 0;
        }
        if (now_us >= next_irq0_us) {
            if (!irq0_waiting) {
                machine_event e;
                memset(&e, 0, sizeof e);
                e.kind = MACHINE_IRQ0;
                push(&e);
                irq0_waiting = 1;
                irq0_at = machine_events_delivered() + machine_events_left();
            }
            next_irq0_us += machine_irq0_period();
            if (now_us > next_irq0_us + 100000u) {
                next_irq0_us = now_us;          /* behind by more than 0.1 s: the ticks are lost */
            }
        }
        while (now_ms >= snapshot_ms) {
            snapshot();
        }
        if (now_ms >= end_ms) {
            platform_quit();
            fprintf(stderr, "eob-port: the run reached its end time\n");
            exit(0);
        }
        if (now_ms - last_present_ms >= 16u) {
            present();
            last_present_ms = now_ms;
        }
        if (!waiting || machine_events_left() > 0) {
            return;
        }
        platform_sleep_ms(1);
    }
}

/* the program's end (DOS 4Ch reaches tc_fail with 76): close the window, no message */
static void ended(int code, const char *msg)
{
    platform_quit();
    if (code == 76) {
        fprintf(stderr, "eob-port: the game ended (exit code %d)\n", machine_exit_code);
        exit(machine_exit_code > 0 ? machine_exit_code : 0);
    }
    fprintf(stderr, "eob-port: %s\n", msg);
    exit(code);
}

int live_run(const char *game_dir, const char *save_dir, const char *cmdline, int mouse_on, int adlib)
{
    farptr argv;
    s16 argc, rc;
    if (platform_init("Eye of the Beholder", 3) != 0 && getenv("TC_PLAY_SNAPSHOT") == NULL) {
        return 2;                               /* no window, and not the headless smoke test */
    }
    platform_set_game_dir(game_dir);
    platform_set_save_dir(save_dir);
    fprintf(stderr, "eob-port: playing the game in %s (Exit to DOS, or close the window, to quit)\n", game_dir);
    tc_fail_hook = ended;
    machine_events_reset();
    machine_boot("EOB.EXE", (const u8 *)cmdline, (u16)strlen(cmdline), mouse_on, adlib);
    machine_replay = 1;                         /* delivered as in a replay: lazily, by the machine */
    machine_cclock = 0xFFFFFFFFu;
    machine_more = more;
    tc_live = 1;
    snapshot_list = getenv("TC_PLAY_SNAPSHOT");
    snapshot_next();
    if (getenv("TC_PLAY_END") != NULL) {
        end_ms = (u32)strtoul(getenv("TC_PLAY_END"), NULL, 10);
    }
    start_ms = platform_ms();
    next_irq0_us = machine_irq0_period();
    argc = tc_start(&argv);
    rc = Main(argc, argv);
    platform_quit();
    return rc;
}
