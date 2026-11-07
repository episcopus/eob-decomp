/* tclib.h: the Borland library functions the port's runtime provides (port/runtime/tclib.c).

   The library is library (AGENTS.md): it is not translated from the original but written here with
   Turbo C++ 1.00's behaviour, on the arena. tools/port/tc2c.py calls a library function of a unit as
   tcl_<name>, and reads this file to know which ones exist: any other library function a unit calls
   is a stub that stops (build/port/gen/unported.c) until the runtime has it. The ones that need the
   machine (files, the heap, DOS and BIOS calls) or keep state in the library's DGROUP data (rand's
   seed, the heap, the file table) come with the machine layer (playbook 80, M3).

   A variadic function takes its variable arguments as tc2c lays them out: the bytes Turbo C pushes
   (2 for an int, 4 for a long or a far pointer), lowest address first. */
#ifndef TCLIB_H
#define TCLIB_H

#include "tcport.h"

/* STRING.H */
u16 tcl_strlen(farptr s);
farptr tcl_strcpy(farptr dst, farptr src);
farptr tcl_strcat(farptr dst, farptr src);
s16 tcl_strcmp(farptr a, farptr b);
farptr tcl_strchr(farptr s, s16 c);
farptr tcl_strupr(farptr s);
farptr tcl_strlwr(farptr s);

/* MEM.H */
farptr tcl_memset(farptr s, s16 c, u16 n);
farptr tcl_memmove(farptr dst, farptr src, u16 n);
farptr tcl_memchr(farptr s, s16 c, u16 n);
void tcl_movmem(farptr src, farptr dst, u16 n);

/* STDLIB.H, CTYPE.H */
s32 tcl_atol(farptr s);
farptr tcl_itoa(s16 value, farptr buf, s16 radix);
farptr tcl_ltoa(s32 value, farptr buf, s16 radix);
s16 tcl_toupper(s16 c);

/* port/runtime/tcdos.c: on the machine, or with the library's data in DGROUP */
void tc_library_data(void);   /* the initial values of that data (step D3) */
s16 tcl_rand(void);
void tcl_srand(u16 seed);
s16 tcl_getdisk(void);
s16 tcl_setdisk(s16 drive);
s32 tcl_time(farptr timer);
s16 tcl_int86(s16 intno, farptr in, farptr out);
void tcl_harderr(farptr handler);
s16 tcl_open(farptr path, s16 access, const u8 *va, u16 va_size);
s16 tcl_close(s16 fd);
s16 tcl_read(s16 fd, farptr buf, u16 len);
s16 tcl_write(s16 fd, farptr buf, u16 len);
s32 tcl_lseek(s16 fd, s32 offset, s16 whence);
s32 tcl_filelength(s16 fd);
s16 tcl_remove(farptr path);
/* PROCESS.H (tcdos.c): the program's end, DOS 4Ch */
void tcl_exit(s16 status);

/* port/runtime/tcheap.c: the far heap */
farptr tcl_farmalloc(u32 size);
void tcl_farfree(farptr block);
farptr tcl_realloc(farptr block, u16 size);
u32 tcl_farcoreleft(void);
s16 tcl_farheapwalk(farptr hi);

/* STDIO.H */
s16 tcl_sprintf(farptr buf, farptr fmt, const u8 *va, u16 va_size);

/* DOS.H: the vector table is the arena's 0000:0000, four bytes a vector */
farptr tcl_getvect(s16 n);
void tcl_setvect(s16 n, farptr isr);

#endif
