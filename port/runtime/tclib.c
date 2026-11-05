/* tclib.c: the Borland library functions of tclib.h, with Turbo C++ 1.00's behaviour on the arena.

   Written from the library's documented behaviour, not translated from its code (the library is
   library, AGENTS.md). Where a detail of the result is Turbo C's own and not yet checked against the
   original it is marked [H]; M2's per-function test (playbook 80) runs each of these beside the
   original's on the states the goldens reach. */
#include <stdio.h>
#include <stdlib.h>

#include "tcport.h"
#include "tclib.h"

static u8 rd(farptr p, u16 i) { return tc_ld_u8(tc_fp_add(p, i)); }
static void wr(farptr p, u16 i, u8 v) { tc_st_u8(tc_fp_add(p, i), v); }

/* ---- STRING.H ---------------------------------------------------------------------------------- */

u16 tcl_strlen(farptr s)
{
    u16 n = 0;
    while (rd(s, n) != 0) {
        n++;
    }
    return n;
}

farptr tcl_strcpy(farptr dst, farptr src)
{
    u16 i = 0;
    u8 c;
    do {
        c = rd(src, i);
        wr(dst, i, c);
        i++;
    } while (c != 0);
    return dst;
}

farptr tcl_strcat(farptr dst, farptr src)
{
    tcl_strcpy(tc_fp_add(dst, tcl_strlen(dst)), src);
    return dst;
}

/* the difference of the first bytes that differ, as unsigned chars [H: the value, not its sign] */
s16 tcl_strcmp(farptr a, farptr b)
{
    u16 i = 0;
    for (;;) {
        u8 x = rd(a, i), y = rd(b, i);
        if (x != y || x == 0) {
            return (s16)((s16)x - (s16)y);
        }
        i++;
    }
}

/* the first c (as a char) in s, the terminating NUL included; null when there is none */
farptr tcl_strchr(farptr s, s16 c)
{
    u16 i = 0;
    for (;;) {
        u8 x = rd(s, i);
        if (x == (u8)c) {
            return tc_fp_add(s, i);
        }
        if (x == 0) {
            return 0;
        }
        i++;
    }
}

farptr tcl_strupr(farptr s)
{
    u16 i;
    u8 c;
    for (i = 0; (c = rd(s, i)) != 0; i++) {
        if (c >= 'a' && c <= 'z') {
            wr(s, i, (u8)(c - 32));
        }
    }
    return s;
}

farptr tcl_strlwr(farptr s)
{
    u16 i;
    u8 c;
    for (i = 0; (c = rd(s, i)) != 0; i++) {
        if (c >= 'A' && c <= 'Z') {
            wr(s, i, (u8)(c + 32));
        }
    }
    return s;
}

/* ---- MEM.H --------------------------------------------------------------------------------------- */

farptr tcl_memset(farptr s, s16 c, u16 n)
{
    u16 i;
    for (i = 0; i < n; i++) {
        wr(s, i, (u8)c);
    }
    return s;
}

/* overlapping blocks copy as if through a buffer: backwards when the destination is above the
   source in the same segment [H: Turbo C compares the offsets only] */
farptr tcl_memmove(farptr dst, farptr src, u16 n)
{
    u16 i;
    if (TC_SEG(dst) == TC_SEG(src) && TC_OFF(dst) > TC_OFF(src)) {
        for (i = n; i > 0; i--) {
            wr(dst, (u16)(i - 1), rd(src, (u16)(i - 1)));
        }
    } else {
        for (i = 0; i < n; i++) {
            wr(dst, i, rd(src, i));
        }
    }
    return dst;
}

farptr tcl_memchr(farptr s, s16 c, u16 n)
{
    u16 i;
    for (i = 0; i < n; i++) {
        if (rd(s, i) == (u8)c) {
            return tc_fp_add(s, i);
        }
    }
    return 0;
}

/* movmem(src, dst, n): memmove with the operands the other way round */
void tcl_movmem(farptr src, farptr dst, u16 n)
{
    tcl_memmove(dst, src, n);
}

/* ---- STDLIB.H, CTYPE.H --------------------------------------------------------------------------- */

static int is_space(u8 c) { return c == ' ' || (c >= 9 && c <= 13); }

/* leading white space, an optional sign, decimal digits; the value wraps in 32 bits [H] */
s32 tcl_atol(farptr s)
{
    u16 i = 0;
    u32 v = 0;
    int neg = 0;
    while (is_space(rd(s, i))) {
        i++;
    }
    if (rd(s, i) == '-' || rd(s, i) == '+') {
        neg = rd(s, i) == '-';
        i++;
    }
    while (rd(s, i) >= '0' && rd(s, i) <= '9') {
        v = v * 10u + (u32)(rd(s, i) - '0');
        i++;
    }
    return (s32)(neg ? 0u - v : v);
}

/* the digits of v in radix 2 to 36, lower-case letters; a minus sign for a negative value only in
   radix 10, as Turbo C's itoa and ltoa [H] */
static farptr to_text(u32 v, int neg, farptr buf, s16 radix)
{
    char tmp[34];
    u16 n = 0, o = 0;
    if (radix < 2 || radix > 36) {
        wr(buf, 0, 0);
        return buf;
    }
    if (neg) {
        v = 0u - v;
        wr(buf, o++, '-');
    }
    do {
        u32 d = v % (u32)radix;
        tmp[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= (u32)radix;
    } while (v != 0);
    while (n > 0) {
        wr(buf, o++, (u8)tmp[--n]);
    }
    wr(buf, o, 0);
    return buf;
}

farptr tcl_itoa(s16 value, farptr buf, s16 radix)
{
    if (radix == 10) {
        return to_text((u32)(s32)value, value < 0, buf, radix);
    }
    return to_text((u16)value, 0, buf, radix);
}

farptr tcl_ltoa(s32 value, farptr buf, s16 radix)
{
    return to_text((u32)value, radix == 10 && value < 0, buf, radix);
}

s16 tcl_toupper(s16 c)
{
    return (s16)(c >= 'a' && c <= 'z' ? c - 32 : c);
}

/* ---- STDIO.H ------------------------------------------------------------------------------------- */

/* Turbo C's printf conversions without the floating point ones: flags - + space # 0, a width and a
   precision (numbers or *), the sizes h l F N, and d i u o x X c s p %. In the large model %s and %p
   take a far pointer, %Ns and %Np a near one (an offset in DGROUP). A floating point conversion stops
   the program: no format of the game has one. */
s16 tcl_sprintf(farptr buf, farptr fmt, const u8 *va, u16 va_size)
{
    u16 fi = 0, out = 0, ai = 0;
    u8 c;

#define ARG16() (ai + 2u <= va_size ? (ai += 2, tc_vr16(va + ai - 2)) : (u16)0)
#define ARG32() (ai + 4u <= va_size ? (ai += 4, tc_vr32(va + ai - 4)) : (u32)0)
#define PUT(ch) (wr(buf, out, (u8)(ch)), out++)

    while ((c = rd(fmt, fi++)) != 0) {
        int left = 0, plus = 0, space = 0, alt = 0, zero = 0, width = 0, prec = -1, size = 0;
        char digits[40];
        const char *prefix = "";
        int nd = 0, i, pad;
        u32 v;
        int neg = 0;

        if (c != '%') {
            PUT(c);
            continue;
        }
        for (;;) {
            c = rd(fmt, fi);
            if (c == '-') left = 1;
            else if (c == '+') plus = 1;
            else if (c == ' ') space = 1;
            else if (c == '#') alt = 1;
            else if (c == '0') zero = 1;
            else break;
            fi++;
        }
        if (c == '*') {
            width = (s16)ARG16();
            if (width < 0) {
                left = 1;
                width = -width;
            }
            c = rd(fmt, ++fi);
        } else {
            while (c >= '0' && c <= '9') {
                width = width * 10 + (c - '0');
                c = rd(fmt, ++fi);
            }
        }
        if (c == '.') {
            prec = 0;
            c = rd(fmt, ++fi);
            if (c == '*') {
                prec = (s16)ARG16();
                c = rd(fmt, ++fi);
            } else {
                while (c >= '0' && c <= '9') {
                    prec = prec * 10 + (c - '0');
                    c = rd(fmt, ++fi);
                }
            }
        }
        if (c == 'h' || c == 'l' || c == 'F' || c == 'N') {
            size = c;
            c = rd(fmt, ++fi);
        }
        fi++;
        switch (c) {
        case '%':
            PUT('%');
            continue;
        case 'c':
            digits[0] = (char)ARG16();
            nd = 1;
            prec = -1;
            zero = 0;
            break;
        case 's': {
            farptr s = size == 'N' ? TC_MKFP(tc_ds, ARG16()) : ARG32();
            u16 n = 0;
            while (rd(s, n) != 0 && (prec < 0 || n < (u16)prec)) {
                n++;
            }
            pad = width > n ? width - n : 0;
            if (!left) for (i = 0; i < pad; i++) PUT(' ');
            for (i = 0; i < n; i++) PUT(rd(s, (u16)i));
            if (left) for (i = 0; i < pad; i++) PUT(' ');
            continue;
        }
        case 'p': {
            u32 p = size == 'N' ? ARG16() : ARG32();
            const char *hex = "0123456789ABCDEF";
            if (size == 'N') {
                for (i = 3; i >= 0; i--) digits[nd++] = hex[(p >> (4 * i)) & 15];
            } else {
                for (i = 7; i >= 4; i--) digits[nd++] = hex[(p >> (4 * i)) & 15];
                digits[nd++] = ':';
                for (i = 3; i >= 0; i--) digits[nd++] = hex[(p >> (4 * i)) & 15];
            }
            prec = -1;
            zero = 0;
            break;
        }
        case 'd':
        case 'i':
        case 'u':
        case 'o':
        case 'x':
        case 'X': {
            int base = c == 'o' ? 8 : (c == 'x' || c == 'X') ? 16 : 10;
            const char *dig = c == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
            char tmp[34];
            int n = 0;
            if (size == 'l') {
                v = ARG32();
                if ((c == 'd' || c == 'i') && (s32)v < 0) {
                    neg = 1;
                    v = 0u - v;
                }
            } else {
                u16 w = ARG16();
                v = w;
                if ((c == 'd' || c == 'i') && (s16)w < 0) {
                    neg = 1;
                    v = (u32)(-(s32)(s16)w);
                }
            }
            while (v != 0) {
                tmp[n++] = dig[v % (u32)base];
                v /= (u32)base;
            }
            if (prec < 0 && n == 0) {
                tmp[n++] = '0';
            }
            while (n < prec) {
                tmp[n++] = '0';
            }
            if (alt && base == 8 && tmp[n - 1] != '0') {
                tmp[n++] = '0';
            }
            while (n > 0) {
                digits[nd++] = tmp[--n];
            }
            if (c == 'd' || c == 'i') {
                prefix = neg ? "-" : plus ? "+" : space ? " " : "";
            } else if (alt && base == 16) {
                prefix = c == 'X' ? "0X" : "0x";
            }
            if (prec >= 0) {
                zero = 0;
            }
            break;
        }
        default:
            fprintf(stderr, "tcport: sprintf conversion %%%c is not in the port's runtime\n", c);
            exit(70);
        }
        {
            int np = 0;
            while (prefix[np]) np++;
            pad = width > nd + np ? width - nd - np : 0;
            if (!left && !zero) for (i = 0; i < pad; i++) PUT(' ');
            for (i = 0; i < np; i++) PUT(prefix[i]);
            if (!left && zero) for (i = 0; i < pad; i++) PUT('0');
            for (i = 0; i < nd; i++) PUT(digits[i]);
            if (left) for (i = 0; i < pad; i++) PUT(' ');
        }
    }
    wr(buf, out, 0);
    return (s16)out;
#undef ARG16
#undef ARG32
#undef PUT
}

/* ---- DOS.H --------------------------------------------------------------------------------------- */

farptr tcl_getvect(s16 n)
{
    return tc_ld_u32(TC_MKFP(0, (u16)((u16)(u8)n * 4u)));
}

void tcl_setvect(s16 n, farptr isr)
{
    tc_st_u32(TC_MKFP(0, (u16)((u16)(u8)n * 4u)), isr);
}
