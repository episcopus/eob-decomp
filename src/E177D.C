/* Unit at EOB.EXE 177D:000A (646 bytes, 4 functions): load a sound driver (adlib.dat or
   pcgamesn.dat) into a 120-byte buffer and a 13000-byte block that sits in the near code
   segment, fix up the block's word table, then play driver commands by poking that table.
   177D:0106 reads and writes the driver register byte at 0000:3B36 (the code-segment block);
   its source is written as a draft: see re/symbols.toml e177d_0106 for what blocks it.
   Placeholder names. */

#include <dos.h>

extern unsigned char far *gBufDCAC; /*@ ds:DCAC */
extern int g80E6; /*@ ds:80E6 */
extern unsigned char gB892; /*@ ds:B892 */
extern unsigned char g9E; /*@ ds:009E */
extern unsigned char g8A67; /*@ ds:8A67 */
extern int g5E22; /*@ ds:5E22 */
extern int g5E24; /*@ ds:5E24 */
extern int g5E26; /*@ ds:5E26 */
extern int g5E28; /*@ ds:5E28 */
extern int g5E2A; /*@ ds:5E2A */
extern int gFBCA; /*@ ds:FBCA */

extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileRead(int handle, void far *buf, unsigned long len); /*@ 1C0E:0400 */
extern void FileClose(int handle); /*@ 1C0E:03BB */
extern void far *f0_03AA(void); /*@ 0000:03AA */
extern void f0_03B0(int idx, int off, int val); /*@ 0000:03B0 */
extern int f0_03CA(int idx, int off); /*@ 0000:03CA */
extern void far *f0_03E3(int idx); /*@ 0000:03E3 */
extern void f0_03FA(void); /*@ 0000:03FA */
extern void f0_400C(int c); /*@ 0000:400C */
extern void f1948_0418(void far *p); /*@ 1948:0418 */
extern void f1948_0458(void); /*@ 1948:0458 */
extern int f17A6_0F61(int x, int y); /*@ 17A6:0F61 */

/*@ 177D:000A */ void f177D_000A(void)
{
    int h;

    if (gB892 == 0)
        h = FileOpen("adlib.dat", 1);
    else
        h = FileOpen("pcgamesn.dat", 1);
    FileRead(h, gBufDCAC, 120L);
    FileRead(h, f0_03AA(), 0x32C8L);
    FileClose(h);
    f0_03FA();
}

/*@ 177D:0070 */ void f177D_0070(int i)
{
    int c;
    void far *p;

    if (!g80E6)
        return;
    c = gBufDCAC[i];
    if (c < 0x96) {
        if (gB892 == 0) {
            f0_400C(c);
            return;
        }
        if (g9E == 1) {
            if (c < 0 && g8A67) {
                g8A67 = 0;
                f1948_0458();
            }
            if (c > 0 && g8A67 == 0) {
                g8A67 = 1;
                f1948_0458();
            }
        }
        c &= 0x7f;
        p = f0_03E3(c);
        f1948_0418(p);
    }
}
void f177D_0106(int a, int b);

/*@ 177D:00F3 */ void f177D_00F3(int a)
{
    f177D_0106(a, gFBCA);
}
/*@ 177D:0106 draft */ void f177D_0106(int a, int b)
{
    int n;
    void far *p;

    if (!g80E6)
        return;
    if (g5E22 > 0) {
        if (gB892 == 0) {
            if (g5E24 < 0x10) {
                while (*(unsigned char far *)MK_FP(FP_SEG(f0_03AA), 0x3b36))
                    ;
                f0_03B0(g5E26, 3, g5E28);
                f0_03B0(g5E26, 1, g5E2A);
                g5E26 = g5E22;
                g5E28 = f0_03CA(g5E22, 3);
                g5E2A = f0_03CA(g5E22, 1);
                n = g5E28 = f0_03CA(g5E22, 3);
                if (g5E24 > 0) {
                    n = g5E28 + ((0x3f - g5E28) * (g5E24 - 1) >> 4);
                    f0_03B0(g5E22, 3, n);
                    f0_03B0(g5E22, 1, 0x20 - g5E24);
                }
                f0_400C(g5E22);
                *(unsigned char far *)MK_FP(FP_SEG(f0_03AA), 0x3b36) = 0xb;
            }
        } else {
            if (g9E == 1) {
                if (g5E22 < 0 && g8A67) {
                    g8A67 = 0;
                    f1948_0458();
                }
                if (g5E22 > 0 && g8A67 == 0) {
                    g8A67 = 1;
                    f1948_0458();
                }
            }
            g5E22 &= 0x7f;
            p = f0_03E3(g5E22);
            f1948_0418(p);
        }
    }
    g5E24 = b ? f17A6_0F61(gFBCA, b) : 0;
    g5E22 = gBufDCAC[a];
}
