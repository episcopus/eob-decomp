#!/usr/bin/env python3
"""preview.py: render figure SVGs to one PNG (stacked, about print size x2) to look at them.

    python3 tools/handbook/preview.py OUT.png a.svg b.svg ...
"""
import os, sys, subprocess, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
FONTS = os.environ.get("HANDBOOK_FONTS", os.path.join(ROOT, "build", "handbook", "fonts"))
NODE_MODULES = os.environ.get("HANDBOOK_NODE_MODULES", "~/.npm/_npx/e41f203b7505f1fb/node_modules")


def main():
    out, svgs = sys.argv[1], sys.argv[2:]
    css = open(os.path.join(HERE, "fonts.css")).read().replace("FONTDIR", "file://" + FONTS)
    parts = []
    for p in svgs:
        parts.append(f'<div style="margin:0 0 18px"><div style="font:11px monospace;color:#a00">{os.path.basename(p)}</div>'
                     f'<div class="art" style="width:1060px;background:#FAF7F0;border:1px solid #ddd">{open(p).read()}</div></div>')
    html = f'<html><head><style>{css} .art svg{{width:100%;height:auto;display:block}}</style></head><body style="margin:20px;background:#fff">{"".join(parts)}</body></html>'
    tmp = out + ".html"
    open(tmp, "w").write(html)
    env = dict(os.environ, HANDBOOK_NODE_MODULES=NODE_MODULES)
    subprocess.run(["node", os.path.join(HERE, "shot.mjs"), "png", tmp, out, "1100"], check=True, env=env)
    os.remove(tmp)
    print(out)


main()
