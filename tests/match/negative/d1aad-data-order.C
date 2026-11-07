/* NEGATIVE FIXTURE (make match-gate): the data-only module d1aad (ds:802A-80EC, the graphics driver's slot
   table) written in C, as make data-skeleton drafts it: the same bytes and far pointers, but Turbo C writes
   a module's data fixups downward and the original's run upward, as TASM writes them (an assembled module).
   The matcher must report the relocation order; a build error or a match fails the gate. */
/*@data */
/* the data of d1aad, ds:802A to ds:80EC (194 bytes, 7 relocations): drafted by make data-skeleton from the original's bytes; each item at its annotation, in address order. */
/* what the pointers name that the file does not declare: */
extern char far *gCurFont; /*@ ds:FE4E */
extern int gDrawPage; /*@ ds:8115 */
extern unsigned gPageSegs[]; /*@ ds:FC0C */
extern void MemFree(void far *p); /*@ 19BC:0137 */
extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void far _GetFarPointer(void); /*@ 1A72:033A */
extern void far _SetFarPointer(void); /*@ 1A72:02FE */
void far *g802A[7] = { /*@ ds:802A */
    &gCurFont, gPageSegs, &gDrawPage, MemAlloc,
    MemFree, _GetFarPointer, _SetFarPointer
};
void (far *gCopyPage)() = 0; /*@ ds:8046 */
void (far *gRestoreRectToPage)(int x, int y, int w, int h, void far *buf) = 0; /*@ ds:804A */
void (far *gRestoreRect)(int x, int y, int width, int height, void far *buf) = 0; /*@ ds:804E */
void (far *gCopyRegion)(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2) = 0; /*@ ds:8052 */
void (far *gClearPage)(void) = 0; /*@ ds:8056 */
void (far *gDrawCell8x8)(void far *src, int x, int y) = 0; /*@ ds:805A */
void (far *gDrawIndexedTile)(int tile, int x, int y) = 0; /*@ ds:805E */
void (far *gDrawLine)(int x1, int y1, int x2, int y2, int colour) = 0; /*@ ds:8062 */
void (far *gXorRect)(int x1, int y1, int x2, int y2, int colour) = 0; /*@ ds:8066 */
void (far *gFillRect)(int x1, int y1, int x2, int y2, int colour) = 0; /*@ ds:806A */
unsigned char g806E[4] = { 0x00, 0x00, 0x00, 0x00 }; /*@ ds:806E */
int (far *gGetPixel)(int x, int y) = 0; /*@ ds:8072 */
unsigned char g8076[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; /*@ ds:8076 */
void (far *gSaveRectFromPage)(int x, int y, int w, int h, void far *buf) = 0; /*@ ds:807E */
void far * (far *gGetSpriteCellRle)(int x, int y, int w, int h, void far *buf) = 0; /*@ ds:8082 */
void far * (far *gGetSpriteCell)(int x, int y, int w, int h, void far *cgaMap) = 0; /*@ ds:8086 */
unsigned char g808A[4] = { 0x00, 0x00, 0x00, 0x00 }; /*@ ds:808A */
void (far *gCopyToPlanar)(void far *buf, int a, int b) = 0; /*@ ds:808E */
unsigned char g8092[4] = { 0x00, 0x00, 0x00, 0x00 }; /*@ ds:8092 */
void (far *gDrawShapeMaskedFlipped)(int page, void far *src, int x, int y, int depth) = 0; /*@ ds:8096 */
void (far *gDrawShapeMasked)(int page, void far *src, int x, int y, int depth) = 0; /*@ ds:809A */
void (far *gPutPixel)(int x, int y, int handle) = 0; /*@ ds:809E */
void far *gRestoreDrawState = 0; /*@ ds:80A2 */
void (far *gDrawShapeFlipped)(int page, void far *src, int x, int y) = 0; /*@ ds:80A6 */
void far *gSaveDrawState = 0; /*@ ds:80AA */
void (far *gSaveRect)(int x, int y, int w, int h, void far *buf) = 0; /*@ ds:80AE */
unsigned char g80B2[4] = { 0x00, 0x00, 0x00, 0x00 }; /*@ ds:80B2 */
void (far *gSetRegion)(int left, int right, int top, int bottom) = 0; /*@ ds:80B6 */
void (far *gSetPalette)(void far *table) = 0; /*@ ds:80BA */
unsigned char g80BE[4] = { 0x00, 0x00, 0x00, 0x00 }; /*@ ds:80BE */
void (far *gDrawShape)(int page, void far *src, int x, int y) = 0; /*@ ds:80C2 */
int (far *gSaveRectSize)(int width, int height) = 0; /*@ ds:80C6 */
void (far *gDrawText)(char far *text, int x, int y, int colour, int extra) = 0; /*@ ds:80CA */
unsigned gRowOffsetTableOff = 0x0000; /*@ ds:80CE */
unsigned gRowOffsetTableSeg = 0x0000; /*@ ds:80D0 */
void (far *gSetRemapTable)(void far *colors) = 0; /*@ ds:80D2 */
void (far *gDrawShapeRemapped)(int page, void far *src, int x, int y) = 0; /*@ ds:80D6 */
void (far *gDrawShapeFlippedRemapped)(int page, void far *src, int x, int y) = 0; /*@ ds:80DA */
unsigned char g80DE[4] = { 0x00, 0x00, 0x00, 0x00 }; /*@ ds:80DE */
void far *gRemapTable = 0; /*@ ds:80E2 */
int gSoundOn = 1; /*@ ds:80E6 */
int gTunesOn = 1; /*@ ds:80E8 */
int gKeyboardOptions = 272; /*@ ds:80EA */
