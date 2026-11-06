<div class="newbox" markdown="1">
<div class="newlabel">WHAT THIS DOCUMENT IS</div>

The design document of a matching decompilation of **Eye of the Beholder** (DOS, 1991), rewritten after the decompilation finished. Part I says what was done and how it is proved. Part II describes the programs, how they start and run, and the files they read. Part III explains how the game itself works, system by system, from the code that was recovered. Every claim carries a tag (**[V]** verified, **[I]** inference, **[H]** hypothesis, **[?]** open) and the address, function or tool output behind it.
</div>

<div class="tiles" markdown="1">
<div class="tile teal"><div class="num">1,011</div><div class="lbl">functions at L2: EOB.EXE 783, INTRO.EXE 213, START1.EXE 15; the compiler turns our source into the original's instructions</div></div>
<div class="tile navy"><div class="num">69</div><div class="lbl">units, 25,000 lines of C and assembly in <code>src/</code>, every function and global named from the game's own tables and strings</div></div>
<div class="tile ochre"><div class="num">7 of 7</div><div class="lbl">golden sessions replay on the original and on the rebuilt programs with the same instructions, screens, sound and state</div></div>
<div class="tile plum"><div class="num">4 of 4</div><div class="lbl">graphics drivers (VGA, CGA, EGA, Tandy) rebuild byte for byte from readable assembly</div></div>
<div class="tile grey"><div class="num">3 days</div><div class="lbl">from the first matched unit (1 November 2026) to every function at L2 (3 November), by agents under a human&rsquo;s run control</div></div>
<div class="tile red"><div class="num">2</div><div class="lbl">documented differences left in the rebuilds: the link date and the order of the overlay stub entries</div></div>
</div>

## The decisive finding

The three programs were written in **C** and built with **Borland Turbo C++ 1.00** (large model, `-O -Z`), linked by **TLINK 3.0**, with the game's overlays appended to the EXE in Borland's VROOMM format. Westwood's sound driver and the four graphics drivers are hand-written assembly assembled with **TASM 2.0**. Because the original language is C and the compiler can be found and run, the right proof is the strongest one there is: the original compiler turns our source back into the original instructions, function by function, and a tool, not a person, says so.

<!-- FIG brief_programs -->

## How it is proved

A function is **matched** when `make match` compiles our C with the original compiler and finds the original's instructions, with every address field naming the same global, function, string or segment. The matcher links each unit twice with the data and code shifted, so addresses move and constants do not; bytes that do not move must be equal. A unit is **rebuilt** when `make rebuild` links all units into a program whose load image, relocations and overlay code equal the original's. A program is **played** when the oracle, a deterministic PC emulator that runs the original and the rebuild, produces the same screens, sound writes and state on recorded sessions (the goldens).

<!-- FIG brief_ladder -->

## Six days, eight phases

The work followed the Pool of Radiance decompilation (the model for the process) and went faster, because the original language was C and its compiler could be reproduced. The first unit matched on 31 October; by 3 November every function of the three programs was at L2; the rebuild, the consistency pass (one name per address everywhere) and the first translation of the code to a portable form followed on the 4th and 5th.

<!-- FIG brief_timeline -->

## What we found about the game

The decompiled source answers questions that no documentation did. Part III gives the full account; these are the shapes that came out.

- A **real-time** game with no rounds and no initiative: one loop, `RunMapLoop`, takes a command, runs a table of 16 timed effects that drive the hands, the monsters and the spells, redraws and plays sound, all counted in ticks of the 18.2 Hz BIOS timer (sections 3.1, 3.6, 5.7).
- A **three-program chain**: `START1.EXE` asks for graphics and sound, `INTRO.EXE` plays the intro and loads `EOB.EXE` itself, with 23 of its units the same object code as `EOB.EXE`'s (section 2.6).
- **Drivers as plug-ins**: four graphics drivers loaded as overlays export 40 routines through one table; the game never touches video memory itself (section 2.5).
- A first-person view that is **not projected at all**: the 3D view looks at 18 cells and draws pre-made wall blocks at fixed depths; only monsters are scaled (section 5.12).
- **Levels as a maze file, a script file and a wall set**: the cells are four bytes, the logic is a bytecode of 26 opcodes run by one interpreter, and each level's state is saved in 2,040 bytes (sections 5.3, 5.5).
- An **AdLib driver of 20 KB** inside segment 0, with its own script interpreter and a 74-entry command table; a sound is attenuated with distance by patching its own bytes (section 5.14).
- **Quirks the original keeps**: the Ring of Sustenance never works, two saving throw tables are wrong in ways the player cannot see, and one script test in four compares a literal that can only match once (sections 5.1, 5.7, 5.5, 6).

## Reading order

Readers new to the game should read section 3.1 (the program in one page), then 5.12 (how the first-person view is drawn) and 5.14 (sound). Readers who want to extend the decompilation should read section 2.1 (the toolchain, with every switch and its proof) first. Evidence tags and the addresses they cite are the same across the document: `1B82:00C7` is a resident function (segment:offset in the load module), `ovl2:09CC` an overlay function, `ds:8115` a global in the data segment, and `src/E1B82.C` the file in which it is written.
