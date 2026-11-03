/* Unit at EOB.EXE 1A47:0005 (447 bytes, 3 functions): loading a bitmap font file. The first
   function opens a file, validates its header (file length == 0x104 + 128 * height * ceil(width/8),
   height at +0x102, width in bits at +0x103), allocates and returns the whole file; the second
   remembers the font pointer and its two geometry bytes in globals; the third scales a string
   length by the font width (LoadFont, SetFont, TextWidth; whitepaper 4.4 for the font files). */

/*@flags -ml -O -Z -G */

#include <stdio.h>
#include <fcntl.h>

extern void far *gCurFont; /*@ ds:FE4E */
extern unsigned char gFontWidth; /*@ ds:8100 */
extern unsigned char gFontHeight; /*@ ds:8101 */

extern int open(char far *path, int access); /*@ 0000:6443 */
extern int read(int fd, void far *buf, unsigned len); /*@ 0000:65E1 */
extern long filelength(int fd); /*@ 0000:5F26 */
extern long lseek(int fd, long offset, int whence); /*@ 0000:61FF */
extern int close(int fd); /*@ 0000:50C5 */
extern unsigned strlen(char far *s); /*@ 0000:67F6 */
extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void MemFree(void far *p); /*@ 19BC:0137 */

/* 1A47:0005: loads the font file `name` whole into memory and returns it, or a null pointer when
   the file cannot be opened or read, memory is short, or its length disagrees with its header. The
   first word is the file length less 2; the header ends at 0x104 with the height (+102h) and the
   width in pixels (+103h) of a fixed-width font of 128 characters. */
/*@ 1A47:0005 */ void far *LoadFont(char far *name)
{
    int height;
    int width;
    unsigned fileLen;
    unsigned size;
    long pos;
    void far *buf;
    char two[2];
    int handle;
    int n;

    buf = NULL;
    handle = open(name, O_RDONLY | O_BINARY);
    if (handle == -1)
        return buf;
    n = read(handle, &fileLen, 2);
    fileLen += 2;
    if (n != 2)
        goto done;
    if (fileLen <= 0x104)
        goto done;
    if (filelength(handle) != fileLen)
        goto done;
    pos = lseek(handle, 0x102, SEEK_SET);
    if (pos != 0x102)
        goto done;
    n = read(handle, two, 2);
    if (n != 2)
        goto done;
    height = two[0];
    width = (two[1] + 7) / 8;
    size = height * width << 7;
    if (size + 0x104 != fileLen)
        goto done;
    size += 0x104;
    buf = MemAlloc(size, 0);
    if (buf == NULL)
        goto done;
    pos = lseek(handle, 0, SEEK_SET);
    if (pos != 0)
        goto discard;
    n = read(handle, buf, fileLen);
    if (n == fileLen)
        goto done;
    MemFree(buf);            /* the original has this block twice; -O merges the two copies */
    buf = NULL;
    goto done;
discard:
    MemFree(buf);
    buf = NULL;
done:
    close(handle);
    return buf;
}

/* 1A47:0169: makes `font` the current font: keeps the far pointer in gCurFont and caches its
   character width (byte +103h) and height (byte +102h) in gFontWidth and gFontHeight. */
/*@ 1A47:0169 */ void SetFont(char far *font)
{
    gCurFont = font;
    gFontWidth = font[0x103];
    gFontHeight = font[0x102];
}

/* 1A47:018E: the width in pixels of string `s` in the current font: the font's character width
   (gCurFont byte +103h) times the string's length; the font is fixed-width. */
/*@ 1A47:018E */ int TextWidth(char far *text)
{
    char far *font;
    int n;

    font = gCurFont;
    n = font[0x103];
    n = n * strlen(text);
    return n;
}
