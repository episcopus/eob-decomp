/* Unit at EOB.EXE 19BC:0007 (728 bytes, 5 functions): the game's far memory manager, a thin
   layer over the Turbo C++ far heap. MemAlloc calls farmalloc for the size plus a one or 16 byte
   header, normalizes the returned pointer and stores a flags byte just before the block;
   MemFree calls farfree; MemRealloc calls realloc and rewrites the header; LargestFreeBlock
   returns the largest free block (farcoreleft and farheapwalk, less one 16 byte block);
   OutOfMemory prints "Memory Allocation Error! Press any key to exit to DOS." and gives up. */

/*@flags -ml -O -Z -G */

#include <alloc.h>
#include <mem.h>
#include <dos.h>

extern unsigned g810E; /*@ ds:810E */
extern int (far *gAllocFailHook)(void); /*@ ds:810A: holds 19BC:02B6 (OutOfMemory) */

/* farmalloc 0000:72DE, farfree 0000:71EE and realloc 0000:743A come from the runtime's far heap
   (ALLOC.H); farcoreleft 0000:5244 and farheapwalk 0000:76C0 likewise. memset (MEM.H) is
   0000:6384 in this large model build, a far pointer fill. */

extern void far *f1BA8_0072(void far *p); /*@ 1BA8:0072: normalizes a far pointer to DX:AX */
extern int f1C0E_0E5B(unsigned a, unsigned char b); /*@ 1C0E:0E5B: first retry before the OOM hook */
extern int f1DDD_002D(int i); /*@ 1DDD:002D */
extern void PrintString(char far *s); /*@ 19EA:0000 */
extern void f1AAD_053A(void); /*@ 1AAD:053A */
extern void f07C8_058A(void); /*@ 07C8:058A */

/* 19BC:0007: allocate a far block of size bytes. It asks farmalloc for one extra byte (or 16 for
   a 0x20 request) and retries: first the hook f1C0E_0E5B(0, 0xff) is tried, then the failure hook
   at ds:810A; only when both return 0 does it give up with NULL. It normalizes the pointer, stores
   the flags byte just before the block (paragraph aligned for flag 0x20) and, for flag 0x10, zero
   fills the whole block in 0xFA00 byte chunks. ds:810E counts the live blocks. The two stage
   retry is written as the nested loop the original compiles to. */
/*@ 19BC:0007 */ void far *MemAlloc(unsigned long size, int flags)
{
    unsigned char far *blk;
    unsigned char far *cur;
    unsigned char pad;
    unsigned long left;

    left = size;
    size += (flags & 0x20) ? 0x10 : 1;
    for (;;) {
        while ((blk = (unsigned char far *)farmalloc(size)) == NULL) {
            if (f1C0E_0E5B(0, 0xff))
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
    if (flags & 0x20) {
        pad = 0x10 - (unsigned char)FP_OFF(blk);
        (blk - 1)[pad] = (pad - 1) | flags;
        blk = (unsigned char far *)MK_FP(FP_SEG(blk) + 1, 0);
    } else {
        *blk = flags;
        blk++;
    }
    if (flags & 0x10) {
        cur = blk;
        while (left > 0xFA00L) {
            memset(cur, 0, 0xFA00);
            left -= 0xFA00L;
            cur = MK_FP(FP_SEG(cur) + 0xFA0, FP_OFF(cur));
        }
        memset(cur, 0, left);
    }
    g810E++;
    return blk;
}

/* 19BC:0137: release a block from MemAlloc. The byte just before the block (at segment minus one,
   offset plus 15) holds the flags MemAlloc stored. A block whose flags have 0x20 set was
   paragraph aligned with an extra offset byte, so it is freed at MK_FP(that segment, 15 minus the
   header's low nibble); every other block is freed at its own address minus one. The allocation
   counter ds:810E falls either way, even when the pointer is NULL. */
/*@ 19BC:0137 */ void MemFree(void far *block)
{
    unsigned char far *hdr;
    unsigned char flags;

    if (block != NULL) {
        hdr = (unsigned char far *)MK_FP(FP_SEG(block) - 1, FP_OFF(block) + 15);
        flags = *hdr;
        if (flags & 0x20)
            farfree((void far *)MK_FP(FP_SEG(hdr), 15 - (flags & 0xf)));
        else
            farfree((void far *)MK_FP(FP_SEG(block), FP_OFF(block) - 1));
    }
    g810E--;
}

/* 19BC:018A: resize a block from MemAlloc, the counterpart of MemFree. It reads the flags byte
   before the block (at segment minus one, offset plus 15) and, for a paragraph aligned block
   (0x20), reallocs at the block recovered from the header's low nibble, stores a fresh header
   just before the new block and returns it at offset 0 of the next segment; otherwise it
   reallocs at the header address, restores the flags byte and returns the block one byte in. */
/*@ 19BC:018A */ void far *MemRealloc(void far *ptr, unsigned size)
{
    unsigned char far *blk;
    unsigned char low;
    unsigned char high;

    blk = (unsigned char far *)MK_FP(FP_SEG(ptr) - 1, FP_OFF(ptr) + 15);
    low = *blk & 0xf;
    high = *blk & 0xf0;
    size += (high & 0x20) ? 0x10 : 1;
    if (high & 0x20) {
        blk = realloc((void far *)MK_FP(FP_SEG(blk), 15 - low), size);
        low = 0x10 - (unsigned char)FP_OFF(blk);
        (blk - 1)[low] = (low - 1) | high;
        blk = MK_FP(FP_SEG(blk) + 1, 0);
    } else {
        blk = realloc(blk, size);
        *blk = high;
        blk++;
    }
    return blk;
}

/* 19BC:023D: the largest free block the far heap can hand out, in bytes, or 0 when there is no
   usable block. It starts from farcoreleft and then walks the heap (farheapwalk returns 2 while a
   block is reported) keeping the largest block not in use, subtracts one 16 byte paragraph and
   clamps a non positive result to 0. The callers (for example 1BB1:0002) compare the result with
   the size they need plus 16. */
/*@ 19BC:023D */ unsigned long LargestFreeBlock(void)
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

/* 19BC:02B6: the allocation failure hook, installed at ds:810A (the far pointer 19BC:02B6 is
   stored there) and called by MemAlloc when farmalloc returns NULL. It restores video mode 7,
   prints "Memory Allocation Error! Press any key to exit to DOS.", runs the two cleanups and
   returns 0, which tells MemAlloc not to retry. */
/*@ 19BC:02B6 */ int OutOfMemory(void)
{
    f1DDD_002D(7);
    PrintString("Memory Allocation Error! Press any key to exit to DOS.");
    f1AAD_053A();
    f07C8_058A();
    return 0;
}
