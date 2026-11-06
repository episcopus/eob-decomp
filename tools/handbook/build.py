#!/usr/bin/env python3
"""Build the Eye of the Beholder whitepaper PDF from the repository's Markdown (`make handbook`).

The practice is poolrad-decomp's handbook pipeline, adapted:

    Markdown (docs/design/WHITEPAPER.md, brief.md, work/status.md, docs/reference/prior-art.md)
      -> HTML (python-markdown), figures placed from `<!-- FIG id -->` lines (tools/handbook/figs/<id>.svg + .txt)
      -> Chromium PDF twice (pass one finds the page of every heading from the tagged outline, pass two fills the
         table of contents; a third pass if the numbers moved)
      -> pypdf writes the bookmarks and the metadata.

    python3 tools/handbook/build.py [--out PATH] [--figs]      (--figs: run every figs/*.py first)

Needs: python-markdown, beautifulsoup4, pypdf (`make handbook-setup` makes build/handbook/venv), node with Playwright
(HANDBOOK_NODE_MODULES) and its Chromium, and the fonts in build/handbook/fonts (README.md). No dash characters
(U+2014, U+2013) may reach the output: the build rewrites them and fails if one is left.
"""
import argparse, glob, html, os, re, subprocess, sys
import markdown
from bs4 import BeautifulSoup

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
VERSION = "1.0"
DATE = "5 November 2026"
OUT_NAME = f"Eye-of-the-Beholder-Whitepaper-v{VERSION}.pdf"


def find_fonts():
    for d in (os.environ.get("HANDBOOK_FONTS"), os.path.join(ROOT, "build", "handbook", "fonts"),
              os.path.join(ROOT, "..", "eob-decomp", "build", "handbook", "fonts")):
        if d and os.path.exists(os.path.join(d, "Inter.ttf")):
            return os.path.abspath(d)
    sys.exit("fonts not found: put Inter.ttf and DejaVuSansMono*.ttf in build/handbook/fonts (tools/handbook/README.md)")


def find_node_modules():
    for d in (os.environ.get("HANDBOOK_NODE_MODULES"), os.path.join(ROOT, "build", "handbook", "node_modules"),
              "~/.npm/_npx/e41f203b7505f1fb/node_modules"):
        if d and os.path.exists(os.path.join(d, "playwright")):
            return d
    sys.exit("playwright for node not found: set HANDBOOK_NODE_MODULES (tools/handbook/README.md)")


FONTS = find_fonts()

# ---------------------------------------------------------------- structure
PARTS = {
    "I": ("At a Glance", "What this project is, what it proved, and what the game turned out to be."),
    "II": ("The Program", "The programs, how they were built, how they start and run, and the files they read."),
    "III": ("The Game Systems", "Characters, items, the maze, scripts, monsters, combat, magic, the screen, the view and the sound: "
                                "how each works, with every claim tied to the code that proves it."),
    "IV": ("Reference", "What is still open, where the project stands, and the work this one builds on."),
}

TAGS = {"V": ("tv", "Verified"), "I": ("ti", "Inference"), "H": ("th", "Hypothesis"), "?": ("tq", "Open question")}

# whitepaper section number -> part
PART_OF = {"1": "II", "2": "II", "3": "II", "4": "II", "5": "III", "6": "IV"}

MD_EXT = ["markdown.extensions." + e for e in ("tables", "fenced_code", "sane_lists", "md_in_html")]


def read(p):
    return open(os.path.join(ROOT, p), encoding="utf-8").read()


def nodash(s):
    return (s.replace(" \u2014 ", ", ").replace("\u2014", ", ").replace(" \u2013 ", " to ").replace("\u2013", " to "))


def norm(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())


# ---------------------------------------------------------------- figures
_uid = [0]


def svg_inline(path):
    s = open(path, encoding="utf-8").read()
    return re.sub(r"<\?xml[^>]*>", "", s)


def caption_html(text):
    t = html.escape(nodash(text.strip()), quote=False)
    return re.sub(r"`([^`]+)`", r"<code>\1</code>", t)


def fig_block(ident):
    txt = os.path.join(HERE, "figs", ident + ".txt")
    svg = os.path.join(HERE, "figs", ident + ".svg")
    if not (os.path.exists(txt) and os.path.exists(svg)):
        print(f"  !! missing figure {ident}")
        return (f'<figure class="fig new" data-fig="{ident}"><div class="art" style="height:30mm;border:1pt dashed red"></div>'
                f'<figcaption><b class="fn">Figure 0.</b> MISSING {ident}</figcaption></figure>')
    body = svg_inline(svg)
    _uid[0] += 1
    body = re.sub(r'id="(a[0-9A-Fa-f]{6})"', lambda m: f'id="{m.group(1)}_{_uid[0]}"', body)
    body = re.sub(r'url\(#(a[0-9A-Fa-f]{6})\)', lambda m: f'url(#{m.group(1)}_{_uid[0]})', body)
    cap = open(txt, encoding="utf-8").read()
    return (f'<figure class="fig new" data-fig="{ident}"><div class="art">{body}</div>'
            f'<figcaption><b class="fn">Figure 0.</b> {caption_html(cap)}</figcaption></figure>')


# ---------------------------------------------------------------- markdown
def md_to_html(text):
    text = nodash(text)
    # figure lines become a paragraph holding a marker the placer swaps for the figure
    text = re.sub(r"^<!--\s*FIG\s+(\w+)\s*-->\s*$", lambda m: f"\n[[FIG {m.group(1)}]]\n", text, flags=re.M)
    out = markdown.markdown(text, extensions=MD_EXT, output_format="html5")
    soup = BeautifulSoup(out, "html.parser")
    # confidence tags in text nodes outside code
    pat = re.compile(r"\*{0,2}\[(V|I|H|\?)\]\*{0,2}(?!\()")
    for node in list(soup.find_all(string=True)):
        if node.find_parent(["code", "pre", "a"]):
            continue
        s = str(node)
        if not pat.search(s):
            continue
        esc = html.escape(s, quote=False)
        frag = BeautifulSoup(pat.sub(lambda m: f'<span class="tag {TAGS[m.group(1)][0]}">{m.group(1)}</span>', esc), "html.parser")
        node.replace_with(frag)
    for p in soup.find_all("p"):
        m = re.fullmatch(r"\s*\[\[FIG (\w+)\]\]\s*", p.get_text())
        if m:
            p.replace_with(BeautifulSoup(fig_block(m.group(1)), "html.parser"))
    return soup


# ---------------------------------------------------------------- chapters
def split_whitepaper(text):
    """Chapters from WHITEPAPER.md: one per `## N.` section, except section 5, whose intro is a chapter and which has
    one chapter per `### 5.n`. Returns [(section_number, kicker, title, body_md, demote)]."""
    text = re.sub(r"^# .*\n", "", text, count=1)
    secs = [s for s in re.split(r"(?m)^## ", text) if s.strip()]
    chapters = []
    for s in secs:
        head, _, body = s.partition("\n")
        mm = re.match(r"(\d+)\.\s+(.*)", head.strip())
        if not mm:
            continue                      # the lines before the first section
        num, title = mm.group(1), mm.group(2)
        if num == "5":
            parts = re.split(r"(?m)^### ", body)
            chapters.append((num, f"Section {num}", title, parts[0], 1))
            for sub in parts[1:]:
                h, _, b = sub.partition("\n")
                m2 = re.match(r"(\d+\.\d+)\s+(.*)", h.strip())
                chapters.append((num, f"Section {m2.group(1)}", m2.group(2), b, 2))
        else:
            chapters.append((num, f"Section {num}", title, body, 1))
    return chapters


def demote(md_text, n):
    """Subtract n from the level of every heading outside code fences (never above `##`)."""
    out, fence = [], False
    for line in md_text.split("\n"):
        if line.startswith("```"):
            fence = not fence
        m = re.match(r"^(#{1,6}) ", line)
        if m and not fence:
            line = "#" * max(2, len(m.group(1)) - n) + line[len(m.group(1)):]
        out.append(line)
    return "\n".join(out)


def whitepaper_intro(wp):
    m = re.match(r"# .*\n\n(.*?)\n\n", wp, flags=re.S)
    return m.group(1) if m else ""


def build_chapters():
    """[(part, key, kicker, title, lead, source, body_soup)]"""
    chs = [("I", "brief", "", "The Project in Brief",
            "Eye of the Beholder (Westwood and SSI, 1991) is a first-person AD&D dungeon crawler. This project recovers its source: "
            "C and assembly that the original compiler turns back into the original programs, byte for byte, "
            "and an account of how the game works that every claim of can be checked against.",
            None, md_to_html(read("tools/handbook/brief.md")))]
    wp = read("docs/design/WHITEPAPER.md")
    for (num, kicker, title, body, dem) in split_whitepaper(wp):
        key = "s" + re.sub(r"\W", "", kicker.replace("Section ", "")) + "-" + norm(title)[:10]
        chs.append((PART_OF.get(num, "IV"), key, kicker, title, "", "docs/design/WHITEPAPER.md",
                    md_to_html(demote(body, dem))))
    prior = re.sub(r"^# .*\n", "", read("docs/reference/prior-art.md"), count=1)
    chs.append(("IV", "prior", "", "Prior Art and Licences",
                "What other projects found, and what we may and may not take from them.", "docs/reference/prior-art.md",
                md_to_html(demote(prior, 0))))
    st = re.sub(r"^# .*\n", "", read("work/status.md"), count=1)
    chs.append(("IV", "status", "", "Project Status", "Where every phase stands (generated: work/status.md).",
                "work/status.md", md_to_html(demote(st, 0))))
    return chs


def prepare(chs):
    """Heading ids, table classes, chapter openers, TOC rows."""
    toc, chap_no, part_done, htmls = [], 0, set(), []
    for (part, key, kicker, title, lead, src, soup) in chs:
        chap_no += 1
        for hn, h in enumerate(soup.find_all(["h2", "h3", "h4", "h5"]), 1):
            h["id"] = f"{key}-h{hn}"
        t0 = soup.find("table")
        if t0 is not None and t0.find("thead") is not None and not norm(t0.find("thead").get_text()):
            t0["class"] = "meta"
            t0.find("thead").decompose()
        for t in soup.find_all("table"):
            cols = len(t.find("tr").find_all(["td", "th"])) if t.find("tr") else 0
            if cols >= 5:
                t["class"] = (t.get("class") or []) + ["wide"]
        where = html.escape(kicker) if kicker else f"Chapter {chap_no}"
        opener = f'<div class="kicker">Part {part} &middot; {where}</div><h1 id="ch-{key}">{html.escape(title)}</h1>'
        if lead:
            opener += f'<p class="lead">{html.escape(lead)}</p>'
        if src:
            opener += f'<p class="source">Source: <code>{html.escape(src)}</code></p>'
        sec = f'<section class="chapter" style="page: ch{chap_no}" data-title="{html.escape(title)}">{opener}{str(soup)}</section>'
        if part not in part_done:
            part_done.add(part)
            pt = PARTS[part]
            htmls.append(f'<section class="partpage" id="part-{part}"><div class="pnum">Part {part}</div>'
                         f'<h1 class="ptitle">{html.escape(pt[0])}</h1><p class="psub">{html.escape(pt[1])}</p></section>')
            toc.append((0, f"Part {part} · {pt[0]}", f"part-{part}", None))
        htmls.append(sec)
        m = re.match(r"Section (\d+\.\d+)$", kicker or "")
        toc.append((1, f"{m.group(1)} {title}" if m else title, f"ch-{key}", None))
        for h in BeautifulSoup(sec, "html.parser").find_all("h2"):
            if h.find_parent("figure"):
                continue
            txt = re.sub(r"\s*\((`|make |src/|tools/|re/).*$", "", h.get_text(" ", strip=True))
            toc.append((2, txt, h["id"], None))
    return htmls, toc


def number_figures(t):
    n = [0]

    def rep(m):
        n[0] += 1
        return f'<b class="fn">Figure {n[0]}.</b>'
    return re.sub(r'<b class="fn">Figure 0\.</b>', rep, t), n[0]


def toc_html(toc, pages):
    rows = []
    for (lvl, txt, hid, tag) in toc:
        rows.append(f'<div class="t{lvl}"><a href="#{hid}">{html.escape(txt)}</a><span class="pg">{pages.get(hid, "")}</span></div>')
    return '<section class="toc" id="contents"><h1 class="tochead">Contents</h1>' + "".join(rows) + "</section>"


def cover_html():
    svg = svg_inline(os.path.join(HERE, "figs", "cover.svg"))
    return f'''<section class="cover">
<div class="ckicker">eob-decomp &middot; whitepaper</div>
<h1 class="ctitle">Eye of the Beholder,<br>Decompiled</h1>
<p class="csub">How Westwood&rsquo;s 1991 first-person dungeon crawler works, read from the source recovered by decompiling it: the programs, the data, the systems, the screen and the sound, and the proof that the source is the original.</p>
<div class="cart">{svg}</div>
<div class="cfoot"><b>Version {VERSION}</b> &middot; compiled {DATE} from the project&rsquo;s Markdown sources &middot; state: phases 0 to 5 complete (all 1,011 functions of EOB.EXE, INTRO.EXE and START1.EXE at L2; the rebuilds replay every golden session), Phase 6 (one vocabulary) in review, Phase 7 (portable build) under way<br>
Covers: docs/design/WHITEPAPER.md &middot; work/status.md &middot; docs/reference/prior-art.md</div>
</section>'''


def page_css(chs):
    return "\n".join(f'@page ch{i} {{ @bottom-left {{ content: "{ch[3]}"; }} }}'.replace('"', '"') for i, ch in enumerate(chs, 1))


def fonts_css():
    css = open(os.path.join(HERE, "fonts.css")).read().replace("FONTDIR", "file://" + FONTS)
    for f, w, st in (("Lora-Regular.ttf", 400, "normal"), ("Lora-Italic.ttf", 400, "italic"), ("Lora-Bold.ttf", 700, "normal")):
        if os.path.exists(os.path.join(FONTS, f)):
            css += f"\n@font-face {{ font-family: Lora; src: url('file://{FONTS}/{f}'); font-weight: {w}; font-style: {st}; }}"
    return css


def assemble(toc_pages):
    chs = build_chapters()
    htmls, toc = prepare(chs)
    css = fonts_css() + "\n" + open(os.path.join(HERE, "style.css"), encoding="utf-8").read() + "\n" + page_css(chs)
    body = cover_html() + toc_html(toc, toc_pages) + "".join(htmls)
    body, nfig = number_figures(body)
    doc = (f'<!doctype html><html><head><meta charset="utf-8"><title>Eye of the Beholder, Decompiled: Whitepaper v{VERSION}</title>'
           f'<style>{css}</style></head><body>{body}</body></html>')
    assert "\u2014" not in doc and "\u2013" not in doc, "dash in output"
    return doc, toc, nfig


def render(doc, out_pdf):
    path = os.path.join(ROOT, "build", "handbook", "book.html")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, "w", encoding="utf-8").write(doc)
    env = dict(os.environ, HANDBOOK_NODE_MODULES=find_node_modules())
    subprocess.run(["node", os.path.join(HERE, "shot.mjs"), "pdf", path, out_pdf], check=True, env=env)


def outline_pages(pdf):
    from pypdf import PdfReader
    r = PdfReader(pdf)
    res = []

    def walk(o):
        for x in o:
            if isinstance(x, list):
                walk(x)
            else:
                res.append((x.title, r.get_destination_page_number(x) + 1))
    walk(r.outline)
    return res, len(r.pages)


def match_pages(toc, outl):
    pages, i = {}, 0
    for (lvl, txt, hid, tag) in toc:
        k = norm(re.sub(r"^\d+\.\d+ ", "", txt.split(" \u00b7 ", 1)[-1]))[:30]
        j = i
        while j < len(outl):
            if k and k in norm(outl[j][0]):
                pages[hid] = outl[j][1]
                i = j + 1
                break
            j += 1
    return pages


def finish(pdf_in, pdf_out, toc, pages):
    from pypdf import PdfReader, PdfWriter
    r = PdfReader(pdf_in)
    w = PdfWriter()
    w.append(r, import_outline=False)
    stack = {}
    for (lvl, txt, hid, tag) in toc:
        if hid not in pages or lvl > 2:
            continue
        parent = stack.get(lvl - 1) if lvl > 0 else None
        it = w.add_outline_item(txt, pages[hid] - 1, parent=parent)
        stack[lvl] = it
        for d in range(lvl + 1, 4):
            stack.pop(d, None)
    w.add_metadata({"/Title": f"Eye of the Beholder, Decompiled: Whitepaper v{VERSION}",
                    "/Subject": "How Eye of the Beholder works, from its decompiled source",
                    "/Author": "eob-decomp project",
                    "/Creator": "Markdown sources, Chromium, pypdf"})
    w.page_mode = "/UseOutlines"
    with open(pdf_out, "wb") as f:
        w.write(f)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "docs", OUT_NAME))
    ap.add_argument("--figs", action="store_true", help="run every figs/*.py first")
    a = ap.parse_args()
    if a.figs:
        for s in sorted(glob.glob(os.path.join(HERE, "figs", "*.py"))):
            r = subprocess.run([sys.executable, s], cwd=HERE, capture_output=True, text=True)
            if r.returncode:
                sys.exit(f"figure script failed: {s}\n{r.stderr[-800:]}")
    work = os.path.join(ROOT, "build", "handbook")
    os.makedirs(work, exist_ok=True)
    doc, toc, nfig = assemble({})
    print(f"figures: {nfig}")
    p1 = os.path.join(work, "pass1.pdf")
    render(doc, p1)
    outl, n1 = outline_pages(p1)
    pages = match_pages(toc, outl)
    pages["contents"] = 2
    doc, toc, nfig = assemble(pages)
    p2 = os.path.join(work, "pass2.pdf")
    render(doc, p2)
    outl2, n2 = outline_pages(p2)
    pages2 = match_pages(toc, outl2)
    if pages2 != pages:
        print("page numbers moved after the TOC fill; third pass")
        doc, toc, nfig = assemble(pages2)
        render(doc, p2)
        outl2, n2 = outline_pages(p2)
        pages2 = match_pages(toc, outl2)
    finish(p2, a.out, toc, pages2)
    missing = [t for t in toc if t[2] not in pages2]
    print(f"pages: {n2}; toc entries: {len(toc)}; missing pages: {len(missing)} {missing[:5]}; wrote {a.out}")


if __name__ == "__main__":
    main()
