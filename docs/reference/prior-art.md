# Prior art and licence rules

Everything here is a lead, tagged [H] in the whitepaper until proven on our files. Nothing is copied into `src/` or `tools/` from a source whose licence forbids it.

| Source | What it gives | Licence | Use |
|---|---|---|---|
| `../poolrad-decomp` | The process, the tools (overlay parser, emu2 runner, matcher design, oracle) | ours | Copy and adapt freely |
| ScummVM, Kyra engine (`engines/kyra`, EOB support) | File formats (PAK, CPS/LCW, VCN/VMP, MAZ, INF scripts), game logic | GPL | Read only: formats and behaviour as leads, no code copied |
| Unofficial patch 1.9 release notes (`eob/version.nfo`) | What the patch changed in the data | | Lead for the whitepaper |
| [H] Matching reconstructions of Borland C games (for example id Software titles rebuilt byte for byte with Borland C++) | Evidence that Borland C code generation can be matched, and techniques | various | Read for technique; verify against our compiler |
