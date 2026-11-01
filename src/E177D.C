/* Unit at EOB.EXE 177D:000A (646 bytes, 4 functions): load a sound driver (adlib.dat or
   pcgamesn.dat) into a 120-byte buffer and a 13000-byte block that sits in the near code
   segment, fix up the block's word table, then play driver commands by poking that table.
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

/*@ 177D:0070 stub */ void f177D_0070(int i) {}
/*@ 177D:00F3 stub */ void f177D_00F3(int a) {}
/*@ 177D:0106 stub */ void f177D_0106(int a, int b) {}
