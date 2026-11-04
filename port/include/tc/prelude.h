/* prelude.h: what tools/port/tc2c.py puts in front of a src/ unit before clang reads it.

   clang parses the unit as C for the MSP430 target, whose int is 16 bits and long 32, as Turbo
   C++'s, with signed char (no -K) and structs packed on bytes (no -a). Every data pointer of the
   large model is far, so the far, near and huge keywords carry nothing the translator needs: it
   gives every pointer four bytes itself. cdecl, pascal and interrupt are dropped the same way;
   tc2c refuses a unit that defines an interrupt function until the port handles them (playbook 80,
   open question 6). */
#pragma pack(1)
#define far
#define near
#define huge
#define cdecl
#define pascal
#define interrupt
