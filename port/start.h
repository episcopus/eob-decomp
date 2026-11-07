/* start.h: the program's start in the port (port/start.c) and the replay of a golden (port/replay.c). */
#ifndef TC_START_H
#define TC_START_H

#include "tcport.h"

/* After machine_boot: the units' data, then what the start-up code, the library and the overlay
   manager leave before main; main's argv in *argv, argc returned. */
s16 tc_start(farptr *argv);

/* Plays a recording (tools/port/replay.py writes it) headless from the port's own image, in the game
   directory `game_dir`, dumping the arena at each checkpoint into `dump_dir`; the process's exit code. */
int replay_run(const char *script, const char *game_dir, const char *dump_dir);

/* Plays the game live in a window from the game directory `game_dir` (port/live.c); the command tail
   as INTRO.EXE passes it (" 3 4 1 homie 3": VGA, no sound, the mouse). */
int live_run(const char *game_dir, const char *cmdline, int mouse, int adlib);

#endif
