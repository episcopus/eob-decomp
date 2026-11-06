"""figlib: the diagram toolkit of the poolrad-decomp handbook (v0.5).

Every figure is an SVG with a viewBox 760 units wide (the text column); 1 unit is about 0.5 pt on the page.
Fonts: Inter (labels), DejaVu Sans Mono (code); sizes 10..14 units. No em or en dashes anywhere.

Usage:
    from figlib import *
    c = Canvas(760, 300)
    c.section(0, 0, "WHAT THE REBUILD COMPARES")
    c.box(0, 20, 200, 60, "teal", "make rebuild", ["compiles src/POOL.PAS", "compares segments"])
    c.arrow([(200, 50), (260, 50)])
    c.save("figs/f_rebuild.svg")
"""
import html, math

PAL = {
    # kind: (text, stroke, fill)
    "teal":  ("#14504A", "#1F6F66", "#DCEBE7"),
    "navy":  ("#2C386B", "#3B4A8A", "#E1E5F2"),
    "ochre": ("#7A5412", "#A8741A", "#F4E8CC"),
    "plum":  ("#5C2D50", "#7A3E6B", "#EFE0EB"),
    "red":   ("#80311F", "#A8452F", "#F5E0D9"),
    "grey":  ("#3C3A36", "#8A8373", "#ECE7DD"),
    "paper": ("#2B2A28", "#BDB5A2", "#FFFDF8"),
}
INK, INK2, MUTED, GREY = "#2B2A28", "#3C3A36", "#6E695F", "#8A8373"
RULE, PANEL, CARD = "#D9D2C3", "#F4F0E6", "#FFFDF8"
SOLID = {"teal": "#1F6F66", "navy": "#3B4A8A", "ochre": "#A8741A", "plum": "#7A3E6B", "red": "#A8452F", "grey": "#8A8373"}
MID = {"teal": "#A9D0C8", "navy": "#B9C2E0", "ochre": "#E3C98F", "plum": "#D5B8CC", "red": "#E3B2A5", "grey": "#D9D2C3"}

SANS = "Inter, 'Inter Display', sans-serif"
MONO = "'DejaVu Sans Mono', monospace"


def esc(s):
    s = str(s)
    assert "\u2014" not in s and "\u2013" not in s, "no em or en dashes: " + s
    return html.escape(s, quote=True)


def text_width(s, size=12, mono=False, weight=400):
    """Approximate rendered width in units."""
    if mono:
        return len(s) * size * 0.602
    w = 0.0
    for ch in s:
        if ch in "il.,:;'|!I()[]`":
            w += 0.28
        elif ch in "mwMW":
            w += 0.82
        elif ch.isupper() or ch.isdigit():
            w += 0.64
        elif ch == " ":
            w += 0.27
        else:
            w += 0.53
    return w * size * (1.04 if weight >= 600 else 1.0)


def wrap(s, width, size=12, mono=False, weight=400):
    """Wrap s into lines no wider than width units. Keeps explicit newlines."""
    out = []
    for para in str(s).split("\n"):
        words = para.split(" ")
        line = ""
        for w in words:
            t = (line + " " + w) if line else w
            if text_width(t, size, mono, weight) <= width or not line:
                line = t
            else:
                out.append(line)
                line = w
        out.append(line)
    return out


class Canvas:
    def __init__(self, w=760, h=400, bg=None):
        self.w, self.h = w, h
        self.items = []
        self.defs = set()
        self.bg = bg

    # ---------- primitives ----------
    def raw(self, s):
        self.items.append(s)

    def rect(self, x, y, w, h, fill="none", stroke="none", sw=1, rx=4, dash=None, opacity=None):
        d = f' stroke-dasharray="{dash}"' if dash else ""
        o = f' opacity="{opacity}"' if opacity is not None else ""
        self.items.append(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{rx}" '
                          f'fill="{fill}" stroke="{stroke}" stroke-width="{sw}"{d}{o}/>')

    def line(self, x1, y1, x2, y2, stroke=GREY, sw=1, dash=None):
        d = f' stroke-dasharray="{dash}"' if dash else ""
        self.items.append(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="{stroke}" stroke-width="{sw}"{d}/>')

    def circle(self, cx, cy, r, fill="none", stroke="none", sw=1):
        self.items.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r:.1f}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>')

    def path(self, d, fill="none", stroke=GREY, sw=1, dash=None):
        dd = f' stroke-dasharray="{dash}"' if dash else ""
        self.items.append(f'<path d="{d}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"{dd}/>')

    def text(self, x, y, s, size=12, weight=400, color=INK, anchor="start", mono=False, italic=False, spacing=None):
        fam = MONO if mono else SANS
        it = ' font-style="italic"' if italic else ""
        sp = f' letter-spacing="{spacing}"' if spacing else ""
        self.items.append(f'<text x="{x:.1f}" y="{y:.1f}" font-family="{fam}" font-size="{size}" font-weight="{weight}" '
                          f'fill="{color}" text-anchor="{anchor}"{it}{sp}>{esc(s)}</text>')

    def lines(self, x, y, lines, size=12, lh=None, **kw):
        lh = lh or size * 1.32
        for i, s in enumerate(lines):
            self.text(x, y + i * lh, s, size=size, **kw)
        return y + len(lines) * lh

    def para(self, x, y, s, width, size=12, lh=None, **kw):
        """Wrapped paragraph; y is the first baseline. Returns the baseline after the last line."""
        ls = wrap(s, width, size, kw.get("mono", False), kw.get("weight", 400))
        return self.lines(x, y, ls, size=size, lh=lh, **kw)

    # ---------- composites ----------
    def section(self, x, y, label, color=None, size=11):
        """Small spaced capitals heading, as in v0.4 ('WHAT THE ORACLE JUDGES'). y is the baseline."""
        self.text(x, y, label.upper(), size=size, weight=700, color=color or "#14504A", spacing=1.6)

    def box(self, x, y, w, h, kind="teal", title=None, body=None, size=12.5, tsize=14, dash=None,
            align="center", fill=None, mono_body=False, title_mono=False, sw=1.2, rx=6):
        """Rounded box with an optional bold title and wrapped body lines (list or str)."""
        tc, sc, fc = PAL[kind]
        self.rect(x, y, w, h, fill=fill or fc, stroke=sc, sw=sw, rx=rx, dash=dash)
        lines = []
        if body:
            if isinstance(body, str):
                body = [body]
            for b in body:
                lines += wrap(b, w - 16, size, mono_body)
        lh = size * 1.3
        th = tsize * 1.25 if title else 0
        total = th + len(lines) * lh
        cy = y + (h - total) / 2 + (tsize * 0.95 if title else size * 0.95)
        ax = {"center": x + w / 2, "left": x + 9}[align]
        anchor = {"center": "middle", "left": "start"}[align]
        if title:
            self.text(ax, cy, title, size=tsize, weight=700, color=tc, anchor=anchor, mono=title_mono)
            cy += th - (tsize * 0.95) + size * 0.95 + 1
        for i, l in enumerate(lines):
            self.text(ax, cy + i * lh, l, size=size, color=INK2 if kind != "paper" else INK2, anchor=anchor, mono=mono_body)
        return (x, y, w, h)

    def card(self, x, y, w, h, kind, header, rows, size=12, header_right=None):
        """A card with a solid coloured header band (like the phase cards of v0.4) and text rows."""
        tc, sc, fc = PAL[kind]
        self.rect(x, y, w, h, fill=fc, stroke=sc, sw=1.2, rx=6)
        self.path(f"M{x},{y+20} L{x},{y+6} Q{x},{y} {x+6},{y} L{x+w-6},{y} Q{x+w},{y} {x+w},{y+6} L{x+w},{y+20} Z",
                  fill=SOLID[kind], stroke=SOLID[kind], sw=1.2)
        self.text(x + 8, y + 14, header, size=11.5, weight=700, color="#FFFFFF")
        if header_right:
            self.text(x + w - 8, y + 14, header_right, size=11, weight=600, color="#FFFFFF", anchor="end")
        yy = y + 36
        for r in rows:
            for l in wrap(r, w - 18, size):
                self.text(x + 9, yy, l, size=size, color=INK2)
                yy += size * 1.3
        return yy

    def pill(self, x, y, s, kind="teal", size=11, solid=False, mono=False):
        """A rounded tag; (x, y) is its left edge and vertical centre. Returns its right edge."""
        tc, sc, fc = PAL[kind]
        w = text_width(s, size, mono, 600) + 14
        self.rect(x, y - size * 0.85, w, size * 1.7, fill=SOLID[kind] if solid else fc, stroke=sc, sw=1, rx=size * 0.85)
        self.text(x + w / 2, y + size * 0.36, s, size=size, weight=600, color="#FFFFFF" if solid else tc,
                  anchor="middle", mono=mono)
        return x + w

    def badge(self, cx, cy, s, kind="teal", r=8, size=10):
        self.circle(cx, cy, r, fill=SOLID[kind])
        self.text(cx, cy + size * 0.36, s, size=size, weight=700, color="#FFFFFF", anchor="middle")

    def _marker(self, color):
        mid = "a" + color.strip("#")
        self.defs.add(f'<marker id="{mid}" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" '
                      f'orient="auto-start-reverse"><path d="M0,1 L9,5 L0,9 z" fill="{color}"/></marker>')
        return mid

    def arrow(self, pts, color=GREY, sw=1.3, dash=None, both=False, label=None, lsize=10.5, lpos=0.5, ldx=0, ldy=-5,
              lcolor=None, curve=False):
        """Polyline arrow through pts [(x,y),...]; arrow head at the end (both=True: both ends)."""
        mid = self._marker(color)
        if curve and len(pts) == 2:
            (x1, y1), (x2, y2) = pts
            d = f"M{x1:.1f},{y1:.1f} C{x1:.1f},{(y1+y2)/2:.1f} {x2:.1f},{(y1+y2)/2:.1f} {x2:.1f},{y2:.1f}"
        else:
            d = "M" + " L".join(f"{x:.1f},{y:.1f}" for x, y in pts)
        dd = f' stroke-dasharray="{dash}"' if dash else ""
        ms = f' marker-start="url(#{mid})"' if both else ""
        self.items.append(f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{sw}"{dd} marker-end="url(#{mid})"{ms}/>')
        if label:
            # place at fraction lpos along the total length
            segs = list(zip(pts, pts[1:]))
            L = sum(math.dist(a, b) for a, b in segs)
            t = L * lpos
            for a, b in segs:
                d0 = math.dist(a, b)
                if t <= d0 or (a, b) == segs[-1]:
                    f = t / d0 if d0 else 0
                    lx, ly = a[0] + (b[0] - a[0]) * f, a[1] + (b[1] - a[1]) * f
                    break
                t -= d0
            self.text(lx + ldx, ly + ldy, label, size=lsize, color=lcolor or MUTED, anchor="middle")

    def hbar(self, x, y, w, h, frac, kind="teal", track=True, label=None, lsize=10.5):
        if track:
            self.rect(x, y, w, h, fill="#ECE7DD", rx=h / 2)
        if frac > 0:
            self.rect(x, y, max(h, w * frac), h, fill=SOLID[kind], rx=h / 2)
        if label:
            self.text(x + 6, y + h / 2 + lsize * 0.36, label, size=lsize, weight=600, color="#FFFFFF")

    def check(self, x, y, ok=True, kind="teal", size=9):
        """A small tick box (v0.4 phase cards); (x, y) is the top-left."""
        self.rect(x, y, size, size, fill=CARD if not ok else SOLID[kind], stroke=SOLID[kind], sw=1, rx=2)
        if ok:
            self.path(f"M{x+2},{y+size*0.55} L{x+size*0.42},{y+size-2} L{x+size-1.8},{y+2}", stroke="#FFFFFF", sw=1.4)

    def legend(self, x, y, items, size=10.5):
        """items: [(kind, label, dashed?)]; returns right edge."""
        for it in items:
            kind, label = it[0], it[1]
            dash = it[2] if len(it) > 2 else None
            tc, sc, fc = PAL[kind]
            self.rect(x, y - 8, 12, 10, fill=fc, stroke=sc, sw=1, rx=2, dash=dash)
            self.text(x + 16, y + 0.5, label, size=size, color=MUTED)
            x += 16 + text_width(label, size) + 16
        return x

    def panel(self, x, y, w, h, fill=PANEL, stroke=None, rx=8, dash=None):
        self.rect(x, y, w, h, fill=fill, stroke=stroke or "none", sw=1, rx=rx, dash=dash)

    # ---------- output ----------
    def svg(self):
        defs = "<defs>" + "".join(sorted(self.defs)) + "</defs>" if self.defs else ""
        bg = f'<rect x="0" y="0" width="{self.w}" height="{self.h}" fill="{self.bg}"/>' if self.bg else ""
        body = "\n".join(self.items)
        return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {self.w} {self.h:.0f}" '
                f'width="{self.w}" height="{self.h:.0f}" class="fig">{defs}{bg}\n{body}\n</svg>')

    def save(self, path):
        s = self.svg()
        assert "\u2014" not in s and "\u2013" not in s
        with open(path, "w") as f:
            f.write(s)
        return path


def preview(svg_paths, out_png, width=1100):
    """Render SVG files to one PNG (stacked) with Chromium, at about print size x2, for checking."""
    import os
    from playwright.sync_api import sync_playwright
    parts = []
    for p in svg_paths:
        parts.append(f'<div style="margin:0 0 18px 0"><div style="font:11px monospace;color:#a00">{os.path.basename(p)}</div>'
                     f'<img src="file://{os.path.abspath(p)}" style="width:{width-40}px;display:block;background:#FAF7F0;border:1px solid #ddd"></div>')
    h = f'<html><body style="margin:20px;background:#fff">{"".join(parts)}</body></html>'
    tmp = os.path.abspath(out_png + ".html")
    open(tmp, "w").write(h)
    with sync_playwright() as pw:
        b = pw.chromium.launch()
        pg = b.new_page(viewport={"width": width, "height": 800}, device_scale_factor=1)
        pg.goto("file://" + tmp)
        pg.wait_for_timeout(300)
        pg.screenshot(path=out_png, full_page=True)
        b.close()
    return out_png
