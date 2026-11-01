/* Unit at EOB.EXE 1B82:0009 (547 bytes, 4 functions): loading a file into a buffer and unpacking
   it by the format byte at its start. Compiled with -G (optimise for speed: `inc sp; inc sp` and
   `add sp,4` after calls), unlike the spike units. Placeholder names. */

/*@flags -ml -O -Z -G */

#include <dos.h>

extern unsigned gBufSeg; /*@ ds:812A */
extern unsigned gBufSize; /*@ ds:812C */

extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */
extern void far *f19EC_0002(int kind); /*@ 19EC:0002 */
extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void MemFree(void far *p); /*@ 19BC:0137 */
extern void Unpack0(char far *src, void far *dst, long arg); /*@ 1BA8:0006 */
extern void Unpack3(unsigned src, unsigned dst); /*@ 1D9A:0113 */
extern void Unpack1(unsigned src, unsigned dst, unsigned work); /*@ 1D9A:018B */
extern void Unpack2(unsigned src, unsigned dst, unsigned work); /*@ 1D9A:02CB */
extern void Unpack4(char far *src, void far *dst, int len); /*@ 1D9A:0013 */

void Unpack(char far *src, void far *dst);

/*@ 1B82:0009 */ void LoadFile(char far *name, void far *buf, unsigned len)
{
    int h;

    h = FileOpen(name, 1);
    FileRead(h, buf, len);
    FileClose(h);
}

/*@ 1B82:0041 */ void LoadPacked(char far *name, int srcKind, int dstKind)
{
    unsigned n;
    void far *src;
    void far *dst;
    int h;

    src = f19EC_0002(srcKind);
    dst = f19EC_0002(dstKind);
    h = FileOpen(name, 1);
    FileRead(h, (void far *)&n, 2L);
    FileRead(h, src, n);
    FileClose(h);
    Unpack(src, dst);
}

/*@ 1B82:00C7 draft */ void Unpack(char far *src, void far *dst)
{
    void far *work;
    long size;
    int n;

    switch ((long)*src) {
    case 0L:
        Unpack0(src + 8, dst, *(long far *)(src + 2));
        break;
    case 3L:
        Unpack3(FP_SEG(src), FP_SEG(dst));
        break;
    case 1L:
        if (gBufSize < 0x3C00) {
            work = MemAlloc(0x3C00L, 0x20);
            Unpack1(FP_SEG(src), FP_SEG(dst), FP_SEG(work));
            MemFree(work);
        } else
            Unpack1(FP_SEG(src), FP_SEG(dst), gBufSeg);
        break;
    case 2L:
        if (gBufSize < 0xFC00) {
            work = MemAlloc(0xFC00L, 0x20);
            Unpack2(FP_SEG(src), FP_SEG(dst), FP_SEG(work));
            MemFree(work);
        } else
            Unpack2(FP_SEG(src), FP_SEG(dst), gBufSeg);
        break;
    case 4L:
        src += 2;
        size = *(long far *)src;
        src += 4;
        n = *(int far *)src;
        src += 2;
        src += n;
        Unpack4(src, dst, size);
        break;
    }
}

/*@ 1B82:0217 */ void SetUnpackBuffer(unsigned seg, unsigned size)
{
    if (gBufSeg = seg)
        ;
    if (gBufSize = size)
        ;
}
