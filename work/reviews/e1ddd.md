# Review: <unit id>

Reviewer: rev-sonnet
Commit reviewed: 780ddb57e1299c491e644bec4a67634956072240

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

Second review round. The first round (commits a3ca08c, b6bbb6b in git history) failed the unit on
the int86 argument and an "always calls" claim; both were fixed (ee48384) and I re-read them against
the bytes: they are right now (0000:60BD pushed as int86(0x10, &FF46, &FF36) at 1DDD:0019..001D and
0061..0065; ds:84A2..84AE = 4,9,0D,13,13,7,2 and ds:84B0 = 0 in the image).

- [x] must-fix src/E1DDD.C:13: "The callers pass i = 7, so that comparison reads g84A2[7]" is wrong as a general statement. The image has 8 far calls to 1DDD:002D (bytes 9a 2d 00 dd 1d at file offsets 0xa648, 0xa668, 0xa6b2, 0xa6e9, 0xa9a8, 0xaa6c, 0xaaac, 0x1c67d). Six push 7 (07C8:01C4, 0265, 0524, 05E8, 0628 and 19BC:02B9). Two push the word ds:8102 (the graphics mode argv[1][0] reduced by '1', WHITEPAPER 111): 07C8:01E4 (`push word ptr [0x8102]` then lcall 1DDD:002D) and 07C8:022E (same). Those two are the in range case that takes the mode from the ds:84A2 table, and the compare against ds:84A0 then really reads a table entry (call skipped when the video mode is already the restore mode). The comment describes only the i = 7 case and the review text "all four callers push 7" in the previous round was also incomplete. Say: callers push either 7 (6 sites, the restore case: the else branch, g84A2[7] = ds:84B0 = 0, calls only when ds:84A0 is nonzero) or ds:8102 (2 sites, 07C8:01E4 and 022E, the table case). [FIXED (this commit): the header now names all 8 sites and both cases; every site re-read in the disassembly.]
- [x] should src/E1DDD.C:11-12: "the word at ds:84A0 (argv[5] ...)". Confirmed it is a word: 07C8:0074 calls 0000:5D63 with the far string pointer argv[5] and stores only AX at 07C8:007B (`mov [84A0], ax`, DX is not stored). WHITEPAPER 115 calls it `atol`; the bytes show a word store, so 5D63 looks like atoi (an int result). The C file is right; the whitepaper line is the thing to fix (not in scope for a unit branch). Keeping the word type in the file is correct; maybe say "atoi-like, int result [H]". [FIXED: the header now says 5D63 is atoi-like with an int result (07C8:0074 passes argv[5], 07C8:007B stores only AX), not atol; the whitepaper line stays for the human.]
- [x] note f1DDD_0006 (get video mode) has no direct far call in the image (no bytes 9a 06 00 dd 1d); it may be dead code or reached some other way. Its name stays a placeholder; nothing here is wrong. [ACKNOWLEDGED, no change.]
- [x] note f1DDD_002D semantics worth recording once a name is wanted: at the table path (i < 7) it is "set video mode i unless it equals the restore mode"; at i = 7 it is "restore the mode in ds:84A0" (and skips when ds:84A0 is 0, which is the image default before argv[5] is parsed). The code is not a bug, it is how the original reads past the table. [FIXED: the header now records both paths; the name stays a placeholder.]
- [x] note `make match FILE=src/E1DDD.C` exits 0 here: 2 of 2 functions match, unit complete 106 bytes. Code reading for meaning (types, unsigned for mode words, f60BD void with a comment, int return for f1DDD_0006 returning AL masked to 8 bits) is fine. [ACKNOWLEDGED; still 2 of 2, 106 bytes after the comment fix.]

## Summary

The unit is two INT 10h video mode calls through Borland int86 and is understandable from the header, except that the header misstates which arguments reach f1DDD_002D: it says the callers pass 7, but two of the eight sites pass ds:8102 and use the table. That one comment is a must-fix, no code change needed. Names stay placeholders (no evidence of the callers' roles beyond the whitepaper), which is fine.
