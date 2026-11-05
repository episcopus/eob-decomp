# The whitepaper as a PDF

`make handbook` typesets `docs/design/WHITEPAPER.md` (with the opening chapter `brief.md`, `docs/reference/prior-art.md`
and `work/status.md`) into `docs/Eye-of-the-Beholder-Whitepaper-v<version>.pdf`, the way poolrad-decomp typeset its handbook:

    Markdown -> HTML (python-markdown) -> figures placed -> Chromium PDF (twice, the second pass fills the contents) -> pypdf bookmarks

- `build.py`: chapters (one per whitepaper section, section 5 one per `5.n`), figure placement, the two-pass contents, bookmarks.
  The version and date are constants at its top. No dash characters reach the output.
- `style.css`, `fonts.css`: page size (160.2 x 228.6 mm), typography, chapter and part openers.
- `figlib.py`: the diagram toolkit. A figure is `figs/<id>.py` (writes `figs/<id>.svg`) plus `figs/<id>.txt` (its caption). A line
  `<!-- FIG <id> -->` in a Markdown source places it; the build numbers the figures. `preview.py out.png a.svg b.svg` renders figures to look at.
- `brief.md`: the opening chapter, written by hand for each version. `shot.mjs`: the Playwright call that prints the PDF.

## What you need

| Thing | How |
|---|---|
| `build/handbook/venv` | `make handbook-setup` (markdown, beautifulsoup4, pypdf) |
| Playwright for node and its Chromium | `npm install playwright --prefix build/handbook && npx playwright install chromium`, or `HANDBOOK_NODE_MODULES=<a node_modules with playwright>` |
| Fonts in `build/handbook/fonts` | `Inter.ttf` (variable), `DejaVuSansMono.ttf` and `DejaVuSansMono-Bold.ttf`; optionally `Lora-Regular.ttf`, `Lora-Italic.ttf`, `Lora-Bold.ttf` (the body face: Georgia is used without them) |

Nothing here is committed that the project does not own: fonts and node modules stay in `build/` (ignored).
Whitepaper figures that are generated from facts (sizes, counts) are written by hand from the cited evidence; when the
evidence changes, change the script and rebuild.

## Data the figures read

`data/` holds measurements a figure script cannot recompute without running the oracle or a decoder by hand: `snd_opcounts.json`
(how often each AdLib script command occurs in `ADLIB.DAT` and `SOUND.DAT`), `snd_timeline.json` (the tick at which each scene of
the intro starts, from breakpoints in the oracle, 2026-11-05) and `paldiff.json` (the five `.PAL` files against `EOBPAL.COL`).
They are evidence for the figures, not inputs of the game; section 5.14 and 5.15 say how each was measured.
