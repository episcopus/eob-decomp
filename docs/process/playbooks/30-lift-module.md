# 30 Decompile a unit to L2

**Inputs:** a unit in `re/units.toml` (kind `c`, `asm` or `unknown`: the kind is a guess until it matches), its functions in `re/symbols.toml`.

## The source file

One file per unit: `src/<ID>.C` (upper case, `E0CF7.C` for unit `e0cf7`), functions in address order, starting with a comment that gives the unit's address, size and what it does. Every function and every global it uses carries its original address in a comment on the line that declares it:

```c
extern char gFlag; /*@ ds:F3B9 */                                     /* a global: DGROUP offset */
extern void far *ListAdd(void far *list, void far *item); /*@ 1658:001E */  /* a callee */
/*@ 0CF7:0007 */ void ListInit(struct Node far *head)                 /* a function of this unit */
```

More annotations:

```c
/*@flags -ml -O -Z -G */                                  /* the file's switches (default: cflags) */
/*@ 08E6:00C3 stub */ void Obj00C3(struct Obj28 far *o) {}  /* a sibling not written yet: called near
                                                              by this file's functions, not compared */
/*@ 0ACD:091C draft */ void Turn091C(int how)             /* written, not matching yet: compared and
                                                              reported, never a failure, never L2 */
/*@ ovl2:0110 */ void LoadItemTypes(void)                 /* a function of overlay 2 */
```

Siblings must be in the file (as stubs) and in address order: a call to a function of the same file compiles to `push cs; call near`, to an extern to a far call. Callers elsewhere reach an overlay function through its stub entry (`2106:002A` for entry 2 of the stub at `2106`, entries at +20h, 5 bytes each).

**Assembly units** are `src/<ID>.ASM`, assembled by TASM 2.0 with `/mx`, `.MODEL LARGE` (the code segment is word aligned): `PUBLIC _Name ; @ 19EA:0000`, `EXTRN _Callee:FAR ; @ ...`. Write forward jumps as plain `jmp label`: TASM is one pass and pads a forward `jmp` that turns out short with a NOP (`EB xx 90`), as the original shows. Data inside the code segment is written where it is (`src/E1990.ASM`).

Examples: `src/E0CF7.C`, `src/E07A9.C` (whole units), `src/E08E6.C` (a jump table, stub siblings), `src/E1B82.C` (`-G`), `src/EOV2.C` (overlay), `src/E19EA.ASM`, `src/E1990.ASM`.

## Steps

1. Disassemble the unit: `.venv/bin/python tools/re/disasm.py eob/EOB.EXE <SEG>` (far calls marked `reloc`, DGROUP strings decoded).
2. Write the C, routine by routine, in address order. Declare every callee and global with its annotation; a callee's signature comes from what the caller pushes (`push` order is right to left, `add sp,n` gives the bytes).
3. `make match FILE=src/<ID>.C`. The first difference is printed with both instructions; `tools/match/cmatch.py FILE --show NAME` prints ours and the original side by side.
4. When everything matches, `make match FILE=src/<ID>.C WRITE=1` records L2 in `re/symbols.toml`.

## Idioms found so far (Turbo C++ 1.00, `-ml -O -Z`; whitepaper 2.1.1)

| The code shows | The source was |
|---|---|
| `push cs; call near` to a far function | a call to a function defined earlier in the same file |
| a value kept in SI across a loop | a local `int` (register variables are automatic) |
| `mov si,1; jmp test; body; inc si; test: cmp si,N; jl body` | `for (i = 1; i < N; i++)` |
| one store to `es:[bx]`, then `es:[bx+2]`, with one `les bx` | two member stores through a far pointer global (`-Z` keeps ES:BX) |
| `xor ax,ax; push ax` reused for the next argument | `-Z`: the same constant is not reloaded |
| two branches ending in a jump to one shared call | `if (c) f(a); else f(b);` (`-O` merges the tails), not `f(c ? a : b)` |
| `cmp x,0; je L; jmp far_end` | `if (!x) { long body }` |
| `mov [g],ax; or ax,ax` and no jump | `if (g = x) ;` (an empty body; a macro compiled out?) |
| one `les bx` for two tests that both `continue` | one `if (a || b) continue;` (two `if`s put a label between them and reload) |
| case bodies of a jump table in some order | the `case` labels in that order in the source |
| `inc sp; inc sp` or `add sp,4` after calls, `pop cx` nowhere | the unit was compiled with `-G`: `/*@flags -ml -O -Z -G */` |
| `test byte ptr [g],1` | `g & 1` on a char |
| a `cmp` whose result is unused, after a conditional jump | an `if` with an empty body: `if (a == 0 && p[1][1]) ;` |

## Rules

- Never change the matcher to make a function match; never use `__emit__` or inline bytes.
- Five failed attempts on one function: stop, write what you tried in its `notes` in `re/symbols.toml`, move on.
