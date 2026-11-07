/* tcdos.c: the Borland library functions that need the machine (DOS and BIOS calls) or keep the
   library's own data in DGROUP: the random generator, the disk, the clock, the files, int86, harderr
   (playbook 80, M3, "The library on the machine").

   Written from the library's documented behaviour, not translated from its code (the library is
   library, AGENTS.md), and checked against the original: each definition carries the library
   routine's address in EOB.EXE, `/@ 0000:XXXX library @/`, so tc2c gives it an entry in the
   per-function test, which calls the original routine and this one on the same state and compares
   memory, the registers it returns and the devices (tools/port/hybrid.py, tests/port_asm/library.py).
   The library's data stays at its DGROUP offsets, where the state digests see it; a DOS or BIOS call
   is machine_int, so the vector table decides as the CPU's INT does. */
#include "tcport.h"
#include "tclib.h"
#include "machine.h"

#include <string.h>

/* the library's data in DGROUP, at its offsets in EOB.EXE */
#define LIB(off) TC_MKFP(tc_ds, (off))
#define RAND_SEED 0x8784u          /* rand's 32-bit state */

/* ---- STDLIB.H: the random generator ------------------------------------------------------------ */

/* a linear congruential generator over 32 bits: the state times 015A4E35h plus 1; rand returns bits
   16 to 30 of the new state */
/*@ 0000:55B7 library */ s16 tcl_rand(void)
{
    u32 s = tc_ld_u32(LIB(RAND_SEED)) * 0x015A4E35u + 1u;
    tc_st_u32(LIB(RAND_SEED), s);
    return (s16)((s >> 16) & 0x7FFFu);
}

/*@ 0000:55A6 library */ void tcl_srand(u16 seed)
{
    tc_st_u32(LIB(RAND_SEED), seed);
}

/* ---- DIR.H: the current drive ------------------------------------------------------------------ */

static void dos(tc_regs *r)
{
    machine_library_int(0x21, r);
}

/* 0 for A: */
/*@ 0000:5E11 library */ s16 tcl_getdisk(void)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    r.ax = 0x1900u;
    dos(&r);
    return (s16)(r.ax & 0xFFu);
}

/* the number of logical drives */
/*@ 0000:5E17 library */ s16 tcl_setdisk(s16 drive)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    r.ax = 0x0E00u;
    r.dx = (u16)(drive & 0xFF);
    dos(&r);
    return (s16)(r.ax & 0xFFu);
}

/* ---- TIME.H: time, through getdate, gettime and dostounix -------------------------------------- */

#define TZ_TIMEZONE 0x8818u        /* tzset's timezone (long: seconds west of GMT) */
#define TZ_DAYLIGHT 0x881Cu        /* tzset's daylight (int: the zone has a summer time) */
#define TZ_NAMES 0xFFDCu           /* tzname's two buffers, the standard and the summer name */

/* tzset: from the environment's TZ ("EST5EDT": a name, hours west, a summer name); with no TZ the
   library's default, EST5EDT. A TZ is not in the port's runtime yet: the goldens' environment has
   none. */
static void tzset_(void)
{
    u16 env = tc_ld_u16(TC_MKFP((u16)(tc_load_seg - 0x10u), 0x2Cu));
    u16 i = 0;
    while (tc_ld_u8(TC_MKFP(env, i)) != 0) {
        if (tc_ld_u8(TC_MKFP(env, i)) == 'T' && tc_ld_u8(TC_MKFP(env, (u16)(i + 1u))) == 'Z' &&
            tc_ld_u8(TC_MKFP(env, (u16)(i + 2u))) == '=') {
            tc_fail(74, "UNIMPLEMENTED in the port's runtime: tzset with TZ in the environment");
        }
        while (tc_ld_u8(TC_MKFP(env, i)) != 0) {
            i++;
        }
        i++;
    }
    tc_st_u32(LIB(TZ_TIMEZONE), 5u * 3600u);
    tc_st_u16(LIB(TZ_DAYLIGHT), 1);
    for (i = 0; i < 4; i++) {                  /* tzname[0] and tzname[1], two 4-byte buffers */
        tc_st_u8(LIB((u16)(TZ_NAMES + i)), (u8)"EST"[i]);
        tc_st_u8(LIB((u16)(TZ_NAMES + 4u + i)), (u8)"EDT"[i]);
    }
}

static int leap(u16 y) { return (y % 4u) == 0; }   /* 1970 to 2099 */

static u16 day_of_year(u16 y, u16 mon, u16 day)    /* 0 for 1 January */
{
    static const u16 before[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    return (u16)(before[mon - 1u] + day - 1u + (mon > 2u && leap(y) ? 1u : 0u));
}

/* the US summer time of the time's year [H: the library's own rule and hour not yet checked against
   the original but in June 1991, the oracle's date]: from the first Sunday of April, 2:00, to the last
   Sunday of October, 1:00 standard time */
static int is_summer(u16 y, u16 yday, u16 hour)
{
    u32 days = 0;
    u16 k, apr1, oct31, start, end;
    for (k = 1970; k < y; k++) {
        days += leap(k) ? 366u : 365u;
    }
    /* 1 January 1970 was a Thursday (weekday 4, Sunday 0) */
    apr1 = day_of_year(y, 4, 1);
    oct31 = day_of_year(y, 10, 31);
    start = (u16)(apr1 + (7u - (u16)((days + apr1 + 4u) % 7u)) % 7u);
    end = (u16)(oct31 - (u16)((days + oct31 + 4u) % 7u));
    if (yday < start || yday > end) {
        return 0;
    }
    if (yday == start) {
        return hour >= 2u;
    }
    if (yday == end) {
        return hour < 1u;
    }
    return 1;
}

/*@ 0000:5831 library */ s32 tcl_time(farptr timer)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u16 y, mon, day, hour, min, sec, yday, k;
    u32 t;
    r.ax = 0x2A00u;                            /* getdate */
    dos(&r);
    y = r.cx;
    mon = (u16)(r.dx >> 8);
    day = (u16)(r.dx & 0xFFu);
    memset(&r, 0, sizeof r);
    r.ax = 0x2C00u;                            /* gettime */
    dos(&r);
    hour = (u16)(r.cx >> 8);
    min = (u16)(r.cx & 0xFFu);
    sec = (u16)(r.dx >> 8);
    tzset_();                                  /* dostounix */
    yday = day_of_year(y, mon, day);
    t = 0;
    for (k = 1970; k < y; k++) {
        t += leap(k) ? 366u : 365u;
    }
    t = (t + yday) * 86400u + (u32)hour * 3600u + (u32)min * 60u + sec + tc_ld_u32(LIB(TZ_TIMEZONE));
    if (tc_ld_u16(LIB(TZ_DAYLIGHT)) != 0 && is_summer(y, yday, hour)) {
        t -= 3600u;
    }
    if (timer != 0) {
        tc_st_u32(timer, t);
    }
    return (s32)t;
}

/* ---- errors: errno and _doserrno -------------------------------------------------------------- */

#define ERRNO 0x007Du
#define DOSERRNO 0x879Au

/* errno for each DOS error code 0 to 58h (-1 where DOS's code has no C one), as the original sets it
   (tests/port_asm/library.py derives it again: int86 on a handler that returns each code with the carry
   set) */
static const s8 dos_to_errno[0x59] = {
    0, 19, 2, 2, 4, 5, 6, 8, 8, 8, 20, 21, 5, 19, -1, 22,                                   /* 00 */
    5, 17, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,                          /* 10 */
    5, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,                          /* 20 */
    -1, -1, 15, -1, 35, 2, -1, 15, -1, -1, -1, -1, 19, -1, -1, 2,                          /* 30 */
    2, 5, 15, 2, -1, -1, -1, 19, -1, -1, -1, -1, -1, -1, -1, -1,                           /* 40 */
    35, -1, -1, -1, -1, 35, -1, 19, -1,                                                    /* 50 */
};

/* the library's error exit: a DOS error code (or minus an errno, 35 at most) into errno and _doserrno;
   any other code is taken as 57h. Returns -1. */
static s16 ioerror(s16 code)
{
    if (code < 0) {
        if ((s16)-code <= 35) {
            tc_st_u16(LIB(ERRNO), (u16)-code);
            tc_st_u16(LIB(DOSERRNO), 0xFFFFu);
            return -1;
        }
        code = 0x57;
    } else if (code >= 0x59) {
        code = 0x57;
    }
    tc_st_u16(LIB(DOSERRNO), (u16)code);
    tc_st_u16(LIB(ERRNO), (u16)(s16)dos_to_errno[code]);
    return -1;
}

/* ---- DOS.H: int86 ------------------------------------------------------------------------------ */

/* the flags of the 8086's 8-bit `cmp a, b`, with IF set */
static u16 cmp_flags(u8 a, u8 b)
{
    u8 r = (u8)(a - b);
    u16 f = 0x0202u;
    u8 low = r;
    int ones = 0;
    while (low != 0) {
        ones += low & 1u;
        low >>= 1;
    }
    if (a < b) {
        f |= 0x0001u;                          /* CF */
    }
    if ((ones & 1) == 0) {
        f |= 0x0004u;                          /* PF */
    }
    if ((a ^ b ^ r) & 0x10u) {
        f |= 0x0010u;                          /* AF */
    }
    if (r == 0) {
        f |= 0x0040u;                          /* ZF */
    }
    if (r & 0x80u) {
        f |= 0x0080u;                          /* SF */
    }
    if ((a ^ b) & (a ^ r) & 0x80u) {
        f |= 0x0800u;                          /* OF */
    }
    return f;
}

/* REGS: ax, bx, cx, dx, si, di, cflag, flags (words). The INT executes with the flags of the library's
   test of the interrupt number against 25h and 26h, absread and abswrite, which leave a word on the
   stack: a byte compare, with 25h below 25h and with 26h above 26h (every number checked,
   tests/port_asm/library.py). IF is the caller's, on: the game never calls int86 with interrupts off.
   Those two are not in the port's runtime. */
/*@ 0000:60BD library */ s16 tcl_int86(s16 intno, farptr in, farptr out)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    r.ax = tc_ld_u16(tc_fp_add(in, 0));
    r.bx = tc_ld_u16(tc_fp_add(in, 2));
    r.cx = tc_ld_u16(tc_fp_add(in, 4));
    r.dx = tc_ld_u16(tc_fp_add(in, 6));
    r.si = tc_ld_u16(tc_fp_add(in, 8));
    r.di = tc_ld_u16(tc_fp_add(in, 10));
    r.ds = tc_ds;
    r.es = tc_ds;
    if ((u8)intno == 0x25u || (u8)intno == 0x26u) {
        tc_fail(74, "UNIMPLEMENTED in the port's runtime: int86 with INT %02Xh", (unsigned)intno);
    }
    r.flags = cmp_flags((u8)intno, (u8)intno < 0x25u ? 0x25u : 0x26u);
    /* the library's int86 runs its INT from a stub it writes on the stack (an INT n, then a RETF), so
       the oracle counts the call with the game's accesses, not the library's (tools/oracle/cclock.py:
       the INT's return address is in the stack segment) */
    machine_int((u8)intno, &r);
    tc_st_u16(tc_fp_add(out, 0), r.ax);
    tc_st_u16(tc_fp_add(out, 2), r.bx);
    tc_st_u16(tc_fp_add(out, 4), r.cx);
    tc_st_u16(tc_fp_add(out, 6), r.dx);
    tc_st_u16(tc_fp_add(out, 8), r.si);
    tc_st_u16(tc_fp_add(out, 10), r.di);
    tc_st_u16(tc_fp_add(out, 12), (u16)(r.flags & 1u));
    tc_st_u16(tc_fp_add(out, 14), r.flags);
    if (r.flags & 1u) {
        ioerror((s16)r.ax);
    }
    return (s16)r.ax;
}

/* ---- DOS.H: harderr ---------------------------------------------------------------------------- */

#define HARDERR_HANDLER 0xFFD6u    /* the handler harderr installed (far pointer) */
/* INT 24h goes to the library's own handler, 0000:6014, which calls the one installed. The port's DOS
   raises no critical error (nor does the oracle's), so nothing runs there; a vector the port follows
   to it would stop at the missing code (machine_call_handler). */
#define HARDERR_ISR 0x6014u

/*@ 0000:604C library */ void tcl_harderr(farptr handler)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    tc_st_u32(LIB(HARDERR_HANDLER), handler);
    r.ax = 0x2524u;
    r.ds = tc_load_seg;
    r.dx = HARDERR_ISR;
    dos(&r);
}

/* ---- IO.H: the files ---------------------------------------------------------------------------- */

/* _openfd: a flag word for each of the library's 20 handles (the open's access bits, less O_CREAT,
   O_TRUNC and O_EXCL; FFFFh when closed), then _fmode, the default of O_TEXT and O_BINARY. Only close
   checks a handle against the 20; the others index the table with any handle, as the original does. */
#define OPENFD 0x8758u
#define FMODE 0x8780u
#define NFILE 20

#define F_RDONLY 0x0001u
#define F_WRONLY 0x0002u
#define F_RDWR 0x0004u
#define F_CREAT 0x0100u
#define F_TRUNC 0x0200u
#define F_EXCL 0x0400u
#define F_APPEND 0x0800u
#define F_TEXT 0x4000u
#define F_BINARY 0x8000u
#define F_EOF 0x0200u              /* in the flag word: at the end of the file (read returns 0) */
#define F_CHANGED 0x1000u          /* in the flag word: written, created or truncated */
#define F_DEVICE 0x2000u           /* in the flag word: a character device */

static farptr openfd(s16 fd)
{
    return LIB((u16)(OPENFD + 2u * (u16)fd));
}

/* a DOS call on a path or a buffer (DS:DX); 0, or the DOS error code with the carry */
static u16 dos_ptr(tc_regs *r, farptr p)
{
    r->ds = TC_SEG(p);
    r->dx = TC_OFF(p);
    dos(r);
    return (r->flags & 1u) ? r->ax : 0;
}

static u16 dos_call(tc_regs *r)
{
    dos(r);
    return (r->flags & 1u) ? r->ax : 0;
}

static void text_mode(const char *fn)
{
    tc_fail(74, "UNIMPLEMENTED in the port's runtime: %s on a text-mode handle (the game's are binary)", fn);
}

/* open(path, access, mode): mode only with O_CREAT. With O_CREAT the file's attributes are read
   first: a file that is there is opened (refused with O_EXCL), one that is not is created, read-only
   unless the mode has S_IWRITE (80h), and nothing more is asked of it. An opened file's device bit
   comes from IOCTL 0; O_TRUNC then writes nothing at its start. */
/*@ 0000:6443 library */ s16 tcl_open(farptr path, s16 access_, const u8 *va, u16 va_size)
{
    u16 access = (u16)access_;
    u16 err, flags;
    s16 fd;
    tc_regs r;
    if (!(access & (F_TEXT | F_BINARY))) {
        access |= (u16)(tc_ld_u16(LIB(FMODE)) & (F_TEXT | F_BINARY));
    }
    if (access & F_CREAT) {
        u16 mode = va_size >= 2 ? (u16)(va[0] | (va[1] << 8)) : 0;
        memset(&r, 0, sizeof r);
        r.ax = 0x4300u;                        /* _chmod(path, 0) */
        err = dos_ptr(&r, path);
        if (err == 0) {
            if (access & F_EXCL) {
                return ioerror(0x50);          /* the file exists */
            }
        } else {
            ioerror((s16)err);                 /* errno stays set when the create succeeds */
            memset(&r, 0, sizeof r);
            r.ax = 0x3C00u;                    /* _creat */
            r.cx = (mode & 0x80u) ? 0u : 1u;
            err = dos_ptr(&r, path);
            if (err != 0) {
                return ioerror((s16)err);
            }
            fd = (s16)r.ax;
            tc_st_u16(openfd(fd), (u16)((access & (u16)~(F_CREAT | F_TRUNC | F_EXCL)) | F_CHANGED));
            return fd;
        }
    }
    memset(&r, 0, sizeof r);
    r.ax = (u16)(0x3D00u | (((access & 7u) >> 1) | (access & 0xF0u)));
    err = dos_ptr(&r, path);
    if (err != 0) {
        return ioerror((s16)err);
    }
    fd = (s16)r.ax;
    memset(&r, 0, sizeof r);
    r.ax = 0x4400u;                            /* IOCTL: the device information */
    r.bx = (u16)fd;
    flags = (u16)(access & (u16)~(F_CREAT | F_TRUNC | F_EXCL));
    if (dos_call(&r) == 0 && (r.dx & 0x80u)) {
        flags |= F_DEVICE;
    }
    if (access & F_TRUNC) {
        memset(&r, 0, sizeof r);
        r.ax = 0x4000u;                        /* write nothing: the end of the file here */
        r.bx = (u16)fd;
        dos_call(&r);
    }
    if (access & (F_CREAT | F_TRUNC)) {
        flags |= F_CHANGED;
    }
    tc_st_u16(openfd(fd), flags);
    return fd;
}

/*@ 0000:50C5 library */ s16 tcl_close(s16 fd)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u16 err;
    if ((u16)fd >= NFILE) {
        return ioerror(6);
    }
    r.ax = 0x3E00u;
    r.bx = (u16)fd;
    err = dos_call(&r);
    tc_st_u16(openfd(fd), 0xFFFFu);
    return err ? ioerror((s16)err) : 0;
}

/*@ 0000:65E1 library */ s16 tcl_read(s16 fd, farptr buf, u16 len)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u16 flags = tc_ld_u16(openfd(fd)), err;
    if (len == 0 || (flags & F_EOF)) {
        return 0;
    }
    if (!(flags & F_BINARY)) {
        text_mode("read");
    }
    r.ax = 0x3F00u;
    r.bx = (u16)fd;
    r.cx = len;
    err = dos_ptr(&r, buf);
    return err ? ioerror((s16)err) : (s16)r.ax;
}

/*@ 0000:61FF library */ s32 tcl_lseek(s16 fd, s32 offset, s16 whence)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u16 err;
    tc_st_u16(openfd(fd), (u16)(tc_ld_u16(openfd(fd)) & (u16)~F_EOF));
    r.ax = (u16)(0x4200u | ((u16)whence & 0xFFu));
    r.bx = (u16)fd;
    r.cx = (u16)((u32)offset >> 16);
    r.dx = (u16)offset;
    err = dos_call(&r);
    if (err != 0) {
        return (s32)ioerror((s16)err);
    }
    return (s32)(((u32)r.dx << 16) | r.ax);
}

/*@ 0000:5BFE library */ s16 tcl_write(s16 fd, farptr buf, u16 len)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u16 flags, err;
    if (len == 0) {
        return 0;
    }
    if (tc_ld_u16(openfd(fd)) & F_APPEND) {
        tcl_lseek(fd, 0, 2);
    }
    flags = tc_ld_u16(openfd(fd));
    if (!(flags & F_BINARY)) {
        text_mode("write");
    }
    r.ax = 0x4000u;
    r.bx = (u16)fd;
    r.cx = len;
    err = dos_ptr(&r, buf);
    if (err != 0) {
        return ioerror((s16)err);
    }
    tc_st_u16(openfd(fd), (u16)(tc_ld_u16(openfd(fd)) | F_CHANGED));
    return (s16)r.ax;
}

/* the length from the end, the position kept */
/*@ 0000:5F26 library */ s32 tcl_filelength(s16 fd)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u16 err, pos_hi, pos_lo;
    u32 len;
    r.ax = 0x4201u;
    r.bx = (u16)fd;
    err = dos_call(&r);
    if (err != 0) {
        return (s32)ioerror((s16)err);
    }
    pos_hi = r.dx;
    pos_lo = r.ax;
    memset(&r, 0, sizeof r);
    r.ax = 0x4202u;
    r.bx = (u16)fd;
    err = dos_call(&r);
    if (err != 0) {
        return (s32)ioerror((s16)err);
    }
    len = ((u32)r.dx << 16) | r.ax;
    memset(&r, 0, sizeof r);
    r.ax = 0x4200u;
    r.bx = (u16)fd;
    r.cx = pos_hi;
    r.dx = pos_lo;
    err = dos_call(&r);
    if (err != 0) {
        return (s32)ioerror((s16)err);
    }
    return (s32)len;
}

/*@ 0000:6B57 library */ s16 tcl_remove(farptr path)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u16 err;
    r.ax = 0x4100u;
    err = dos_ptr(&r, path);
    return err ? ioerror((s16)err) : 0;
}

/* ---- the library's data in DGROUP (step D3) ----------------------------------------------------- */

/* The initial values of the library variables the runtime's functions keep in DGROUP, as Turbo C++
   1.00's library has them in a program's image: _openfd's flags for the five standard handles
   (stdin O_RDONLY, stdout and stderr O_WRONLY, the three of them devices; stdaux O_RDWR and stdprn
   O_WRONLY, binary) and FFFFh for the other 15, _fmode O_TEXT, rand's seed 1, tzset's EST5EDT
   (18000 s west, a summer time) with tzname's two far pointers to its buffers. The port starts from
   its own image (tc2c_data writes the program's data), so the runtime writes these;
   tools/port/image.py compares them with EOB.EXE's. The rest of the library's data, and the start-up
   code's, are not the runtime's: no function of the port reads them. */
void tc_library_data(void)
{
    static const u16 std_fd[5] = {0x2001u, 0x2002u, 0x2002u, 0xA004u, 0xA002u};
    u16 i;
    for (i = 0; i < NFILE; i++) {
        tc_st_u16(LIB((u16)(OPENFD + 2u * i)), i < 5 ? std_fd[i] : 0xFFFFu);
    }
    tc_st_u16(LIB(FMODE), F_TEXT);
    tc_st_u32(LIB(RAND_SEED), 1u);
    tc_st_u32(LIB(TZ_TIMEZONE), 5u * 3600u);
    tc_st_u16(LIB(TZ_DAYLIGHT), 1);
    tc_st_u32(LIB(0x8810u), TC_MKFP(tc_ds, TZ_NAMES));        /* tzname[0] */
    tc_st_u32(LIB(0x8814u), TC_MKFP(tc_ds, TZ_NAMES + 4u));   /* tzname[1] */
}

/* ---- PROCESS.H: exit ----------------------------------------------------------------------------- */

/* exit(status): the program's end. The Borland library runs its atexit functions and its #pragma exit
   ones (the overlay manager's and the start-up code's: vectors back, files closed), then DOS 4Ch with
   the status; the game registers no atexit function, and the port has none of the library's own to
   undo, so this is the DOS call, which ends the run (the machine stops, port/runtime/dos.c). */
/*@ 0000:50F3 library */ void tcl_exit(s16 status)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    r.ax = (u16)(0x4C00u | (u8)status);
    dos(&r);
}
