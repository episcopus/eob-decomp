/* irq.c: the interrupts the hardware raises and the clock the machine reads, on the proof's clock
   (playbook 80, "The interrupts and the C clock"; machine.h).

   The events come in the order they are due, each with its position on the C clock (C-unit
   instructions) and the number of the game's accesses (port accesses and service calls, the library's
   apart) made before it: a recording's (tests/sessions/golden/<name>.clock.json, tools/oracle/cclock.py)
   when the port replays a session, the bench's in the per-function test, the host's in live play
   (M7). An event is delivered once both counts have reached its own: after each access
   (machine_access), at the C units' block boundaries (M5), and while a hand-written routine spins on
   what a handler writes (machine_wait). The oracle's instruction count, which its timer, retrace
   bit, AdLib timers and time of day read, is set from the same recording at the access where it
   changed. */
#include <stdlib.h>
#include <string.h>

#include "tcport.h"
#include "machine.h"

u32 machine_cclock;
u32 machine_accesses;

static machine_event *events;
static u32 event_count, event_cap, event_next;
static machine_time_mark *times;
static u32 time_count, time_cap, time_next;
static int delivering;

#ifdef TC_HYBRID
/* the hybrid build: the bench's devices take the interrupt (its in-service flags, the scancode in port
   60h, the mouse event and handler in the ROM's trampoline data) and its clock is set; 1 when done */
int tc_hybrid_raise(const machine_event *e);
int tc_hybrid_set_insns(u32 insns);
#endif

void machine_events_reset(void)
{
    event_count = event_next = 0;
    time_count = time_next = 0;
    machine_cclock = 0;
    machine_accesses = 0;
    delivering = 0;
}

static void *grow(void *p, u32 *cap, size_t size)
{
    u32 n = *cap ? *cap * 2u : 1024u;
    void *q = realloc(p, (size_t)n * size);
    if (q == NULL) {
        tc_fail(70, "out of memory for the machine's events");
    }
    *cap = n;
    return q;
}

void machine_event_push(const machine_event *e)
{
    if (event_count == event_cap) {
        events = grow(events, &event_cap, sizeof *events);
    }
    events[event_count++] = *e;
}

void machine_time_push(u32 access, u32 insns)
{
    if (time_count == time_cap) {
        times = grow(times, &time_cap, sizeof *times);
    }
    times[time_count].access = access;
    times[time_count].insns = insns;
    time_count++;
}

u32 machine_events_left(void) { return event_count - event_next; }

static int due(const machine_event *e)
{
    return e->at <= machine_cclock && e->access <= machine_accesses;
}

/* what the CPU and the interrupt controller do for the event, then its handler */
static void deliver(const machine_event *e)
{
    int done = 0;
#ifdef TC_HYBRID
    done = tc_hybrid_raise(e);
#endif
    switch (e->kind) {
    case MACHINE_IRQ0:
        if (!done) {
            hw.irq0_in_service = 1;
        }
        machine_irq(0x08);
        return;
    case MACHINE_IRQ1:
        if (!done) {
            hw.kbd_data = e->scancode;
            hw.irq1_in_service = 1;
        }
        machine_irq(0x09);
        return;
    case MACHINE_MOUSE: {
        /* the driver's event: its data and the game's handler in the trampoline's data, then the
           trampoline (input.py Mouse.deliver); written in the arena in the hybrid build too, where the
           bench's own ROM has it, since the handler in C reads its registers there */
        tc_regs r;
        {
            static const u16 off[] = {0x3080u, 0x3082u, 0x3084u, 0x3086u, 0x3088u, 0x308Au};
            u16 v[6];
            unsigned i;
            v[0] = e->cond;
            v[1] = e->buttons;
            v[2] = e->x;
            v[3] = e->y;
            v[4] = v[5] = 0;
            for (i = 0; i < 6; i++) {
                tc_st_u16(TC_MKFP(MACHINE_ROM_SEG, off[i]), v[i]);
            }
            tc_st_u16(TC_MKFP(MACHINE_ROM_SEG, 0x3090u), mouse.handler_off);
            tc_st_u16(TC_MKFP(MACHINE_ROM_SEG, 0x3092u), mouse.handler_seg);
        }
        memset(&r, 0, sizeof r);
        machine_call_handler(MACHINE_ROM_SEG, 0x3000u, &r);
        return;
    }
    default:
        tc_fail(70, "an event of kind %u", e->kind);
    }
}

int machine_deliver_due(void)
{
    int n = 0;
    if (delivering) {
        return 0;                               /* a handler's own accesses: after it returns */
    }
    delivering = 1;
    while (event_next < event_count && due(&events[event_next])) {
        machine_event e = events[event_next++];
        deliver(&e);
        n++;
    }
    delivering = 0;
    return n;
}

void machine_access(void)
{
    machine_accesses++;
    while (time_next < time_count && times[time_next].access <= machine_accesses) {
        u32 insns = times[time_next++].insns;
#ifdef TC_HYBRID
        if (tc_hybrid_set_insns(insns)) {
            continue;
        }
#endif
        machine_time.insns = insns;
    }
}

void machine_wait(void)
{
    if (delivering) {
        tc_fail(75, "a handler waits for an interrupt (the machine delivers none inside a handler)");
    }
    if (event_next < event_count && due(&events[event_next])) {
        delivering = 1;
        {
            machine_event e = events[event_next++];
            deliver(&e);
        }
        delivering = 0;
        return;
    }
    if (event_next < event_count) {
        tc_fail(75, "a routine waits for an interrupt; the next is due at C clock %lu after access %lu, the "
                "machine is at %lu after %lu", (unsigned long)events[event_next].at,
                (unsigned long)events[event_next].access, (unsigned long)machine_cclock,
                (unsigned long)machine_accesses);
    }
    tc_fail(75, "a routine waits for an interrupt and none is left to come");
}
