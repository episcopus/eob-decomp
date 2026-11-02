/* Unit at EOB.EXE 1B82:0009 (547 bytes, 4 functions): file loading and unpacking. LoadFile reads
   len bytes of a named file into a buffer; LoadPacked takes two slot kinds, srcKind and dstKind, and
   f19EC_0002 turns each into the far pointer (offset 0) of that slot's segment (MK_FP(gSegsFC0C[kind
   | 1], 0), a segment read from the page/buffer table, not an allocation), reads a 2-byte length then
   the packed block and calls Unpack; Unpack dispatches on the block's format byte to one of five
   unpackers (1BA8:0006 for format 0 and 1D9A:0013/0113/018B/02CB for formats 4/3/1/2), allocating a
   work buffer for formats 1 and 2 when the shared buffer at gBufSeg/gBufSize is too small;
   SetUnpackBuffer records that shared buffer. Built as C++ (the -P flag): in the tc100/tc101 trials
   only the C++ front end keeps the format byte's sign extension to long in Unpack's switch, and it
   reproduces all four functions' bytes. WHITEPAPER 4.2 (WHITEPAPER.md:151) has the packed-file header
   and the formats; WHITEPAPER 2.1.1, line 41, has the -P front end that keeps the sign extension at
   1B82:00C7. */

/*@flags -ml -O -Z -G -P */

#include <dos.h>

/* The work buffer of formats 1 and 2, kept as two words so its segment can be read from memory:
   the original pushes the far pointer's second word (1B82:013F/0197 `push [bp-2]`), where the
   C++ front end would fold `FP_SEG(work)` to `push dx`. MemAlloc with flag 0x20 returns
   MK_FP(seg + 1, 0), an offset-0 block (the `inc ax; mov [bp-4],0` at 19BC:00B3), so the segment
   word alone names the block. */
typedef union {
    void far *block;    /* the block as MemAlloc returns it, passed on to MemFree */
    unsigned words[2];  /* words[1] is the block's segment (what Unpack1/Unpack2 receive) */
} WorkBuffer;

extern "C" {

extern unsigned gBufSeg; /*@ ds:812A */
extern unsigned gBufSize; /*@ ds:812C */

extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */
extern void far *f19EC_0002(int kind); /*@ 19EC:0002 */
extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void MemFree(void far *block); /*@ 19BC:0137 */
extern void Unpack0(char far *src, void far *dst, long len); /*@ 1BA8:0006 */
extern void Unpack3(unsigned srcSeg, unsigned dstSeg); /*@ 1D9A:0113 */
extern void Unpack1(unsigned srcSeg, unsigned dstSeg, unsigned workSeg); /*@ 1D9A:018B */
extern void Unpack2(unsigned srcSeg, unsigned dstSeg, unsigned workSeg); /*@ 1D9A:02CB */
extern void Unpack4(char far *src, void far *dst, int len); /*@ 1D9A:0013 */

void Unpack(char far *src, void far *dst);

/* 1B82:0009: reads len bytes of the file called name into buf: FileOpen in mode 1, FileRead, FileClose.
   QUIRK 1B82:0009: the FileOpen result is not checked. The handle is kept in SI (1B82:001F) and goes
   straight to FileRead with no test. One return path of FileOpen (1C0E:0121, reached in mode 1 through
   1C0E:0127 and 1C0E:010C) sets ax to 0xFFFF (-1). The other return paths of 1C0E:00DB are not traced
   here, and the result is not tested before FileRead, so a failed open would read through the handle
   it returned. */
/*@ 1B82:0009 */ void LoadFile(char far *name, void far *buf, unsigned len)
{
    int handle;

    handle = FileOpen(name, 1);
    FileRead(handle, buf, len);
    FileClose(handle);
}

/* 1B82:0041: loads a packed file: takes two slot kinds, srcKind and dstKind; f19EC_0002 turns each
   into the far pointer (offset 0) of that slot's segment. Reads a 2-byte length then that many bytes
   into src, closes the file, and unpacks src into dst. QUIRK 1B82:0041: as in LoadFile, the FileOpen
   result is not checked before FileRead. */
/*@ 1B82:0041 */ void LoadPacked(char far *name, int srcKind, int dstKind)
{
    unsigned packedLen;
    void far *src;
    void far *dst;
    int handle;

    src = f19EC_0002(srcKind);
    dst = f19EC_0002(dstKind);
    handle = FileOpen(name, 1);
    FileRead(handle, (void far *)&packedLen, 2L);   /* the packed block's byte length */
    FileRead(handle, src, packedLen);
    FileClose(handle);
    Unpack((char far *)src, dst);
}

/* 1B82:00C7: unpacks the block src into dst by its first byte, the format:
     0: Unpack0 gets the data at block offset +8 and the 32-bit unpacked size at block offset +2 as
        its length, and copies that many bytes;
     3: Unpack3 receives the source and destination segments;
     1 and 2: Unpack1/Unpack2 get a 0x3C00/0xFC00 byte work buffer, the shared one at gBufSeg when
              gBufSize is large enough, otherwise a MemAlloc'd block freed after the call;
     4: Unpack reads the long unpacked size at block offset +2 and passes it on as Unpack4's length
        (the decoder uses only its low word, 1D9A:003B/0043), skips the u16 byte count at block offset
        +6 (WHITEPAPER 4.2 (WHITEPAPER.md:151) calls it palette bytes [H]; no pinned file shows a
        non-zero value) and calls Unpack4 with the data at block offset +8+skipLen.
   The original sign-extends the format byte (cbw; cwd) and switches on the int in AX. The C++ front
   end (-P) emits those bytes; the C front end folds the extension away (re/symbols.toml note on
   1B82:00C7). The case bodies follow the source order 0, 3, 1, 2, 4 (1B82:00E7, 010D, 011E, 0176,
   01CC), and the table at cs:020D is indexed by format value.
   QUIRK 1B82:00C7: the switch has no default, so a format byte outside 0 to 4 leaves dst untouched.
   The bytes test the format with `cmp bx, 4` at 1B82:00D8 and an unsigned jump, so a format above 4,
   including the negative values 0x80 to 0xFF, skips the switch. */
/*@ 1B82:00C7 */ void Unpack(char far *src, void far *dst)
{
    WorkBuffer work;
    long size;
    unsigned skipLen;

    switch ((int)(long)*src) {
    case 0L:
        Unpack0(src + 8, dst, *(long far *)(src + 2));
        break;
    case 3L:
        Unpack3(FP_SEG(src), FP_SEG(dst));
        break;
    case 1L:
        if (gBufSize < 0x3C00) {
            work.block = MemAlloc(0x3C00L, 0x20);
            Unpack1(FP_SEG(src), FP_SEG(dst), work.words[1]);
            MemFree(work.block);
        } else
            Unpack1(FP_SEG(src), FP_SEG(dst), gBufSeg);
        break;
    case 2L:
        if (gBufSize < 0xFC00) {
            work.block = MemAlloc(0xFC00L, 0x20);
            Unpack2(FP_SEG(src), FP_SEG(dst), work.words[1]);
            MemFree(work.block);
        } else
            Unpack2(FP_SEG(src), FP_SEG(dst), gBufSeg);
        break;
    case 4L:
        src += 2;                   /* block offset +2: skip the format byte and the pad byte */
        size = *(long far *)src;    /* block offset +2: the 32-bit unpacked size */
        src += 4;
        /* block offset +6: a u16 byte count that Unpack skips; 0 in all 131 format-4 entries of the
           PAK files. WHITEPAPER 4.2 (WHITEPAPER.md:151) calls it palette bytes [H]: no pinned file
           shows a non-zero value. */
        skipLen = *(unsigned far *)src;
        src += 2;
        src += skipLen;
        Unpack4(src, dst, (int)size); /* Unpack4 uses size's low word (1D9A:003B) */
        break;
    }
}

/* 1B82:0217: records the shared work buffer's segment and size. Both are plain stores tested and
   discarded (the empty if bodies are what the bytes show, 1B82:0217-022C). */
/*@ 1B82:0217 */ void SetUnpackBuffer(unsigned seg, unsigned size)
{
    if (gBufSeg = seg)
        ;
    if (gBufSize = size)
        ;
}

} /* extern "C" */
