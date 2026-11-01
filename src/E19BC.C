/* Unit at EOB.EXE 19BC:0007 (728 bytes, 5 functions): the game's far memory manager, a thin
   layer over the Turbo C++ far heap. MemAlloc calls farmalloc for the size plus a one or 16 byte
   header, normalizes the returned pointer and stores a flags byte just before the block;
   MemFree calls farfree; f19BC_018A calls realloc and rewrites the header; LargestFreeBlock
   returns the largest free block (farcoreleft and farheapwalk, less one 16 byte block);
   OutOfMemory prints "Memory Allocation Error! Press any key to exit to DOS." and gives up.
   Placeholder names. */

/*@flags -ml -O -Z -G */

#include <alloc.h>
#include <mem.h>
#include <dos.h>

extern unsigned g810E; /*@ ds:810E */

/* farmalloc 0000:72DE, farfree 0000:71EE and realloc 0000:743A come from the runtime's far heap
   (ALLOC.H); farcoreleft 0000:5244 and farheapwalk 0000:76C0 likewise. memset (MEM.H) is
   0000:6384 in this large model build, a far pointer fill. */

extern int f1DDD_002D(int i); /*@ 1DDD:002D */
extern void PrintString(char far *s); /*@ 19EA:0000 */
extern void f1AAD_053A(void); /*@ 1AAD:053A */
extern void f07C8_058A(void); /*@ 07C8:058A */

/*@ 19BC:0007 stub */ void far *MemAlloc(unsigned long size, int flags) { (void)size; (void)flags; return 0; }

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

/*@ 19BC:018A stub */ void f19BC_018A(void) {}

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
