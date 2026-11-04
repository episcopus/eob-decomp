/* Unit at EOB.EXE 1BA4:000C (57 bytes, 1 function): format a long argument as a decimal string
   with ltoa into a 30 byte local buffer, then hand the buffer and four more word arguments to a
   far function pointer kept at ds:80CA (installed at run time). The only caller, 1547:0991, draws
   screen text. Placeholder names. */

extern char *ltoa(long value, char *string, int radix); /*@ 0000:62F1 */
extern void (far *gDrawText)(); /*@ ds:80CA */

/*@ 1BA4:000C */ void PrintLong(long value, int a, int b, int c, int d)
{
    char buf[30];

    ltoa(value, buf, 10);
    gDrawText(buf, a, b, c, d);
}
