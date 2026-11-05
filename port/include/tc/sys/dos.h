/* dos.h for tc2c (README.md): what the C units use from Turbo C++ 1.00's DOS.H. */
#ifndef TC_DOS_H
#define TC_DOS_H
#ifndef NULL
#define NULL 0L
#endif
struct WORDREGS { unsigned int ax, bx, cx, dx, si, di, cflag, flags; };
struct BYTEREGS { unsigned char al, ah, bl, bh, cl, ch, dl, dh; };
union REGS { struct WORDREGS x; struct BYTEREGS h; };
struct SREGS { unsigned int es, cs, ss, ds; };
int int86(int intno, union REGS *inregs, union REGS *outregs);
int int86x(int intno, union REGS *inregs, union REGS *outregs, struct SREGS *segregs);
void segread(struct SREGS *segp);
void interrupt (far *getvect(int interruptno))();
void setvect(int interruptno, void interrupt (far *isr)());
void far *MK_FP(unsigned seg, unsigned ofs);
unsigned FP_SEG(void far *p);
unsigned FP_OFF(void far *p);
#endif
