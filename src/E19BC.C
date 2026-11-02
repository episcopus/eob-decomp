/* Unit at EOB.EXE 19BC:0007 (728 bytes, 5 functions): the game's far memory manager, a thin
   layer over the Turbo C++ far heap. MemAlloc calls farmalloc for the size plus a one or 16 byte
   header, normalizes the returned pointer and stores a flags byte just before the block;
   MemFree calls farfree; MemRealloc calls realloc and rewrites the header; LargestFreeBlock
   returns the largest free block (farcoreleft and farheapwalk, less one 16 byte block);
   OutOfMemory prints "Memory Allocation Error! Press any key to exit to DOS." and gives up.
   Compiled with -G (optimise for speed): every call's arguments are cleaned up with `add sp,n`
   (19BC:003C, 004E, 0085, 00EB, 0126, 017F, 01DF, 021D, 0288) or the two byte `inc sp; inc sp`
   (19BC:02C2), where the -O -Z units use `pop`. */

/*@flags -ml -O -Z -G */

#include <alloc.h>
#include <mem.h>
#include <dos.h>

/* ds:810E is the net allocation counter gAllocCount: MemAlloc adds one for each successful
   allocation (19BC:0129) and MemFree subtracts one on every call, including a NULL pointer
   (19BC:0182), so it is the number of successful MemAlloc calls minus the number of MemFree
   calls and can fall below the number of live blocks. What reads the count is unknown;
   src/E07A9.C:108 declares it and its StartupAlloc clears it at src/E07A9.C:242 (07A9:00E6). */
extern unsigned gAllocCount; /*@ ds:810E */
/* The image stores 19BC:02B6 (OutOfMemory) at ds:810A, so before main that is what the hook
   points at. main (07C8:0001) overwrites it with AllocationFailure (07C8:0625) before any
   allocation (src/E07C8.C:132), so at run time the hook is AllocationFailure, not OutOfMemory. */
extern int (far *gAllocFailHook)(void); /*@ ds:810A */

/* farmalloc 0000:72DE, farfree 0000:71EE and realloc 0000:743A come from the runtime's far heap
   (ALLOC.H); farcoreleft 0000:5244 and farheapwalk 0000:76C0 likewise. memset (MEM.H) is
   0000:6384 in this large model build, a far pointer fill. */

extern void far *f1BA8_0072(void far *p); /*@ 1BA8:0072: normalizes a far pointer to DX:AX (its
                                              body calls _NormFar, src/E1BA8.ASM:81-90); the name
                                              stays a placeholder */
extern int EvictFile(unsigned size, unsigned char flags); /*@ 1C0E:0E5B: first retry before the
                                                              OOM hook (src/E1C0E.C:631) */
extern void f1DDD_002D(int mode); /*@ 1DDD:002D: INT 10h AH=0 set video mode (src/E1DDD.C:43) */
extern void PrintString(char far *s); /*@ 19EA:0000 */
extern int WaitGameKey(void); /*@ 1AAD:053A: returns the value _ProcessKeyEvent leaves in AX
                                 (src/E1AAD.ASM:481-483); src/E1C0E.C:46 and src/E082C.C:78
                                 declare int and use it, so the type fits the value */
extern void ShutdownGame(void); /*@ 07C8:058A */

/* The two flag bits MemAlloc stores in a block's header. MEMALLOC_ALIGNED (0x20) returns a
   paragraph aligned block; src/E07A9.C:35 defines this bit and uses it at src/E07A9.C:232, and
   src/E1B82.C:43 defines it and uses it at src/E1B82.C:165 and :174.
   MEMALLOC_ZERO_FILL (0x10) zero fills the block; no caller in src/ passes it (the 0x10L at
   src/E07C8.C:259 is a size, not a flag), so its meaning is read from this unit's code alone. */
#define MEMALLOC_ALIGNED 0x20
#define MEMALLOC_ZERO_FILL 0x10
/* the maxFlags MemAlloc's retry passes to EvictFile: 0xff is the largest unsigned char, and
   FindEvictableFile keeps a slot whose flags are `<= maxFlags` (src/E1C0E.C:617), so any slot
   qualifies. The other caller passes the slot's own flags instead (src/E1C0E.C:875). */
#define EVICT_ANY 0xff
/* the zero-fill chunk: MemAlloc fills a MEMALLOC_ZERO_FILL block in chunks this size and steps the
   segment by the same count in paragraphs (0xFA00 / 16 = 0xFA0, 19BC:00E3). memset's size_t count
   is 16-bit in this large model build (build/toolchain/tc100/INCLUDE/MEM.H:32, :53), so a block
   over 64K must be split; why the original stops at 0xFA00 rather than 0xFFFF is not evidenced. */
#define MEMALLOC_FILL_CHUNK 0xFA00L

/* 19BC:0007: allocate a far block of `size` bytes. It asks farmalloc for one extra byte (or 16
   for a MEMALLOC_ALIGNED request) and retries: first the retry EvictFile(0, EVICT_ANY) is tried,
   then the failure hook at ds:810A (AllocationFailure once main has installed it); only when both
   return 0 does it give up with NULL. It normalizes the pointer, then writes the header: for
   MEMALLOC_ALIGNED the block starts at offset 0 of a paragraph and its header sits at offset 15
   of the paragraph before it (see MemFree); otherwise the header is the byte just before the
   block. For MEMALLOC_ZERO_FILL it zero fills the whole block in MEMALLOC_FILL_CHUNK byte chunks.
   ds:810E is incremented (the net allocation counter described at its declaration). The two
   stage retry is the nested loop the original compiles to: the inner loop retries farmalloc while
   EvictFile(0, EVICT_ANY) succeeds, and the outer loop runs the failure hook when the inner retry
   gives up. */
/*@ 19BC:0007 */ void far *MemAlloc(unsigned long size, int flags)
{
    unsigned char far *blk;
    unsigned char far *cur;
    unsigned char pad;
    unsigned long left;

    left = size;
    size += (flags & MEMALLOC_ALIGNED) ? 0x10 : 1;
    for (;;) {
        while ((blk = (unsigned char far *)farmalloc(size)) == NULL) {
            if (EvictFile(0, EVICT_ANY))
                continue;
            break;
        }
        if (blk == NULL && gAllocFailHook())
            continue;
        if (blk != NULL)
            break;
        return NULL;
    }
    blk = (unsigned char far *)f1BA8_0072(blk);
    if (flags & MEMALLOC_ALIGNED) {
        pad = 0x10 - (unsigned char)FP_OFF(blk);
        (blk - 1)[pad] = (pad - 1) | flags;
        blk = (unsigned char far *)MK_FP(FP_SEG(blk) + 1, 0);
    } else {
        *blk = flags;
        blk++;
    }
    if (flags & MEMALLOC_ZERO_FILL) {
        cur = blk;
        while (left > MEMALLOC_FILL_CHUNK) {
            memset(cur, 0, MEMALLOC_FILL_CHUNK);
            left -= MEMALLOC_FILL_CHUNK;
            cur = MK_FP(FP_SEG(cur) + 0xFA0, FP_OFF(cur));
        }
        memset(cur, 0, left);
    }
    gAllocCount++;
    return blk;
}

/* 19BC:0137: release a block from MemAlloc. The header byte (at segment minus one, offset plus
   15) holds the flags MemAlloc stored. A block whose flags have MEMALLOC_ALIGNED set was placed
   at offset 0 of a paragraph with its header in the paragraph before, so it is freed at
   MK_FP(that segment, 15 minus the header's low nibble); every other block is freed at its own
   address minus one. The net allocation counter ds:810E falls either way, even when the pointer
   is NULL. */
/*@ 19BC:0137 */ void MemFree(void far *block)
{
    unsigned char far *hdr;
    unsigned char flags;

    if (block != NULL) {
        hdr = (unsigned char far *)MK_FP(FP_SEG(block) - 1, FP_OFF(block) + 15);
        flags = *hdr;
        if (flags & MEMALLOC_ALIGNED)
            farfree((void far *)MK_FP(FP_SEG(hdr), 15 - (flags & 0xf)));
        else
            farfree((void far *)MK_FP(FP_SEG(block), FP_OFF(block) - 1));
    }
    gAllocCount--;
}

/* 19BC:018A: resize a block from MemAlloc, the counterpart of MemFree. It reads the header byte
   (at segment minus one, offset plus 15): `flagBits` is the masked flags byte (flags & 0xf0; the
   mask drops the low nibble, which no caller in src/ sets) and `hdrLow` is the low nibble. For a
   MEMALLOC_ALIGNED block it reallocs at the block recovered from hdrLow, then reuses hdrLow for
   the new alignment byte, stores a fresh header just before the new block and returns it at
   offset 0 of the next segment; otherwise it reallocs at the header address, restores the flags
   byte and returns the block one byte in. QUIRK 19BC:018A: the realloc result is not tested here
   or at its only caller in src/ (1C00:0055, src/E1C00.ASM:221-225). The caller's `or ax,dx; je`
   does not catch a failed aligned realloc: the 0x20 path returns MK_FP(FP_SEG(blk) + 1, 0), so a
   NULL realloc comes back as 0001:0000 and the caller calls 0001:AX. On that path the header
   store `(blk - 1)[hdrLow]` also runs through the NULL result first and lands near 0000:000F
   after the offset wraps [H], to be confirmed in the oracle. The non-aligned path (`*blk =
   flagBits`) dereferences the NULL at once; no caller in src/ uses it (MemRealloc's only caller
   is 1C00:0055, an aligned block). */
/*@ 19BC:018A */ void far *MemRealloc(void far *ptr, unsigned size)
{
    unsigned char far *blk;
    unsigned char hdrLow;
    unsigned char flagBits;

    blk = (unsigned char far *)MK_FP(FP_SEG(ptr) - 1, FP_OFF(ptr) + 15);
    hdrLow = *blk & 0xf;
    flagBits = *blk & 0xf0;
    size += (flagBits & MEMALLOC_ALIGNED) ? 0x10 : 1;
    if (flagBits & MEMALLOC_ALIGNED) {
        blk = realloc((void far *)MK_FP(FP_SEG(blk), 15 - hdrLow), size);
        hdrLow = 0x10 - (unsigned char)FP_OFF(blk);
        (blk - 1)[hdrLow] = (hdrLow - 1) | flagBits;
        blk = MK_FP(FP_SEG(blk) + 1, 0);
    } else {
        blk = realloc(blk, size);
        *blk = flagBits;
        blk++;
    }
    return blk;
}

/* 19BC:023D: the largest free block the far heap can hand out, in bytes, or 0 when there is no
   usable block. It starts from farcoreleft and then walks the heap (farheapwalk returns _HEAPOK,
   which is 2, while a block is reported, build/toolchain/tc100/INCLUDE/ALLOC.H:13) keeping the
   largest block not in use, subtracts one 16 byte paragraph and clamps a non positive result to
   0. The result is `long`, the type src/E07C8.C:69 and src/E1C0E.C:34 declare (src/E082C.C:65
   still says unsigned long). The
   callers (for example 1BB1:0002, which adds the 16 back, src/E1BB1.ASM:34 and 168) compare the
   result with the size they need plus 16. */
/*@ 19BC:023D */ long LargestFreeBlock(void)
{
    long max;
    struct farheapinfo info;

    max = farcoreleft();
    info.ptr = NULL;
    while (farheapwalk(&info) == _HEAPOK) {
        if (info.in_use == 0 && info.size > max)
            max = info.size;
    }
    max -= 0x10;
    if (max > 0)
        return max;
    return 0;
}

/* 19BC:02B6: the default allocation failure hook. It is the far pointer the image stores at
   ds:810A (re/globals.toml:12: the `lcall [0x810a]` at 19BC:0063 and the pointer stored at DGROUP
   810A); MemAlloc lcalls it only after farmalloc fails and the EvictFile(0, EVICT_ANY) retry
   returns 0 (E19BC.C:68-74, the test at :73). main (07C8:0001) replaces it with AllocationFailure
   (07C8:0625) before any allocation (src/E07C8.C:132), so this function only runs if something
   allocates before main installs the hook. It restores the command line's video
   mode: f1DDD_002D(7) takes the index 7 branch, which loads ds:84A0 (the mode from argv[5],
   src/E07C8.C:124), not the literal 7 (src/E1DDD.C:45-48). It then prints "Memory Allocation
   Error! Press any key to exit to DOS.", waits for a key (WaitGameKey, which does no cleanup) and
   runs the shut-down path ShutdownGame (07C8:058A), which ends in exit(0). The `return 0` only
   meets the hook's int type; it is never reached. */
/*@ 19BC:02B6 */ int OutOfMemory(void)
{
    f1DDD_002D(7);
    PrintString("Memory Allocation Error! Press any key to exit to DOS.");
    WaitGameKey();
    ShutdownGame();
    return 0;
}
