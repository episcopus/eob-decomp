/* Unit at EOB.EXE 1AA6:000F (54 bytes, 1 function): format an int in decimal into a local buffer
   and hand it, with four more ints, to the text routine behind the far function pointer at
   ds:80CA. The map guessed assembly, but the frame, the direct argument pushes and the cdecl
   clean-up are the compiler's. Placeholder names. */

extern char *itoa(int value, char *string, int radix); /*@ 0000:62AA */
extern void (far *gDrawText)(char far *text, int a, int b, int c, int d); /*@ ds:80CA */

/*@ 1AA6:000F */ void PrintInt(int value, int a, int b, int c, int d)
{
    char buf[30];

    itoa(value, buf, 10);
    gDrawText(buf, a, b, c, d);
}
