# Review: e19ea

Reviewer: rev-sonnet
Commit reviewed: 2332e052b44a1e318efd752db3758d27f54a8c9f

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] should src/E19EA.ASM:2: the header says "every register it uses saved", but `push di` / `pop di` (19EA:0004, 001E) saves a register the body never touches (only AX, DX, DS, SI are used; checked on the bytes at 19EA:0000..0021). The extra push is harmless to the match; say "saves AX, DI, DS, DX, SI (DI is saved but not used)" or similar. The direction flag is not saved or cleared either: `lodsb` at 19EA:000E relies on DF being clear on entry (the Borland convention), worth a word.
- [ ] note The name `_PrintString` and the description "print a NUL-terminated far string" are supported: the frame reads a far pointer at [bp+6] (offset) and [bp+8] (segment) and loops `lodsb`/`int 21h` with AH=06h until a NUL byte (19EA:000E..0019). The callers push `ds` and then a DGROUP offset (pattern `push ds; mov ax,off; push ax; lcall 19EA:0000`), which matches that layout. The name stays a placeholder; "print" is true of the effect, the helper is not the C `puts`.
- [ ] note Evidence for the strings it is given (DGROUP is 212E, WHITEPAPER 2.5, so file offset = 0x2800 + 0x212E0 + offset): the call sites at 07C8 and 19BC push ds:30F0 "\r\nNot enough memory to run program." (in `main`, 07C8:00B8), ds:311F "\r\nMemory chain corrupt!\r\n", ds:3139 "\r\nCould not load overlay \"", ds:3184 "\r\nAllocation failure, returning to DOS!", ds:3093 "\r\nUnable to load font ", ds:3067 "Font8.fnt" and ds:7FEC "Memory Allocation Error! Press any key to exit to DOS." (the last from 19BC:02B6). So the helper is the game's console error printer, and all messages carry their own CR LF because DOS 06h does no translation. The WHITEPAPER 2.6 sentence about "Not enough memory" is right but names only one of about eight call sites; this file does not need to list them.
- [ ] note DOS 06h treats DL=FFh as an input request, so a message holding a 0FFh byte would not print it. No string seen above has one; behaviour is the original's, not to be fixed.

## Summary

The unit is one 34 byte far function and is understandable from its source alone. The name `_PrintString` is a placeholder that the call sites support (all string arguments are console messages); the single "should" is a slightly overstated register-save comment (DI is saved but unused, DF assumed clear). No must-fix.
