/* alloc.h for tc2c (README.md): what the C units use from Turbo C++ 1.00's ALLOC.H. */
#ifndef TC_ALLOC_H
#define TC_ALLOC_H
#ifndef NULL
#define NULL 0L
#endif
#define _HEAPEMPTY 1
#define _HEAPOK 2
#define _FREEENTRY 3
#define _USEDENTRY 4
#define _HEAPEND 5
#define _HEAPCORRUPT -1
#define _BADNODE -2
#define _BADVALUE -3
struct farheapinfo { void huge *ptr; unsigned long size; int in_use; };
void far *farmalloc(unsigned long nbytes);
void farfree(void far *block);
unsigned long farcoreleft(void);
int farheapwalk(struct farheapinfo *hi);
void *realloc(void *block, unsigned size);
#endif
