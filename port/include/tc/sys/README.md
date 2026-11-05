# The library headers tc2c reads

`tools/port/tc2c.py` parses the units of `src/` with these in place of Turbo C++ 1.00's own headers,
which are proprietary and stay in `build/toolchain/` (AGENTS.md). Each declares only what the C units of
`EOB.EXE` use from it; the constants and layouts have Turbo C++ 1.00's values (checked against
`build/toolchain/tc100/INCLUDE`, not copied). The port's runtime implements the functions with Turbo C's
semantics (`port/runtime/tclib.c`; the DOS and BIOS ones go through the machine layer, playbook 80 M3).

`MK_FP`, `FP_SEG` and `FP_OFF` are macros in Turbo C; here they are declared as functions, and tc2c
translates their calls to `TC_MKFP`, `TC_SEG` and `TC_OFF`.
