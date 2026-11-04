/* prelude.h: what tools/port/tc2c.py puts in front of a src/ unit before clang reads it.

   clang parses the unit as C for the MSP430 target, whose int is 16 bits and long 32, as Turbo
   C++'s, with signed char (no -K) and structs packed on bytes (no -a). Every data pointer of the
   large model is far, so the far, near and huge keywords carry nothing the translator needs: it
   gives every pointer four bytes itself. cdecl, pascal and interrupt are dropped the same way.
   No C unit defines an interrupt function (the handlers are assembly); the units that name the
   keyword use it in vector types (`void interrupt (far *)()`, E07C8.C, I0892.C), which are far code
   pointers like any other. tc2c still refuses those units until the machine layer has the
   interrupt vector table (playbook 80, "Open questions"). */
#pragma pack(1)
#define far
#define near
#define huge
#define cdecl
#define pascal
#define interrupt
