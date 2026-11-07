/* replay.c: a golden session played headless on the port (playbook 80, M5, "The replay").

   The recording is a text file tools/port/replay.py writes from the golden's clock recording
   (tests/sessions/golden/<name>.clock.json): the command tail and the devices, each interrupt with the
   number of the game's accesses made before it, the oracle's instruction count at the game's and the
   library's accesses (the timer, the retrace bit and the time of day read it), and each checkpoint's
   place: how many interrupts were delivered and how many accesses made before it. The machine delivers
   the interrupts by their access counts (machine_replay, irq.c); at each place a checkpoint can be
   taken (machine_point) the next checkpoint is taken if the port is at its place, and the arena, the
   DAC and the video mode are written to <dump_dir>/<name>.bin for replay.py to compare with the
   golden. After the last checkpoint the replay ends: exit 0. It ends with 3 when the port passes a
   checkpoint's place without being at it (it went another way), and with the runtime's code when it
   stops (tc_fail: a wait with no interrupt to come, a service the machine lacks).

   The lines:
     cmdline <the command tail, its leading space included>
     mouse 0|1
     adlib 0|1
     irq0 <access> [<file calls>]
     irq1 <access> <scancode> [<file calls>]
     mouse_event <access> <condition> <buttons> <x> <y> [<file calls>]
     time <access> <instructions>
     ltime <library access> <instructions>
     checkpoint <name> <interrupts> <accesses> [<file calls>] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tcport.h"
#include "machine.h"
#include "ported.h"
#include "start.h"
#include "platform.h"

#define MAX_CP 64

static struct {
    char name[64];
    u32 events, accesses, files;   /* files ~0: any */
} cps[MAX_CP];
static unsigned n_cp, next_cp;
static const char *dumps;

static void dump(const char *name)
{
    char path[1024];
    FILE *f;
    snprintf(path, sizeof path, "%s/%s.bin", dumps, name);
    f = fopen(path, "wb");
    if (f == NULL || fwrite(tc_mem, 1, 0xB0000u, f) != 0xB0000u || fwrite(video.dac, 1, 768, f) != 768 ||
        fputc(video.mode, f) == EOF) {
        fprintf(stderr, "replay: cannot write %s\n", path);
        exit(2);
    }
    fclose(f);
}

static void point(u32 events, u32 accesses, int idle)
{
    (void)idle;
    while (next_cp < n_cp && cps[next_cp].events == events && cps[next_cp].accesses == accesses &&
           (cps[next_cp].files == 0xFFFFFFFFu || cps[next_cp].files == machine_files)) {
        dump(cps[next_cp].name);
        printf("replay: %s after %lu interrupts and %lu accesses\n", cps[next_cp].name, (unsigned long)events,
               (unsigned long)accesses);
        next_cp++;
    }
    if (next_cp == n_cp) {
        printf("replay: all %u checkpoints taken\n", n_cp);
        fflush(stdout);
        exit(0);
    }
    if (events > cps[next_cp].events || (events == cps[next_cp].events && accesses > cps[next_cp].accesses) ||
        (events == cps[next_cp].events && accesses == cps[next_cp].accesses && cps[next_cp].files != 0xFFFFFFFFu &&
         machine_files > cps[next_cp].files)) {
        printf("replay: %s not reached: its place is after %lu interrupts and %lu accesses, the port is at %lu "
               "and %lu\n", cps[next_cp].name, (unsigned long)cps[next_cp].events,
               (unsigned long)cps[next_cp].accesses, (unsigned long)events, (unsigned long)accesses);
        dump(cps[next_cp].name);
        fflush(stdout);
        exit(3);
    }
}

int replay_run(const char *script, const char *game_dir, const char *dump_dir)
{
    FILE *f = fopen(script, "r");
    char line[512], cmd[200] = "";
    int mouse = 0, adlib = 0;
    farptr argv;
    s16 argc, rc;
    if (f == NULL) {
        fprintf(stderr, "replay: cannot read %s\n", script);
        return 2;
    }
    dumps = dump_dir;
    platform_set_game_dir(game_dir);
    machine_events_reset();
    while (fgets(line, sizeof line, f) != NULL) {
        machine_event e;
        unsigned long a, b, c, d, x, y;
        char name[64];
        memset(&e, 0, sizeof e);
        line[strcspn(line, "\n")] = 0;
        if (strncmp(line, "cmdline ", 8) == 0) {
            snprintf(cmd, sizeof cmd, "%s", line + 8);
        } else if (sscanf(line, "mouse %d", &mouse) == 1 || sscanf(line, "adlib %d", &adlib) == 1) {
        } else if ((d = 0, sscanf(line, "irq0 %lu %lu", &a, &d)) >= 1) {
            e.access = (u32)a;
            e.files = (u32)d;
            e.kind = MACHINE_IRQ0;
            machine_event_push(&e);
        } else if ((d = 0, sscanf(line, "irq1 %lu %lu %lu", &a, &b, &d)) >= 2) {
            e.access = (u32)a;
            e.files = (u32)d;
            e.kind = MACHINE_IRQ1;
            e.scancode = (u8)b;
            machine_event_push(&e);
        } else if ((d = 0, sscanf(line, "mouse_event %lu %lu %lu %lu %lu %lu", &a, &b, &c, &x, &y, &d)) >= 5) {
            e.access = (u32)a;
            e.files = (u32)d;
            e.kind = MACHINE_MOUSE;
            e.cond = (u16)b;
            e.buttons = (u16)c;
            e.x = (u16)x;
            e.y = (u16)y;
            machine_event_push(&e);
        } else if (sscanf(line, "time %lu %lu", &a, &b) == 2) {
            machine_time_push((u32)a, (u32)b);
        } else if (sscanf(line, "ltime %lu %lu", &a, &b) == 2) {
            machine_library_time_push((u32)a, (u32)b);
        } else if ((x = 0xFFFFFFFFul, sscanf(line, "checkpoint %63s %lu %lu %lu", name, &c, &d, &x)) >= 3) {
            if (n_cp == MAX_CP) {
                fprintf(stderr, "replay: more than %d checkpoints\n", MAX_CP);
                return 2;
            }
            snprintf(cps[n_cp].name, sizeof cps[n_cp].name, "%s", name);
            cps[n_cp].events = (u32)c;
            cps[n_cp].accesses = (u32)d;
            cps[n_cp].files = (u32)x;
            n_cp++;
        } else if (line[0] != 0 && line[0] != '#') {
            fprintf(stderr, "replay: %s: cannot read the line: %s\n", script, line);
            return 2;
        }
    }
    fclose(f);
    if (getenv("TC_TRACE") != NULL) {
        machine_trace = fopen(getenv("TC_TRACE"), "w");
        tc_hot_watch = 1;
    }
    machine_boot("EOB.EXE", (const u8 *)cmd, (u16)strlen(cmd), mouse, adlib);
    machine_replay = 1;
    machine_cclock = 0xFFFFFFFFu;               /* the C clock is not counted: by accesses alone */
    machine_point = point;
    argc = tc_start(&argv);
    rc = Main(argc, argv);
    printf("replay: main returned %d before %s\n", rc, next_cp < n_cp ? cps[next_cp].name : "the end");
    return 3;
}
