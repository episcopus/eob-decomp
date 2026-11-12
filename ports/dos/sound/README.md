# ports/dos/sound: the Tandy FM study

Can the Tandy 1000's SN76489-class chip play the game's AdLib sound? This is the desktop prototype of the proposal: keep
Westwood's AdLib driver (`src/E_TEXT.ASM`, its scripts in `ADLIB.DAT` and `SOUND.DAT`) and replace only what reaches the
card, with a mapper that turns the OPL2 register writes into SN76489 writes at the driver's own tick. The study produces
sound files to listen to and numbers to check; four listening passes chose a version of each sound, mostly sounds
designed for the chip, and the DOS build now plays those choices (`SN.DAT` and `SNSOUND.DAT`, "The game's sound"
below).

```bash
make -C ports/dos sound
```

Then open `ports/dos/build/sound/index.html`: every clip in four versions at the same loudness. Needs `make ymfm` at the
repository root (the reference renders) and the unpatched DOS build (`make -C ports/dos ref`, built on demand). The four
captures take two to four minutes each and are kept until `make clean`; rendering takes about a minute.

## The pipeline

| Step | File | What |
|---|---|---|
| Capture | `capture.py` | Runs the unpatched rebuild on the 8088 model (`tools/tandybox.py`), AdLib (card digit 1) or Tandy (3). For `sfx` it waits at the title screen and starts each of the 116 game sounds `ADLIB.DAT` maps with a call of `PlaySound` (177D:0070) at a keyboard poll, one after another, with the silence script between; game sound 20 (the character generation music) gets 110 s. `intro` runs `INTRO.EXE` for 170 s; `introparts` runs it once per sound of its 14 with every other request skipped (the intro's sounds alone, "The game's sound"). Writes the chip log of each window. |
| Map | `tandymap.py` | The model of the mapper (below): OPL2 log in, SN76489 bytes out. |
| Render | `render.cpp` | The OPL2 log through ymfm (the reference), the SN76489 bytes through an emulator of the Tandy's chip; 44.1 kHz WAV, each at the same RMS of its sounding part. Also the measurements the mapper uses (`--patches`, `--lines`) and the spectral centroid used below. |

Clips: `build/sound/clips/<name>/` with `adlib.wav` (the original on an AdLib), `tandy-now.wav` (the game's own Tandy driver
with `PCGAMESN.DAT` or `PCSOUND.DAT`; absent when that driver has no sound for the number), `tandy-plain.wav` and
`tandy-rich.wav` (the mapper), and the logs each was rendered from.

## The mapper

It runs once per driver tick (72.8 Hz), after the driver's own writes, and sees only the OPL2 registers, which an 8086
version would keep in a 256-byte shadow inside `AdlibWrite`. Per FM voice it keeps:

- an envelope for each operator from the patch's rates and levels: an exponential attack, decay to the sustain level,
  the sustain held or not (bit 5 of register 20h), release; key scaling of the rates included; and the key scale level
  (KSL) of each operator, which takes up to 20 dB off a high note (game sound 115's modulator: about 10);
- the pitch, settled when the note starts: the carrier's frequency (waveforms 2 and 3 repeat every half cycle, so they
  sound an octave up), or, when the modulator is strong and slow against the carrier so that the lines fill a harmonic
  series below it (the intro's arpeggio), that series' fundamental. It used to be decided every tick from the
  modulator's envelope, which jumped an octave in the middle of game sounds 103 and 115;
- what the note is, settled at its first audible tick, from a measurement: every voice sound the captures key on (its
  11 patch registers and block) is played for 300 ms on ymfm and its spectral flatness taken (`render --patches`, cached
  in `build/sound/patchnoise.json`). From 0.4 it is noise (white noise is about 0.56), from 0.2 a tone with noise in it
  (both, the noise 6 dB down), below that a tone. Rules on the registers did not work: feedback 6 with a strong
  modulator is the warm arpeggio that opens the intro, and a pitch under 45 Hz is the character generation music's
  bass;
- its spectrum, also measured: each voice sound held at full level on ymfm with its modulator at 0, 3, ... 45 dB, and
  the 12 strongest lines at each level as (frequency over the base frequency, dB) (`render --lines`, 289 tables in
  `build/sound/lines.json`). Between two levels the lines are interpolated, so as the modulator's envelope moves the
  lines fade in and out as they do on the card. A formula (Bessel functions of the FM index) was tried first; it holds
  only for sine operators without feedback, which 45 of the 301 voice and sound pairs the captures key use.

Each tick it ranks the audible tone voices (loudness, a bonus for a new note, for keeping the channel it had, and, when
there are more than three, for the lowest and the highest), gives the best three the tone channels (a voice keeps its
channel), and gives the noise channel to the loudest noise. Levels map 3 dB of OPL2 attenuation to one 2 dB step of the
SN76489, so the AdLib's 45 dB fits the chip's 30 and quiet or distant sounds still sound. Only the registers that
changed are written. Below the chip's floor (109.3 Hz):

- in the music (the intro, game sound 20), a note goes an octave up;
- in a game sound, the voice buzzes at its own pitch: periodic noise clocked by tone 2 at 15 times the pitch, down to
  7.3 Hz (game sound 108's growl is at 23 Hz). This is what most of the sounds picked "now" or "none good" in the first
  pass have in common (below).

`rich` spends the tone channels the moment leaves free on further lines of the voices' measured spectra: the loudest
lines not already playing, each kept on its channel while it lasts (it must lose by 9 dB to be replaced) and none on a
period already in use. A voice's played lines are balanced as its spectrum has them, keep their dB apart one for one
(2 dB a step), and together carry the voice's whole power. With no line to play, a slight detune of the loudest voice.
A low noisy voice clocks its noise from tone 2.

Per game sound, `STYLE` in `tandymap.py` overrides this. `noise` (18 sounds, from listening) puts the whole sound on
the noise channel, as loud as its voices together, its brightness the centroid of their measured lines: in `plain` the
nearest of the three fixed rates, in `rich` white noise clocked by tone 2, following the centroid tick by tick (but not
below a centroid of 250 Hz, where the noise becomes a sparse crackle: game sound 40).

## What it shows (2026-11-09, second pass)

Measured by `make sound`. Envelope match is the correlation of the 10 ms levels of a Tandy render with the AdLib
render: how well loudness and timing follow the original. The centroid compares brightness. The references changed in
this pass (the waveform fix below), so the numbers are not comparable one for one with the first pass's.

| | Tandy now | Mapper, plain | Mapper, rich |
|---|---:|---:|---:|
| Game sounds that play (of the 116 AdLib maps) | 65 | 114 | 114 |
| Envelope match, median (the 65 both play) | 0.65 | 0.92 | 0.92 |
| Envelope match of 0.8 or more (same 65) | 23% | 87% | 83% |
| Centroid within an octave of AdLib (same 65) | 26% | 40% | 50% |
| Character generation music (game sound 20) | none | 0.96 | 0.96 |
| Intro, envelope match | 0.07 | 0.98 | 0.98 |

- The two game sounds the mapper leaves silent: 57 peaks 46 dB down (a slow attack on a quiet patch) and 59 is only the
  silence script's clicks on the AdLib too.
- The music needs more than three voices most of the time: three or more audible voices in 58% of the intro's ticks and
  88% of the character generation music's, which leaves out 2.2 voices a tick on average in the intro and 3.7 in the
  music. The allocator decides what is heard there; this is where a table of per-track priorities would go.
- The chip traffic is small: a median of 39 bytes a second, 264 at most. At about 36 clocks a write
  (the 32 wait states of the Tandy 1000 HX manual, plus the OUT) that is under 0.2% of the EX.
- Brightness: `rich` is now within an octave of the AdLib in half the sounds (median 0.5 octaves darker, `plain` 1.1).
  Game sound 115 checks the spectrum: its AdLib render has the carrier at 883 Hz and sidebands at 12.3 and 14.1 kHz,
  14 dB under; `rich` plays 883 Hz and 14.0 kHz 14 dB under.
- The buzz is the weak point of the envelope match: periodic noise is a train of narrow pulses, which 10 ms levels see
  as a flicker (game sound 108: 0.34 plain, 0.50 rich), while the AdLib's comb of lines is smooth. Whether it sounds
  right is for listening.

## First listening (2026-11-09)

- The first version put tonal voices on the noise channel: the intro's opening arpeggio and the character generation
  bass came out as a low rush of noise, and game sound 15 as white noise. Cause: the register rules above. Fixed by the
  measurement; the music now uses no noise at all.
- Periodic-noise bass is off for the same reason (a rumble under the music).
- Some of today's Tandy sounds were preferred to the mapper's; game sound 17 showed the envelopes working.
- The page has a picker: the best version of each clip and a note, downloaded as `choices.json`. That is the input for
  per-sound choices (mapper or the old driver) and for the allocator's priorities.

## Second listening (2026-11-09)

The first picker pass (`choices-2026-11-09.json`, 113 clips): rich 43, now 35, none good 27, plain 8. With notes:

- 18 sounds heard as noise, to go on the noise channel alone: now `STYLE` "noise".
- 103 and 115 changed pitch abruptly where the AdLib slides: the octave switch in the pitch rule (fixed, above). In 115
  the "slide" is the modulator's envelope opening the sidebands, which the measured lines now follow.
- 108 and 112 to 114 could not go low enough, or lacked the AdLib's fast detail: 108 is a 23 Hz growl played as a
  steady 185 Hz tone (now the buzz); 112 to 114 have their character in sidebands at 6 to 17 kHz that `plain` leaves
  out (now lines in `rich`, up to 16.5 kHz).

What the picks have in common, from the mapper's own view of each sound's voices: most of the time below the chip's
floor are 18 of the 27 "none good" sounds and 21 of the 35 "now" sounds, against 13 of the 42 picked rich; the
sounds picked rich are also the ones with melodies and slides (a median of 10 pitch changes a second, against none
for the other picks). That is what the buzz is for.

A bug found on the way: the AdLib renders of the game sounds were made without register 01 (waveform select), which
the driver turns on, so every voice played sine waves; 256 of the 301 voice and sound pairs use the other waveforms or feedback.
The intro's capture writes the register itself and was right. The first pass compared against the wrong references for
many game sounds; the page now shows each clip's first pick beside a fresh picker.

The page tags each clip with what changed since the first pass, from counters in the mapper (a rule is tagged when it
changed at least 3 ticks, 40 ms): you asked (24), noise only (18), pitch held (31: the first pass's rule, which picked
the pitch every tick from the modulator's level with its envelope, would have jumped an octave), low buzz (49), high
detail (27: rich lines above 4 kHz), key scale (4), and new AdLib (92: an audible voice on a waveform other than the
sine, so the reference itself changed). A bar of filters shows one tag at a time, the first pass's "now" or "none
good" picks, or the clips not picked yet.

## Third pass (2026-11-10)

The second picker pass (`choices-2026-11-10.json`, 116 clips): plain 39, rich 37, now 22, none good 18; 16 of the 18
"none good" were "none good" in the first pass too. What changed, from the notes:

- The page matches loudness as heard: the K-weighted RMS of ITU-R BS.1770 (a +4 dB shelf above about 1.7 kHz, a
  high-pass at 38 Hz) in place of the raw RMS. At one raw RMS a buzz rich in harmonics sounds louder than the AdLib's
  smooth low tone; this was game sound 33's "too loud".
- `TWEAK` in `tandymap.py`, per sound: vibrato (6 Hz after the attack: 83, 110, 114, 115 at 30 to 35 cents, 64 at 25),
  soft noise under 114, a fade of one volume step every 3 ticks at most (111), and "low": a voice under the chip's
  floor plays its loudest measured line above the floor at that line's own level, not its fundamental folded up at
  the voice's full level, and no buzz. In the intro this is the drone of the first part (a 16 Hz voice whose 128 Hz
  harmonic is 20 dB down, which played as a full-volume square wave), now about 14 dB softer; in 64 the wail is its
  harmonics around 1.2 kHz, not a 17 Hz buzz.
- Designed sounds (`designs.py`, a fifth column): for the 16 picked "none good" twice, and the low rumbles 69 and 71, a
  new sound written as a script of the chip's registers, one state a driver tick, from what the sound is used for.
  The uses come from the source: the monster type table (`src/D07C8.C`, movement sounds), the spell table, the
  portal animation (`src/EOV5.C`) and the level scripts (15 is the teleporter, in 34 scripts). 109 is never played;
  AdLib gives it the same sound as 107 and 108. The techniques are the chip's usual ones (the docstring): software
  envelopes, pitch sweeps with white noise clocked by tone 2 swept with them, one tick of white noise for an attack,
  periodic noise for a growl under the floor, vibrato after the attack, a detuned or delayed second channel. A design
  writes at most 350 bytes a second. It is a fixed script: an 8086 version plays it as the game's own Tandy driver
  plays `PCGAMESN.DAT`, and positional sounds need the distance attenuation added.
- The page shows both earlier picks under each name and highlights the last one in the picker, with filters for the
  clips that changed since the last pass (their chip writes compared with the copy in `build/sound/pass2/`), the
  designed ones, the ones with notes, and each last pick.

## Fourth pass (2026-11-11)

The third pass kept every designed sound (its file was the second pass's picks, unchanged; `choices-2026-11-11.json`
is those picks with the 18 designed sounds set to "designed"). So every other game sound but the two pieces of music
(the intro and 20, character generation) now has a design too: 97 new ones, 115 in all, the mapper's versions left as
they were (their chip writes compared with the copy in `build/sound/pass3/`: none changed).

- The uses come from the source, each cited in its design (`use=`): the monster type table (`src/D07C8.C:525`, field
  +16h the attack sound, played at the party's cell; +17h the movement sound, played at the monster's cell and so
  quieter with distance), the spell table (`src/D07C8.C:394-450`), the door steps (`src/E1414.C:316-320`), the items
  (`src/E0BD2.C`, `src/E0DE8.C`), the ending (`src/EOV5.C`) and the 770 SOUND ops of the level scripts (switches 1 and
  7, pressure plates 12, walls 13, locks 14, 81, 82, pits 17, quest done 28).
- Each design is made for its use, not copied from the AdLib: a footstep is taps or pads, an attack a swish, a cry or a
  blow, a spell a chime, a sweep or a burst in the spirit of its name; the AdLib's length and where its energy lies
  were kept in view. Sounds the AdLib shares now differ by use (the dwarf's and the skeleton's attacks, the three
  cures, which rise from two notes to a full chord). 16 and 78 are never played; they take the designs of 103 and 18,
  whose AdLib sounds they share.
- New building blocks in `designs.py`: a click (one tick of white noise), a thud (a dull click, a falling growl under
  the floor, a falling tone), a swish (white noise clocked by tone 2, its rate up and down), a ping, a chime of struck
  notes, a growl, pitch steps, an arpeggio, a wobble. A design writes at most about 360 bytes a second (16 and 103).
- The page starts on the new designs, with the Designed column next to the AdLib one; filters group them by use
  (footsteps, attacks, spells, doors and mechanisms, items, the party, magic places, hits, ambience, deaths) and show
  the ones not picked yet. Every clip starts on its last pick, so the downloaded file holds every clip.

## The game's sound (2026-11-11)

The fourth pass (`picks.json`, 117 clips) kept the designs: designed 110, now 4 (3 and 4, a door opening and closing;
6; 76), plain 1 (119), rich 2 (the two pieces of music). Those picks are now what the game and its intro play on a
Tandy: `tandy-sound.patch` (in `../patches/series`) and two new files beside the game's, `SN.DAT` (EOB.EXE) and
`SNSOUND.DAT` (INTRO.EXE).

```bash
make -C ports/dos sn            # build/sound/SN.DAT and SNSOUND.DAT (the clips' chip logs are their input)
make -C ports/dos sound-check   # the patched programs play every sound as the model of the player does
```

- `sndat.py` reads each picked clip's chip log as one state of the chip's eight registers per driver tick (72.8 Hz)
  and writes the changes as a stream of nibbles (its docstring has the format): which registers change, a tone as a
  step of up to 7 or a byte, or its period; a volume or the noise as one nibble; pauses. What cannot be heard is left
  out (the period of a silent tone, unless tone 2 clocks a sounding noise; the noise register while the noise is off).
  `Player` is the player in Python, call for call, and the build checks that every sound comes back from it as picked.
- `SNPLAY.ASM` is the player (695 bytes with its state), assembled by the pinned TASM and linked by TLINK. Both files
  carry the same player.
- The files have the layout `PCGAMESN.DAT` and `PCSOUND.DAT` have, so the loaders read them the same way: the 120-byte
  map of sounds, then the driver block, which goes to cs:0416 of the AdLib driver's segment, which a Tandy never runs:
  a table of 128 driver sounds, the player, the streams. `SN.DAT`: 114 streams (two pairs of game sounds share one:
  16 and 103, 108 and 109; and 78 and 18), 19,505 bytes, the block 19,385 of the 19,474 bytes to the end of EOB.EXE's
  AdLib driver. `SNSOUND.DAT`: the intro's 14 sounds, 11,036 bytes, the block 10,916 of the 14,000 INTRO.EXE reads.
- The player has two slots, the music and an effect, which play at once. A sound whose table word has bit 15 set is
  music (the intro's sounds 1 to 3); every other sound is an effect (all of EOB.EXE's). A new sound replaces the one
  in its slot. The effect takes each channel it writes, silencing it first, and holds it to its end; the music's
  writes to those channels are kept, not made, and when the effect ends each channel it held gets the music's
  period (or noise) and volume back. With no music playing that is silence, so the game plays one sound at a time,
  as the old driver did. Every fourth pass of the driver's timer (72.8 Hz) a slot plays one frame. An effect from a
  distance of 16 or more is not played and what plays goes on, as on the AdLib; nearer, each volume is
  (distance - 1) x 7/8 steps quieter, the AdLib's level offset at 4 OPL2 steps to one 2 dB step.
- The patch: `LoadSoundDriver` opens `sn.dat` on a Tandy (the literal `pcgamesn.dat` from its seventh letter, so
  DGROUP keeps its layout) and reads up to the larger block; `PlaySound` and `PlaySoundAtCell` write the distance
  of the sound they start where the player reads it (0, or `gCurAttenuation`); `_SoundTick` calls the player when
  the song's segment holds its mark ('SN' at cs:051A), with the offset of its `gSongPtr` (the driver's variables lie
  4 bytes further on in INTRO.EXE, whose copy of the driver is built from the same file), and otherwise plays notes
  as before, so the PC speaker is unchanged. INTRO.EXE's Tandy case opens its literal `pcsound.dat` through
  `FileOpenSn` (a new routine at the end of `E1C0E`, the file layer), which makes the name's first two letters "sn":
  a new literal, or code that moved, would cost the rebuild its places for the unit's literals (it finds them by
  matching the code), so StartupGame keeps every byte but that call's target. The bytes come from code with no
  effect: two dead blocks and a repeated read in `E177D`, register saves and a clamp no path reached in `E1948`;
  `E1C0E` has room after pak-lookup.patch.
- The character generation music (game sound 20) ends 102.1 s after its start, where the AdLib sets the script flag
  (measured on the reference: the `C7h 01` at the end of driver sound 67), and sets it too, so the character screens
  play it again while they wait for a key (`WaitKeyOrPause`, `src/EOV0.C:232`). The old Tandy driver had no music. It
  plays as an effect, as the old player had it: a sound in character generation still cuts it.

The intro (`capture.py introparts`, `tandymap.py intro`): the pick was rich for the 170 s of the intro the study had
captured as one clip, music and effects together. That clip cannot be played as it is: the intro runs about 250 s
before it waits for a key, its scenes start the sounds at their own pace (on the Tandy model the title comes at 73 s,
on the AdLib at 79 s; with this port's speed patches at 29 s), and each sound must start when its scene asks for it.
So each of the intro's sounds was captured alone on the AdLib (every other request skipped) and mapped alone, rich:

| Sound | AdLib driver | What | Length | Envelope match |
|---|---|---|---:|---:|
| 1 | 01h | music: the Westwood card | 30.8 s | 0.83 |
| 2 | 0Bh | music: the title and the scenes after it | 85.7 s | 0.87 |
| 3 | 15h | music: the last scene | 27.9 s | 0.84 |
| 4 to 14 | 1Eh to 29h | effects (a rumble, hits, steps, magic) | 0.1 to 5 s | 0.28 to 0.98 |

- The music uses the three tones as the whole intro did (the low drone of the first part as before, `TWEAK` "low").
  The effects play over it, so `tandymap.py` maps them without the further lines (they take no more tones than they
  have voices that sound) and from tone 2 down, where the music has its further lines.
- Sounds 3, 4, 8 and 9 come after the 170 s the listening passes heard: nobody has listened to them yet. The clips
  are in `build/sound/clips/intro01` to `intro14` (`adlib.wav`, `tandy-rich.wav`); 4 (a rumble on noise clocked by
  tone 2) matches the AdLib's envelope worst, 0.28.
- With the speed patches the intro's scenes run faster than the music was written for: the title's music starts 2 s
  before the Westwood card's ends and cuts it, and runs on under the scenes after it to 114 s.

How it was checked:

- `sndat.py build`: every sound through `Player` gives the picked audible state at every tick, in the slot it is in.
- `make sound-check`: the patched game, from the title screen on the 8088 model, plays each of the 116 game sounds;
  its chip writes are those of the model byte for byte, tick for tick (2026-11-11: 116 of 116). Then the patched
  intro through all its scenes: every one of the player's calls writes what the model's does for the same starts
  (2026-11-11: 47 sounds asked for, 47 starts taken up, 3 of them music, 41,790 calls as the model's;
  two errors planted in the model, an effect that gives its channels back without their volumes and music written
  over a channel the effect holds, were each caught at the first call they changed).
- `make func`: with the patch the gate compares the sounds the programs ask for (EOB.EXE's `PlaySound` and
  `PlaySoundAtCell`: the time, the sound, the cell; INTRO.EXE's `PlaySound`, through `GetSoundData`) instead of the
  chip writes, checks every call of the player against the model, and leaves out of DGROUP the old player's state
  and its playing flags, and of the file reads the old and the new sound files (2026-11-11: the seven sessions the
  same; the intro's 46 sounds all played as the model has them).

Not done: nothing has been heard on a real EX: periodic noise on its chip is still the open question (below). A
priority between effects (a far step cutting a near spell) is not there: the newest effect wins, as with the old
driver.

## What this does not tell

- Nobody has listened yet. The numbers say timing and dynamics follow the original; whether it sounds good is a matter
  of ears, and the rules above are a first set.
- The chip is a model: SMS Power's notes for the Tandy 1000 (15-bit noise register, taps on bits 0 and 4), a period of 0
  counted as 1024, a 12 kHz output filter. The EX's own chip (reportedly an NCR 8496) is not confirmed, and periodic noise
  is the part most likely to differ (a 16-bit register would put every buzz 6% lower). The buzz depends on it, so a test
  on the EX comes before it is kept.
- The measured lines are taken at the pitch a voice sound was first keyed at; lines that fold at the chip's Nyquist
  move the wrong way when the pitch changes a long way within a note.
- The envelope is an approximation of the OPL2's, not its exact counters.
- Times are the 8088 model's (the program's own PIT ticks), so the tempo is the game's; the captures are of the unpatched
  rebuild with nothing else happening (the title screen).
- The mapper here is Python. It is not in the game: what the game plays is its output for the three clips picked plain
  or rich and the intro's sounds, fixed in `SN.DAT` and `SNSOUND.DAT` like the designs (above).
