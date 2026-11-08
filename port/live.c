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
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tcport.h"
#include "machine.h"
#include "ported.h"
#include "start.h"
#include "platform.h"
#include "tclib.h"

static u32 start_ms, last_present_ms, next_irq0_us;
static u32 irq0_pushed;          /* the timer interrupts pushed, on the count of machine_irq0_delivered */
#define IRQ0_BACKLOG 3u          /* how many may wait for a place to be delivered: the host's hiccups cost no ticks */
static u32 seen_ms = 0xFFFFFFFFu;  /* the millisecond `more` last worked in */
static u32 present_ms, present_n;  /* what the presents cost (TC_PLAY_STATS) */

#define NO_SNAPSHOT 0xFFFFFFFFu
static u32 snapshot_ms = NO_SNAPSHOT;     /* TC_PLAY_SNAPSHOT=ms:path;...: the screen and DAC at those times */
static const char *snapshot_list;
static char snapshot_path[1024];
static u32 end_ms = 0xFFFFFFFFu;          /* TC_PLAY_END=ms: the run ends then */

/* the next entry of the list, if any */
static void snapshot_next(void)
{
    const char *colon, *semi;
    size_t n;
    snapshot_ms = NO_SNAPSHOT;
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

/* TC_NULL_INPUT, on any host: set 1 scancodes (hexadecimal) each at a time in milliseconds of the run,
   "40000:50,40100:D0"; an entry "MS:Mx.y.b" is the mouse at the screen's pixel (x, y) with the buttons b
   (1 left, 2 right). Each comes once the clock has reached its time. */
static struct platform_event script_next(u32 now_ms)
{
    static const char *next;
    static int started;
    struct platform_event e = {PLATFORM_EVENT_NONE, 0, 0, 0, 0};
    char *end;
    unsigned long at;
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
    if (end[1] == 'M') {
        e.kind = PLATFORM_EVENT_MOUSE;
        e.mouse_x = (u16)(strtoul(end + 2, &end, 10) * 2);   /* the driver's 640-wide range, as sdl_host.c */
        e.mouse_y = (u16)strtoul(end + 1, &end, 10);
        e.buttons = (u8)strtoul(end + 1, &end, 10);
    } else {
        e.kind = PLATFORM_EVENT_SCANCODE;
        e.scancode = (u8)strtoul(end + 1, &end, 16);
    }
    next = *end == ',' ? end + 1 : end;
    return e;
}

/* what TC_PLAY_STATS reports when the run ends: the timer ticks that came due and how many the game got */
static u32 ticks_due;

static void report(void)
{
    if (getenv("TC_PLAY_STATS") != NULL) {
        fprintf(stderr, "eob-port: stats: %lu ms, tick period %lu us, ticks due %lu, delivered %lu, %lu presents %lu ms\n",
                (unsigned long)(platform_ms() - start_ms), (unsigned long)machine_irq0_period(),
                (unsigned long)ticks_due, (unsigned long)machine_irq0_delivered, (unsigned long)present_n,
                (unsigned long)present_ms);
    }
}

/* The clock is looked at often (every access of the machine and every tc_poll_mask loop turns), so the work
   is done once per millisecond of it. */
static void tick(u32 now_ms)
{
    u32 now_us = now_ms * 1000u;
    struct platform_event p;
    for (;;) {
        machine_event e;
        p = platform_poll_event();
        if (p.kind == PLATFORM_EVENT_NONE) {
            p = script_next(now_ms);
        }
        if (p.kind == PLATFORM_EVENT_NONE) {
            break;
        }
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
    if ((s32)(now_us - next_irq0_us) >= 0) {             /* the microseconds wrap at 71 minutes */
        ticks_due++;
        if (irq0_pushed - machine_irq0_delivered < IRQ0_BACKLOG) {
            machine_event e;
            memset(&e, 0, sizeof e);
            e.kind = MACHINE_IRQ0;
            push(&e);
            irq0_pushed++;
        }
        next_irq0_us += machine_irq0_period();
        if ((s32)(now_us - next_irq0_us) > 100000) {
            next_irq0_us = now_us;                       /* behind by more than 0.1 s: the ticks are lost */
        }
    }
    while (snapshot_ms != NO_SNAPSHOT && now_ms >= snapshot_ms) {
        snapshot();
    }
    if (now_ms >= end_ms) {
        report();
        platform_quit();
        fprintf(stderr, "eob-port: the run reached its end time\n");
        exit(0);
    }
    if (now_ms - last_present_ms >= 16u) {
        u32 t = platform_ms();
        present();
        present_ms += platform_ms() - t;
        present_n++;
        last_present_ms = now_ms;
    }
}

static void more(int waiting)
{
    for (;;) {
        u32 now_ms = platform_ms() - start_ms;
        machine_time.insns = now_ms * 1000u;
        if (now_ms != seen_ms) {
            seen_ms = now_ms;
            tick(now_ms);
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

/* the intro's hand-off: execl(EOB.EXE, ...), which comes here through tcl_execl */
static jmp_buf chained;
static char chain_tail[128];

static void chain(const char *tail)
{
    snprintf(chain_tail, sizeof chain_tail, "%s", tail);
    longjmp(chained, 1);
}

/* a program of its own: the machine as DOS leaves it for a new one */
static void boot(const char *exe, const char *cmdline, int mouse_on, int adlib)
{
    memset(tc_mem, 0, TC_MEM_SIZE);
    memset(tc_hot, 0, sizeof tc_hot);
    tc_state = 0;
    machine_events_reset();
    machine_boot(exe, (const u8 *)cmdline, (u16)strlen(cmdline), mouse_on, adlib);
    machine_replay = 1;                         /* delivered as in a replay: lazily, by the machine */
    machine_cclock = 0xFFFFFFFFu;
    machine_more = more;
    tc_live = 1;
    tc_poll_mask = platform_poll_mask();
    irq0_pushed = machine_irq0_delivered;
    next_irq0_us = (platform_ms() - start_ms) * 1000u + machine_irq0_period();
}

struct play_args {
    const char *cmdline, *intro_cmdline;
    int mouse_on, adlib;
};

/* the intro and the game: on the game's own thread where the host has one (platform_run) */
static int play_all(void *p)
{
    struct play_args *a = p;
    const char *volatile cmdline = a->cmdline;
    farptr argv;
    s16 argc, rc;
    start_ms = platform_ms();
    if (a->intro_cmdline != NULL) {
        if (setjmp(chained) == 0) {
            tc_exec_hook = chain;
            boot("INTRO.EXE", a->intro_cmdline, a->mouse_on, a->adlib);
            argc = tc_start(&argv);
            intro_load_data();
            Intro_Main(argc, argv);
        }
        if (chain_tail[0]) {
            cmdline = chain_tail;
        }
    }
    boot("EOB.EXE", cmdline, a->mouse_on, a->adlib);
    argc = tc_start(&argv);
    rc = Main(argc, argv);
    platform_quit();
    return rc;
}

int live_run(const char *game_dir, const char *save_dir, const char *cmdline, const char *intro_cmdline, int mouse_on,
             int adlib)
{
    static struct play_args a;
    if (platform_init("Eye of the Beholder", 3) != 0 && getenv("TC_PLAY_SNAPSHOT") == NULL && getenv("TC_PLAY_END") == NULL) {
        return 2;                               /* no window, and not the headless smoke test */
    }
    platform_set_game_dir(game_dir);
    platform_set_save_dir(save_dir);
    fprintf(stderr, "eob-port: playing the game in %s (Exit to DOS, or close the window, to quit)\n", game_dir);
    tc_fail_hook = ended;
    snapshot_list = getenv("TC_PLAY_SNAPSHOT");
    snapshot_next();
    if (getenv("TC_PLAY_END") != NULL) {
        end_ms = (u32)strtoul(getenv("TC_PLAY_END"), NULL, 10);
    }
    a.cmdline = cmdline;
    a.intro_cmdline = intro_cmdline;
    a.mouse_on = mouse_on;
    a.adlib = adlib;
    return platform_run(play_all, &a);
}
