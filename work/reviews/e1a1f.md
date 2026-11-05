# Review: e1a1f

Reviewer: p6-rev-2
Commit reviewed: 651ac776e76787e5c5cde14f8d51e2931475c4f0 (P6 round; the earlier round read a5be789)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Round P6 (2026-11-04), commit 651ac776e76787e5c5cde14f8d51e2931475c4f0

Judged the diff a5be789..651ac77 on src/E1A1F.C: P6 changed no name in this unit. The header
dropped the sentence "Names and types are placeholders until the callers name them" and the
DrawBox comment was added; DrawBox and gDrawLine keep their names.

The names are backed: gDrawLine (ds:8062) is called with (x1, y1, x2, y2, colour) by the
callers 0D1C:0454 and 16A5:07C3, and DrawBox draws four axis-aligned lines through it after
clamping each corner to 0..319 and 0..199. The new comment matches the code. No new name needs
evidence; no name claims more than the bytes show.

- [ ] should (carried) src/E1A1F.C:1-4: the header now cites the two callers for the argument
  order, but not the other users of ds:8062 (1547:00BD, 00DC, 00F0; 1C0E:0BCE) that support the
  name gDrawLine. The earlier round's suggestion still applies if a fuller header is wanted.
- [ ] should (carried) src/E1A1F.C:9: the name DrawBox is still the ambiguous one the earlier
  round noted; the new comment says "outline", so the name no longer misleads, but
  DrawRectOutline would be clearer.
- [ ] note The new DrawBox comment (1A1F:000C) is accurate: four edges through gDrawLine after
  the clamps.

No open must-fix. The unit passes.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] should src/E1A1F.C:1-4: the header says names and types are placeholders "until the callers name them", but the evidence for `gDrawLine` and the argument order is already strong and could be stated. `make match FILE=src/E1A1F.C` is 1 of 1. In the original the four pushes of the first edge (1A1F:0063..0070) are colour, y1, x2, y1, x1, i.e. cdecl (x1, y1, x2, y1, colour). The other three edges are (x1,y2,x2,y2), (x1,y1,x1,y2) and (x2,y1,x2,y2), so every call is a horizontal or vertical segment. Outside this unit, `lcall [8062h]` also appears in the code of 1547 (1547:00BD, 00DC, 00F0 ...), 1C0E (1C0E:0BCE ...), always with five pushed words and axis aligned endpoints (1547:00B4..00BD draws (si, di) to (si+3Eh, di)). Citing one or two of these in the header would justify the name `gDrawLine` instead of calling it a placeholder.
- [ ] should src/E1A1F.C:9: the name `DrawBox` is ambiguous. The function draws an outline of four lines, never fills (no fill call, only ds:8062 line calls). A reader may take "box" to mean a filled rectangle. `DrawRectOutline` or `DrawFrame` would say what the bytes do; the header sentence "draw its four edges" is right. Not a must-fix: the name does not claim more than the evidence if read as "box outline".
- [ ] note The clamp ranges are correct for what the constants say: 0x13F = 319 and 0xC7 = 199 (1A1F:0028, 0034, 004E, 005A), inclusive limits of a 320x200 surface. The comparisons are signed (`jge`, `jle`), so `int` is right; coordinates are clamped independently, there is no swap if x1 > x2 or y1 > y2 (the matched code has none), and the function does not check that a pixel plotter honours the same clip.
- [ ] note Callers (checked on our bytes) confirm (x1, y1, x2, y2, colour). 0D1C:0454 reads an 8 byte record table at ds:5482..5488 (offsets +0, +2, +4, +6 pushed in the order y2, x2, y1, x1 after the colour, so x1,y1,x2,y2) with colour 0Ch (call at 0D1C:04AA). 16A5:07C3 (the caller is the function starting there; the call is at 16A5:09E5) passes x=di, y=si, x2 = x + w - 1 (`es:[bx+22h]`, dec) and y2 = y + h - 1 (`es:[bx+24h]`, dec) with colour from a byte (`mov ah,0`). So x2/y2 are inclusive corner coordinates, not width and height. The file does not say so; one phrase would help.
- [ ] note `colour` is `int`. The 16A5 caller widens an unsigned byte (`mov ah,0` at 16A5:09D0) and 0D1C passes the constant 0Ch, so the value range is 0..255, but the pushes are whole words and the callee passes it through untouched; `int` is the faithful type for the far function pointer's parameter, no change needed.
- [ ] note Packet quirk, not a source problem: `make packet UNIT=e1a1f` lists "ds:8062 x1 (E1A1F.C)" under "Globals src/ already names". The global is `gDrawLine`; the name `x1` is a parameter name inside the function pointer type that the packet tool's global scan picked up. A small bug in tools/agent/packet.py for a human to look at (not escalated, it does not affect the match).
- [ ] note ds:8062 is not declared in any other file of `src/` (grep for 8062 finds only this file), so there is no name clash. ds:80CA is a different pointer (gDrawText in E1AA6.C, gFn80CA in E1BA4.C), and ds:80AE is yet another (1C0E:0BB8, six pushes).
- [ ] note The segment 1A1F starts with a 12 byte stub before the unit (1A1F:0000: `mov ax,0; jmp +3; mov ax,1; pop dx; pop bx; pop bp; retf`, a bool returning epilogue, probably the tail of the previous unit's data or an RTL helper). It is outside this unit's start 1A1F:000C and does not need a function here, but the map has no unit for it; whoever owns the map may want to check it.

## Summary

One 163 byte far function, understandable from its source. Its behaviour (clamp both corners to 0..319 and 0..199, then draw top, bottom, left, right through the line plotter at ds:8062) matches the disassembly, both callers and the other users of the pointer. No claim goes beyond the evidence, so there is no must-fix. The suggestions: state the evidence for `gDrawLine` in the header instead of calling it a placeholder, note that x2/y2 are inclusive corners, and consider a name that says "outline". The packet tool's mislabelled global (`x1`) is a tooling note.
