# docs index

| Folder | What it holds |
|---|---|
| `spec/` | `SPEC.md`: the process specification (goals, approach, verification ladder, phases, ADRs) |
| `design/` | `WHITEPAPER.md`: everything learned about the game, its programs and its toolchain |
| `process/` | Playbooks for each agent role, the orchestrator, the readability rules |
| `setup/` | `SETUP-tc.md`: pinning the Turbo C++ 1.00 and Turbo Assembler toolchain |
| `maps/` | Player maps of the twelve levels and of how they connect (PDF), drawn from the game's level files by `tools/maps/` (`make maps`) |
| `reference/` | `prior-art.md`: other work on the game, its licences and how we use it |

The whitepaper is also typeset as a PDF (`make handbook`); it is attached to the repository's releases rather than kept in the tree.
