// shot.mjs: render with the Chromium that Playwright (node) already has.
//   node shot.mjs png  <in.html> <out.png> [width]    full-page screenshot
//   node shot.mjs pdf  <in.html> <out.pdf>            the page's own @page size, outline and tags on
// Playwright is found through HANDBOOK_NODE_MODULES (a node_modules directory holding playwright).
import { createRequire } from 'module';
import path from 'path';
const nm = process.env.HANDBOOK_NODE_MODULES;
if (!nm) { console.error('set HANDBOOK_NODE_MODULES to a node_modules directory with playwright'); process.exit(2); }
const { chromium } = createRequire(path.join(nm, 'x.js'))('playwright');
const [mode, input, output, width] = process.argv.slice(2);
const b = await chromium.launch();
const pg = await b.newPage({ viewport: { width: parseInt(width || '1100'), height: 800 } });
await pg.goto('file://' + path.resolve(input), { waitUntil: 'load' });
await pg.evaluate(() => document.fonts.ready);
await pg.waitForTimeout(800);
if (mode === 'png') await pg.screenshot({ path: output, fullPage: true });
else await pg.pdf({ path: output, preferCSSPageSize: true, printBackground: true, outline: true, tagged: true });
await b.close();
