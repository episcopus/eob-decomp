/* Unit at EOB.EXE 1A47:0005 (447 bytes, 3 functions): loading a bitmap font file. The first
   function opens a file, validates its header (file length == 0x104 + 128 * height * ceil(width/8),
   height at +0x102, width in bits at +0x103), allocates and returns the whole file; the second
   remembers the font pointer and its two geometry bytes in globals; the third scales a string
   length by the font width. Placeholder names. */

/*@flags -ml -O -Z -G */

#include <stdio.h>
#include <fcntl.h>

extern void far *gFont1A47; /*@ ds:FE4E */
extern unsigned char gFontWidth1A47; /*@ ds:8100 */
extern unsigned char gFontHeight1A47; /*@ ds:8101 */

extern int open(char far *path, int access); /*@ 0000:6443 */
extern int read(int fd, void far *buf, unsigned len); /*@ 0000:65E1 */
extern long filelength(int fd); /*@ 0000:5F26 */
extern long lseek(int fd, long offset, int whence); /*@ 0000:61FF */
extern int close(int fd); /*@ 0000:50C5 */
extern unsigned strlen(char far *s); /*@ 0000:67F6 */
extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void MemFree(void far *p); /*@ 19BC:0137 */

/*@ 1A47:0005 */ void far *f1A47_0005(char far *name)
{
    int height;
    int width;
    unsigned w;
    unsigned size;
    long pos;
    void far *buf;
    char two[2];
    int h;
    int n;

    buf = NULL;
    h = open(name, O_RDONLY | O_BINARY);
    if (h == -1)
        return buf;
    n = read(h, &w, 2);
    w += 2;
    if (n != 2)
        goto done;
    if (w <= 0x104)
        goto done;
    if (filelength(h) != w)
        goto done;
    pos = lseek(h, 0x102, SEEK_SET);
    if (pos != 0x102)
        goto done;
    n = read(h, two, 2);
    if (n != 2)
        goto done;
    height = two[0];
    width = (two[1] + 7) / 8;
    size = height * width << 7;
    if (size + 0x104 != w)
        goto done;
    size += 0x104;
    buf = MemAlloc(size, 0);
    if (buf == NULL)
        goto done;
    pos = lseek(h, 0, SEEK_SET);
    if (pos != 0)
        goto discard;
    n = read(h, buf, w);
    if (n == w)
        goto done;
    MemFree(buf);            /* the original has this block twice; -O merges the two copies */
    buf = NULL;
    goto done;
discard:
    MemFree(buf);
    buf = NULL;
done:
    close(h);
    return buf;
}

/*@ 1A47:0169 */ void f1A47_0169(char far *font)
{
    gFont1A47 = font;
    gFontWidth1A47 = font[0x103];
    gFontHeight1A47 = font[0x102];
}

/*@ 1A47:018E */ int f1A47_018E(char far *s)
{
    char far *p;
    int n;

    p = gFont1A47;
    n = p[0x103];
    n = n * strlen(s);
    return n;
}
