# 30 Decompile a C module to L2

**Inputs:** a unit of kind `c` in `re/units.toml`, its functions in `re/symbols.toml`.

## The source file

One file per unit: `src/<ID>.C` (upper case, `E0CF7.C` for unit `e0cf7`), functions in address order, starting with a comment that gives the unit's address, size and what it does. Every function and every global it uses carries its original address in a comment on the line that declares it:

```c
extern char gFlag; /*@ ds:F3B9 */                                     /* a global: DGROUP offset */
extern void far *ListAdd(void far *list, void far *item); /*@ 1658:001E */  /* a callee */
/*@ 0CF7:0007 */ void ListInit(struct Node far *head)                 /* a function of this unit */
```

`src/E0CF7.C` and `src/E07A9.C` are the first two (the go/no-go spike, SPEC 4.2).

## Steps

1. Disassemble the unit: `.venv/bin/python tools/re/disasm.py eob/EOB.EXE <SEG>` (far calls marked `reloc`, DGROUP strings decoded).
2. Write the C, routine by routine, in address order. Declare every callee and global with its annotation; a callee's signature comes from what the caller pushes (`push` order is right to left, `add sp,n` gives the bytes).
3. `make match FILE=src/<ID>.C`. The first difference is printed with both instructions; `-v` on `tools/match/cmatch.py` prints the whole comparison.
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
| `test byte ptr [g],1` | `g & 1` on a char |
| a `cmp` whose result is unused, after a conditional jump | an `if` with an empty body: `if (a == 0 && p[1][1]) ;` |

## Rules

- Never change the matcher to make a function match; never use `__emit__` or inline bytes.
- Five failed attempts on one function: stop, write what you tried in its `notes` in `re/symbols.toml`, move on.
