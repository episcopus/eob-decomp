/* tcport.h: the memory model of the portable build (playbook 80, "The arena").

   The translated units (tools/port/tc2c.py writes them from the C units of src/) keep every value where and as
   the original keeps it: a 1 MB arena is the real-mode address space, EOB.EXE's load module sits
   in it at 1000:0000 as DOS (and the oracle, tools/oracle/machine.py LOAD_SEG) puts it, globals
   are read and written at their DGROUP offsets, and a pointer is a far pointer: segment in the high
   word, offset in the low word, as Turbo C++ large model stores it. So a record, a table of far
   pointers or a saved game has the original's bytes, and the arena can be compared with the
   oracle's memory directly.

   Only portable C99 here, and the exact-width types below; the generated code uses nothing else
   from the host but these helpers. */
#ifndef TCPORT_H
#define TCPORT_H

#include <stdint.h>

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;   /* the runtime's, never the generated code's: file positions, clock products */
typedef int64_t s64;

/* A Turbo C far pointer: (segment << 16) | offset. Null is 0000:0000. Code addresses (a far
   function pointer, an interrupt vector) have the same form. */
typedef u32 farptr;

#define TC_MKFP(seg, off) ((farptr)((((u32)(u16)(seg)) << 16) | (u32)(u16)(off)))
#define TC_SEG(p) ((u16)((u32)(p) >> 16))
#define TC_OFF(p) ((u16)(p))

/* The 8086 wraps a linear address at 1 MB. [H] for the oracle: Unicorn maps 0 to A0000 and the
   ROM area; nothing seen so far addresses above FFFF:000F. */
#define TC_MEM_SIZE 0x100000u
#define TC_LINEAR(p) (((((u32)TC_SEG(p)) << 4) + (u32)TC_OFF(p)) & 0xFFFFFu)

extern u8 tc_mem[TC_MEM_SIZE];

/* Segments of the loaded program: the load segment (the PSP is the 10h paragraphs before it) and
   DGROUP, the segment every `ds:` annotation of src/ is relative to. */
extern u16 tc_load_seg;
extern u16 tc_ds;

/* Far pointer arithmetic changes the offset only, wrapping at 64 KB (Turbo C far pointers, not
   huge ones). */
static inline farptr tc_fp_add(farptr p, u16 delta)
{
    return TC_MKFP(TC_SEG(p), (u16)(TC_OFF(p) + delta));
}

/* Loads and stores. A word is little-endian at any alignment; its second byte is at the next linear
   address, so a word at xxxx:FFFF has its high byte at (xxxx + 1000h):0000, as the oracle does
   (Unicorn, tested 2026-11-05: `mov ax,[0FFFFh]` and `movsw` from offset FFFFh both carry into the
   next 64 KB; an 8086 would wrap to xxxx:0000, a 286 or later would fault). The goldens are the
   oracle's, so the arena follows it. Stores return the value stored, so an assignment stays an
   expression. */
#define TC_NEXT(lin) (((lin) + 1u) & 0xFFFFFu)
static inline u8 tc_ld_u8(farptr p) { return tc_mem[TC_LINEAR(p)]; }
static inline s8 tc_ld_s8(farptr p) { return (s8)tc_mem[TC_LINEAR(p)]; }
static inline u16 tc_ld_u16(farptr p)
{
    u32 a = TC_LINEAR(p);
    return (u16)(tc_mem[a] | ((u16)tc_mem[TC_NEXT(a)] << 8));
}
static inline s16 tc_ld_s16(farptr p) { return (s16)tc_ld_u16(p); }
/* a long or a far pointer is two words, as the compiled code moves it (`mov ax,[bx]`, `mov dx,[bx+2]`:
   the second offset wraps within the segment) */
static inline u32 tc_ld_u32(farptr p)
{
    return (u32)tc_ld_u16(p) | ((u32)tc_ld_u16(tc_fp_add(p, 2)) << 16);
}
static inline s32 tc_ld_s32(farptr p) { return (s32)tc_ld_u32(p); }

static inline u8 tc_st_u8(farptr p, u8 v) { tc_mem[TC_LINEAR(p)] = v; return v; }
static inline s8 tc_st_s8(farptr p, s8 v) { tc_mem[TC_LINEAR(p)] = (u8)v; return v; }
static inline u16 tc_st_u16(farptr p, u16 v)
{
    u32 a = TC_LINEAR(p);
    tc_mem[a] = (u8)v;
    tc_mem[TC_NEXT(a)] = (u8)(v >> 8);
    return v;
}
static inline s16 tc_st_s16(farptr p, s16 v) { tc_st_u16(p, (u16)v); return v; }
static inline u32 tc_st_u32(farptr p, u32 v)
{
    tc_st_u16(p, (u16)v);
    tc_st_u16(tc_fp_add(p, 2), (u16)(v >> 16));
    return v;
}
static inline s32 tc_st_s32(farptr p, s32 v) { tc_st_u32(p, (u32)v); return v; }

/* Bytes with the arena's wrap: a copy or a fill moves both offsets, each wrapping at 64 KB, one byte
   at a time from the first (an overlapping copy behaves as Turbo C's forward byte loops do).
   tc_copy returns dst: a struct assignment's value is its destination. */
farptr tc_copy(farptr dst, farptr src, u16 n);
void tc_zero(farptr dst, u16 n);
/* host bytes into the arena, with its wrap: a definition's initial value (step D3, tc2c_data) */
void tc_put(farptr dst, const u8 *src, u16 n);

/* The arena stack. A function of src/ whose locals need memory (an array, a struct, a local whose
   address is taken) gets a frame of that many bytes below tc_ss:tc_sp, as Turbo C gives it one on
   SS; the frame goes when the function returns (tc2c declares it with the cleanup attribute). The
   frames' contents and places are not the original's (its stack also holds return addresses and
   saved registers), so comparisons with the oracle mask the stack (playbook 80). */
extern u16 tc_ss;
extern u16 tc_sp;
typedef struct {
    farptr base;
    u16 sp;
} tc_frame;
tc_frame tc_frame_enter(u16 size);
static inline void tc_frame_leave(tc_frame *f) { tc_sp = f->sp; }

/* Argument blocks, little-endian as the 8086 stack holds them: a variadic library call's arguments
   after the fixed ones (tc2c lays them out, the runtime reads them), and a call whose caller and
   callee declare the arguments differently (tc2c, call_args). */
static inline void tc_va16(u8 *b, u16 v) { b[0] = (u8)v; b[1] = (u8)(v >> 8); }
static inline void tc_va32(u8 *b, u32 v) { tc_va16(b, (u16)v); tc_va16(b + 2, (u16)(v >> 16)); }
static inline u16 tc_vr16(const u8 *b) { return (u16)(b[0] | (b[1] << 8)); }
static inline u32 tc_vr32(const u8 *b) { return (u32)tc_vr16(b) | ((u32)tc_vr16(b + 2) << 16); }

/* Code addresses. Every function of src/ is registered under its original far address (the
   `@ SSSS:OOOO` annotation, its segment moved by tc_load_seg), so a far function pointer read
   from the arena (a callback field, a driver slot, an interrupt vector) finds its C function. */
typedef void (*tc_fn)(void);
void tc_code_register(farptr addr, tc_fn fn, const char *name);
/* The C function at addr, or null. A call through a far pointer with no C behind it (code the
   original loads, such as a driver) goes to tc_code_far with its arguments as Turbo C pushes them:
   the port stops there; the hybrid build runs the original's code (hybrid.h). */
tc_fn tc_code_find(farptr addr);
u32 tc_code_far(farptr addr, const u8 *args, u16 n, const char *site);

/* A function of the original that the port has no C for yet: reports it and stops. */
void tc_unported(const char *name, const char *addr);

/* Every stop of the runtime goes through tc_fail: the message on stderr and exit(code). A host that
   must not exit (the per-function test, tools/port/hybrid.py) sets tc_fail_hook, which gets the
   message and does not return. */
extern void (*tc_fail_hook)(int code, const char *msg);
void tc_fail(int code, const char *fmt, ...) __attribute__((noreturn, format(printf, 2, 3)));

/* Loads an MZ program's load module at tc_load_seg with its relocations applied (as DOS 4Bh and
   the oracle do); returns 0, or -1 with a message on stderr. ds_para is DGROUP's paragraph in the
   load module (212Eh for EOB.EXE). */
int tc_load_program(const char *path, u16 load_seg, u16 ds_para);

#endif
