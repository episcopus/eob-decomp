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
extern unsigned char far gDrvReg; /*@ 0000:3B36 */        /* a far object in another segment */
```

**Data outside DGROUP** (`mov ax,<relocated segment>; mov es,ax; ... es:[off]`, for example a byte kept in an assembly unit's code segment): declare it as a far object with its segment and offset, `extern unsigned char far gDrvReg; /*@ 0000:3B36 */` (in assembly `EXTRN _gDrvReg:BYTE ; @ 0000:3B36`), and use it by name. `MK_FP` with a constant segment compiles to `xor ax,ax` and cannot match (escalation e177d, 2026-11-01).

Siblings must be in the file (as stubs) and in address order: a call to a function of the same file compiles to `push cs; call near`, to an extern to a far call. Callers elsewhere reach an overlay function through its stub entry (`2106:002A` for entry 2 of the stub at `2106`, entries at +20h, 5 bytes each).

**Assembly units** are `src/<ID>.ASM`, assembled by TASM 2.0 with `/mx`, `.MODEL LARGE` (the code segment is word aligned): `PUBLIC _Name ; @ 19EA:0000`, `EXTRN _Callee:FAR ; @ ...`. **Far calls to another unit** (`9A off seg`): declare the callee at module level, after `.MODEL LARGE` and before `.CODE`, outside any segment (`EXTRN _MemAlloc:FAR ; @ 19BC:0007`), and write the call `call far ptr _MemAlloc`. A plain `call _MemAlloc` assembles as `push cs; call near`, and an EXTRN inside a segment of its own makes TLINK report a fixup overflow and leave the segment word 0000 (escalation e1c00, 2026-11-01; `src/E19FC.ASM` and `src/E1C00.ASM` show the form). Write forward jumps as plain `jmp label`: TASM is one pass and pads a forward `jmp` that turns out short with a NOP (`EB xx 90`), as the original shows. Data inside the code segment is written where it is (`src/E1990.ASM`).

**A C unit with a stopgap assembly file** (eov1, 2026-11-02: the map took compiled C for assembly, and its first functions were transcribed into `src/EOV1.ASM`). Both files are the unit's and both are proven by `make match`; `src/<ID>.C` is the one to write. When a function matches in C, turn its PROC in the `.ASM` into a stub (`PUBLIC _Name ; @ addr stub`, `ret`); when the `.ASM` holds only stubs, delete it. A function never stays in assembly because it already matches there.

**Units of START1.EXE** (`s_text`, `src/S_TEXT.C`) are small model: `/*@flags -ms -O -Z */` (the skeleton writes it), externs and library functions reached by near calls, annotated `0000:XXXX`; a callee that is not a library function `rtl_compare` names is Westwood's (START1's 05F6 to 0681 are s_text's assembly module, not library) and is declared with its address like any extern. s_text is two modules, `src/S_TEXT.C` and `src/S_TEXT.ASM`. A code address is written as its label (`mov ax,offset Int2FHandler`), never as the number it assembles to. Names are `start1/...` in renames.

**Units of INTRO.EXE** (ids `i...`, files `src/I0892.C`) are written the same way, with INTRO.EXE's addresses in every annotation (its DGROUP is paragraph 0E9F, its one overlay `ovl0`); `make match` takes the program from the unit. Most of INTRO.EXE is the library code EOB.EXE links too: a unit with `twin = "<EOB unit>"` in `re/units.toml` is proven by `make twin` (the same module, addresses relabelled one to one) and never written again; its functions carry their twin's names, so a callee there is declared under that name (`extern void PrintString(char far *s); /*@ 097D:0000 */`). Names are per program: a rename of an INTRO address is written `intro/ds:1BC1`, `intro/0892:0835`. Seen so far: `src/I0921.ASM`; i0892 wants `-G`.

Examples: `src/E0CF7.C`, `src/E07A9.C` (whole units), `src/E08E6.C` (a jump table, stub siblings), `src/E1B82.C` (`-G`), `src/EOV2.C` (overlay), `src/E19EA.ASM`, `src/E1990.ASM`.

## Steps

1. Claim the unit from the main checkout: `make claim AGENT=<your name> [UNIT=<id>]` (without UNIT, the next of `work/queue.toml`). Work in the worktree it prints (`../eob-worktrees/<id>`, branch `unit/<id>`), nowhere else. Playbook 90 has the whole loop.
2. `make packet UNIT=<id>` and read `build/packets/<id>.md`: the unit's functions with their callers and callees (overlay calls resolved through their stub entries), the globals and callees `src/` already names (reuse those names and annotations), and the disassembly of every function not at L2 with call targets and known globals named. `tools/re/disasm.py eob/EOB.EXE <SEG>` disassembles any range.
3. No file yet: `make skeleton UNIT=<id>` writes `src/<ID>.C` (or `.ASM`) with every function a stub. Replace stubs with code, one function at a time, in address order. Declare every callee and global with its annotation; a callee's signature comes from what the caller pushes (`push` order is right to left, `add sp,n` gives the bytes).
4. `make match FILE=src/<ID>.C`. The first difference is printed with both instructions; `tools/match/cmatch.py FILE --show NAME` prints ours and the original side by side.
5. A function matches: make it readable before committing (`docs/process/READABILITY.md`): name it, its parameters and locals from the evidence (or `Purpose unknown:` with what is known), comment it with its address, and use the names `src/` already gives the globals and callees it shares (`make packet` lists them). A global or callee that deserves a better name everywhere: `make rename OLD=<name or address> NEW=<Name> WHY="<evidence>"`. `make readability UNIT=<id>` checks the rules. Then `make match FILE=src/<ID>.C WRITE=1` records L2 in `re/symbols.toml`; commit (`<unit>: <what> (<function ids>) [L2]`).
6. Done, stuck, or out of time: every function of the file matched, a `stub`, or a `draft` with a note; commit; `make handoff AGENT=<you> UNIT=<id> STATE=complete|partial|blocked NOTE="..."` from the worktree. `make handoff` refuses a file that does not match, a file with new readability findings (`make readability UNIT=<id>`), and a branch that changes what it may not (below). A question for the human is `STATE=escalate`.

`make integrate` (playbook 90) merges the branch through `make check`; if it rejects it, `make claim` with the same AGENT gives you the worktree back with the reason.

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
| `nop; push cs; call near` (90 0E E8) in overlay code | a call to a function defined later in the same file: the compiler emits a far call and TLINK's overlay build rewrites it in place; write the call normally, `make match` accepts it (eov6, 2026-11-01). A function defined earlier gives `push cs; call near` without the nop |
| `push ss; lea ax,[bp-n]; push ax; push ds; mov ax,offset; push ax; mov cx,n; call F_SCOPY@` | a local ARRAY initialised from a list, `char a[6] = { 0, ... };` (a struct initialised or assigned gives `lea ax` before `push ss`) |
| the same, with a template that holds far pointers to functions | the same array of function pointers, `void (far *a[3])(void) = { F, G, H };`: `make match` compares the template byte by byte, each pointer by the function it names (an overlay function as its stub entry), so order and names must be right |
| `mov ax,[x]; and ax,K; or ax,0; je` where ours is `test word [x],K` | a bit test of a `long`: declare the field or variable `long` |
| identical jump tails kept separately (several `case`s ending in the same store, each with its own copy), where our build merges them into one | the unit was compiled with debug information: `/*@flags -ml -O -Z -v */` (`-y` gives the same bytes). It keeps tails apart from different source lines while TCC still sizes jumps itself. Not `-B` (TCC -S then TASM): that also keeps the tails, but it sizes jumps conservatively and TASM writes `xchg ax,bx` as one byte, so its matches were partial (2026-11-02, whitepaper 2.1.1). The shared library units (`-G`, e19bc to e1de7) have no debug information: e1de7's tails are merged |
| `test byte ptr [g],1` | `g & 1` on a char |
| a `cmp` whose result is unused, after a conditional jump | an `if` with an empty body: `if (a == 0 && p[1][1]) ;` |

## Rules

- Never change the matcher to make a function match; never use `__emit__` or inline bytes. A unit branch cannot change `tools/match/`, its negative fixtures (`tests/match/negative/`), the agent tools, the spec, the pins or `eob/`: the pre-commit hook (`make claim` installs it) and `make handoff` refuse it. A matcher problem is an escalation.
- `make match-gate` is the matcher's regression gate (part of `make check`): every file of `src/` matches, every function recorded at L2 is still proven by `src/`, and every negative fixture is rejected with a difference. A fixture is a matched unit with one deliberate defect (a constant, a global, a callee, a struct field, a string, an operator, a case label, a near call to the wrong sibling, an overlay global, missing switches, an assembly jump); a change to the matcher that lets one through fails the gate.
- Five failed attempts on one function: stop, write what you tried in its `notes` in `re/symbols.toml`, move on.
- A difference no source form produces (every construct and switch tried, the same in every instance in the game; a compiler question, not a source question): keep the function a `draft`, start its `notes` with `blocked:` and the evidence, and escalate once. Units whose only unfinished functions are blocked leave the queue until the human answers (`make watch-agents` lists them), so no session is spent on them. Before you block, look for a source form: both 2026-11-01 "compiler questions" had one (an array initialiser for the `F_SCOPY@` push order, a `long` for `or ax,0`).
