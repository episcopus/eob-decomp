# ports/dos: Eye of the Beholder for the Tandy 1000 EX

The DOS game rebuilt from `src/` by the pinned Turbo C++ 1.00, TASM and TLINK, with a patch series that makes it faster on
a Tandy 1000 EX (8088 at 7.16 MHz, Tandy video and sound) and changes nothing about what the game does. Same method as
`../../poolrad-decomp/ports/dos`, with two differences forced by this binary: a C translation is not used, and every unit
keeps its original length (see below).

Regenerate and deploy (needs `make setup`, `make tc-install`, `make emu2` at the repository root; DOSBox-X for the check):

```bash
make -C ports/dos test
make -C ports/dos dist
```

`build/dist` then holds `EOB.EXE`, `INTRO.EXE`, `TGA.OVL`, `INSTALL.TXT` and `SHA256.TXT`. Copy the three programs over the ones in the game directory on the EX, keeping backups (`INSTALL.TXT`).

| Target | What |
|---|---|
| `make game` | `build/game/EOB.EXE` and `TGA.OVL`: the patches of `patches/series` |
| `make ref` | `build/ref`: no patches; must link the original program (the reference) |
| `make func` | the gate: both builds play `sessions/*.keys` on the program clock; screen, pages, DGROUP, sound and file reads must be the same at every checkpoint |
| `make prof` | 8088 model: what each action costs, ref against game (`BUS=4\|5\|6`: wait states on the bus, `SESSION=`) |
| `make tandy` | DOSBox-X `machine=tandy`, headless: ref and game show the same screens through a real BIOS and DOS |
| `make spans` | the timed effects: every spell burst and logo dissolve, ref against game |
| `make dist` | the files to copy to the machine |
| `make play` | a DOSBox-X window on the Tandy machine (for a look, not a measurement) |

## How it is built

`src/` is never edited. `tools/build.py` copies the tree to `build/<name>/stage`, applies `patches/series`, and links with the
project's own `tools/rebuild` (its copy in the stage gets two additions, both applied by `build.py`: a C module with
`#pragma inline` compiles through TASM, and a module that got shorter is padded to its original length with a module of NOPs in the same segment).

The padding is the first thing this port learned. Code offsets are written as numbers in several assembly units of `src/` (the tick handler's offset, for one), so a change in the length of any resident unit shifts every address after it and the game dies at start-up. So every patched resident unit stays at its original length: new code replaces old code in place, and what is left over is NOPs. Overlays may change size freely (the overlay manager relocates them). The driver `TGA.OVL` is a separate file and free to grow.

| Patch | Change | Unit |
|---|---|---|
| `tga-glyph.patch` | `DrawGlyph`, all text: the two colours' words for each four glyph bits are built when the colours change (16 entries), rows drawn with registers in place of memory counters | TGA.OVL |
| `tile-draw.patch` | `DrawTile`'s Tandy path, the wall view: a tile's flags tested in registers; the flipped tile by `xlat`; the composite tiles (a tile drawn over another) from one built copy of the base tile instead of segment switches per byte; also `NodeScan` for the patch below. `BlitMaskedByte`, whose only callers were the old composite path, is dropped | E17A6 |
| `dispatch-idle.patch` | `DispatchNodeEvent`: with no key and no mouse, walking the node list changes nothing while each node's previous state bits match its current ones; `NodeScan` finds the first that does not. The dead `DispatchKey` is emptied to pay for the call site | E16A5 |
| `view-columns.patch` | `CopyViewColumn`, `ToggleViewColumn` and `ComputeViewSpan`, which build the wall block map of every frame: inner loops in assembly with the clipped column range computed once | EOV4 |

| `tandy-dissolve.patch`, `tga-dissolve.patch` | `Dissolve` (the logo cards, the title) and `ShuffleGrid`: on Tandy, one driver call builds tables of byte offsets and nibble masks and copies all the pixels in registers, same pixels in the same order, instead of a far `GetPixel` and `PutPixel` each | IOV0, EOV5, TGA.OVL |
| `heapcheck.patch` | `RunCampMenu`: no walk of the far heap at every menu key (a debugging check left in the loop). Its message stays in the literal pool, so DGROUP keeps its layout | EOV1 |
| `tga-line.patch` | `DrawLine`: horizontal lines filled by bytes, vertical lines one nibble a row, instead of the general stepping loop (panel frames, menu boxes) | TGA.OVL |
| `particle-burst.patch` | `AnimateParticleBurst` (spell hits): on Tandy a driver routine does a frame in registers; the C loop stays for the other video modes. The four-way `switch` that starts it is compacted to pay for the call | E0EF3, TGA.OVL |
| `tga-glyph2.patch` | `DrawGlyph` again: a two-word loop for even x, the row masks kept between glyphs of the same width and parity, the slow path's bookkeeping out of the fast path; `DrawPageSeg` shorter | TGA.OVL |

The two new driver routines use export slots no other code calls (`SetTileSet`, `DrawMaskedCell16`, which do nothing on this driver). A driver routine was needed: compiling `IOV0.C` through the inline-assembler route shifted its string literals and broke the logo file table, which is a reason for any further patch to prefer the driver or an assembly unit over `#pragma inline` in a C unit with strings.

`make func` is the gate for any further patch: each must leave the game identical (`tools/func.py`).

## How it was measured

Nothing here ran on a real EX. Three separate measurements back each other up:

1. `tools/func.py` plays each session on a program clock (`tools/tandybox.py`): virtual time moves only at the game's own waits (keyboard polls, retrace reads, tick waits, the frame counter spin in `DrawViewFrame`), so a faster build sees the same times and must do the same things. At every checkpoint it compares the 32 KB of Tandy video memory, the 64 KB of pages below it, all of DGROUP (the heap shift of a larger driver, the driver's table of its own offsets, and the pointers to routines a patch moved aside), the Tandy sound writes and the file reads. `INTRO.EXE`'s DGROUP is not compared (it is mostly far pointers to heap blocks whose places follow the driver and overlay sizes); its screen, pages, sound and file reads are. It also gives the paired cost of every action: the same game state, both builds, so the difference is exactly what the patches saved. The method was checked with a planted bug (one entry of the 16-entry glyph table off by one): caught at the first checkpoint.
2. `tools/m8088.py` is an 8088 model: the Intel table clocks, a 4-byte prefetch queue, conditional jumps charged by which way they went, REP per repetition. The bus cycle is 5 clocks (4 plus one wait state: the Tandy 1000 HX technical reference says RAM shared with video inserts "typically one or two" wait cycles; the EX is the same design). `make prof BUS=4` and `BUS=6` are the two ends. The ratios hold across them (below); the seconds do not, and they are estimates.
3. `tools/dbx.py`: DOSBox-X `machine=tandy`, headless, both builds typing the same keys: load the game, turn, sheet. All 7 steady reference screens appear in the game, pixel for pixel, at 320x200.

Disk time is not in any figure: a real floppy or hard card will be slower still on level loads and the first time something is used. Level load is unchanged by these patches (18.9 s to 18.4 s in the model, mostly `Unpack4`, sprite cutting and 5,958 small file reads). The model charges no wait for keyboard and BIOS work beyond the instructions themselves.

## Results, second pass

What the player asked to be faster, from the same 8088 model and the same gate (`make func`, sessions `walk`, `combat`, `panels`, `menus`, `spell`, `intro`).

Timed effects, from the first instruction to the last (`make spans`, one wait state; each row one run):

| Effect | Reference | Now | Gain |
|---|---:|---:|---:|
| Logo card dissolve (each of four) | 14.6 s | 1.64 s | 8.9x |
| Title dissolve | 30.2 s | 3.15 s | 9.6x |
| Magic missile hit (the sparkles) | 3.9 s | 1.85 s | 2.1x |

The dissolves are the bulk of the intro's time on this machine: 88.6 of the 144 modelled seconds to the end of the cards and the title. The sparkles have reached a floor the game sets itself: two retrace waits per frame of the effect (56 frames, 0.93 s of waiting), so further gains there would have to change the speed of the animation.

The paired work of the same game state in milliseconds (reference to now, medians). These replace the first pass's paired figures rather than extend them: the gate's clock now also advances in `DrawViewFrame`'s frame wait, which changes the timeline of each session, so the two tables are not comparable row by row:

| Action | n | Reference | Now | Gain |
|---|---:|---:|---:|---:|
| Step | 23 | 307 | 209 | 1.47x |
| Turn | 14 | 333 | 206 | 1.62x |
| Attack | 14 | 57 | 24 | 2.35x |
| Sidestep | 2 | 133 | 54 | 2.46x |
| Touch a wall button | 3 | 46 | 27 | 1.74x |
| Character sheet | 3 | 905 | 475 | 1.90x |
| Inventory | 4 | 302 | 207 | 1.46x |
| Back to the map | 1 | 525 | 331 | 1.59x |
| Camp menu opens | 1 | 2,080 | 949 | 2.19x |
| Camp sub-page opens | 3 | 1,592 | 615 | 2.59x |
| Back out of a sub-page | 3 | 2,466 | 1,080 | 2.28x |
| Move the camp menu's highlight | 8 | 301 | 61 | 4.94x |
| Move a sub-page's highlight | 3 | 123 | 71 | 1.74x |
| Spell panel opens | 3 | 403 | 245 | 1.64x |
| Spell panel highlight moves | 3 | 431 | 216 | 2.00x |

Each new patch's share of the menu time (cumulative, key press to idle, seconds, one wait state; heap and lines measured on the first pass build):

| After | Camp opens | Sub-page opens | Back out | Sheet |
|---|---|---|---|---|
| first pass | 1.378 | 0.888 | 1.588 | 0.570 |
| + heapcheck | 1.270 | 0.838 | 1.473 | 0.570 |
| + tga-line | 1.094 | 0.745 | 1.279 | 0.568 |
| + glyph2, burst, dissolve | 0.996 | 0.697 | 1.163 | 0.513 |

Steps and turns did not move this pass (key press to idle: step 0.208 s to 0.206 s, turn 0.226 s to 0.226 s). What they cost is `DrawTile` (75 ms), the page copy (24), `ComputeViewSpan` (16), the dispatch loop (17): the text and line work this pass removed is not on that path.

A candidate that was measured and dropped: a cache of flipped wall tiles. An offline trace of the 3,168 flipped tiles in the `walk` session, against a direct-mapped cache, hits 7.8 % at 2 KB and 12.8 % at 4 KB (403 distinct tiles): not worth the memory.

## Results, first pass

Paired cost of the same game state, from `make func` (milliseconds of 8088 work at 7.16 MHz, one wait state; the median across the n actions of the session):

| Action | n | Reference | Game | Median | Total |
|---|---:|---:|---:|---:|---:|
| Step in the dungeon | 23 | 331 | 209 | 1.59x | 1.58x |
| Turn in the dungeon | 14 | 314 | 202 | 1.55x | 1.60x |
| Attack (combat) | 14 | 160 | 114 | 1.40x | 1.46x |
| Sidestep (combat) | 2 | 133 | 67 | 1.99x | 1.93x |
| Touch a wall button | 3 | 46 | 27 | 1.74x | 1.70x |
| Inventory screen | 4 | 302 | 228 | 1.33x | 1.29x |
| Character sheet | 3 | 943 | 564 | 1.67x | 1.70x |
| Back to the map | 1 | 525 | 407 | 1.29x | 1.29x |
| Camp menu | 2 | 2080 | 1232 | 1.69x | 1.58x |

Time on the machine, key press to game idle, on the cycle clock (medians; a real game with the timers running, so the monsters may differ between builds):

| Action | 0 waits (BUS=4) | 1 wait (BUS=5) | 2 waits (BUS=6) |
|---|---|---|---|
| Step | 0.267 s to 0.174 s (1.54x) | 0.343 s to 0.208 s (1.66x) | 0.471 s to 0.242 s (1.95x) |
| Turn | 0.296 s to 0.195 s (1.52x) | 0.383 s to 0.226 s (1.69x) | 0.450 s to 0.256 s (1.75x) |
| Sheet | 0.801 s to 0.472 s (1.70x) | 0.989 s to 0.570 s (1.74x) | 1.155 s to 0.639 s (1.81x) |

Each patch's share (cumulative, medians of paired work, same order as `patches/series`):

| After | Step | Turn | Sheet | Attack |
|---|---|---|---|---|
| reference | 331 ms | 314 ms | 943 ms | 160 ms |
| glyph | 331 | 314 | 575 (1.64x) | 129 (1.24x) |
| + tile-draw | 288 (1.15x) | 291 (1.08x) | 575 | 129 |
| + dispatch-idle | 269 (1.23x) | 270 (1.16x) | 564 | 114 (1.40x) |
| + view-columns | 209 (1.59x) | 202 (1.55x) | 564 | 114 |

Sizes now: EOB.EXE 264,528 bytes (+64 from the reference), INTRO.EXE 88,080 (+48), TGA.OVL 13,509 (+2,069). The game needs about 2 KB more memory than the original (the driver's tables and two new routines; the first pass measured 16.6 KB of DOS free against 17.6 KB at the same point in the session, before this pass's 1 KB of driver code).

## Where the time goes now, and what to try next

Step and turn, per action, after these patches (`make prof`): `DrawTile` 75 ms (flipped tiles about 28 of it, by far the costliest per tile), page-to-screen copy 24, `ComputeViewSpan` 16, `DispatchNodeEvent` and `NodeScan` 17, `DrawShape` 10, sound interrupt 10. The transfers themselves are near the floor of the bus: the view is 10,560 bytes written twice, about 55 ms of the 8088's bus; everything else is the C code around it, so a large further gain would have to redraw less, not faster.

Candidates (each needs `make func` and a measurement): `AnimateConeOfCold` by the same driver route as the burst (level 5 spell, not yet profiled); `DrawTile`'s flipped path as a tile-at-a-time routine (28 of its 75 ms a step, no cache: see above); a 256-entry text colour table once the colour changes are measured; `Unpack4` copying by words (needs a byte of room in E1D9A). Also an option rather than an optimization: halving the two retrace waits per frame of the spell sparkles, if the sparkles should feel faster than the game's original pacing. And the real machine: copy `build/dist` to the EX and compare by hand; the model is the only timing evidence so far.

What is left unverified: nothing here ran on an 8088; a real EX may have DMA and bus behaviour the model does not, and the disk is not modelled. Behaviour was checked in the oracle and DOSBox-X only.
