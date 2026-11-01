/* Unit at EOB.EXE 1A29:000F (470 bytes, 7 functions): thin wrappers around the image
   drawing primitives held in the graphics driver's far function pointer table, plus the
   routine that selects an image. Every wrapper adds the current image's origin offsets
   (ds:FE4A, ds:FE46) to the passed source coordinates and, when the requested image is
   not the current one, first calls the selector 1A29:0189. The selector reads the
   16-byte record of the image from the table at ds:009F (the same table 082C reads) and
   expands it into the globals ds:FE48/FE4A/FE4C/FE46/FE44 before calling ds:80B6.
   Placeholder names. Compiled with -G (the `inc sp; inc sp` clean-ups). */

/*@flags -ml -O -Z -G */

struct Image {              /* 16-byte records; only the first four words are used here */
    int w;
    int h;
    int dx;
    int dy;
    int pad0;
    int pad1;
    int pad2;
    int pad3;
};

extern struct Image gImages[];  /*@ ds:009F */
extern int gImage;              /*@ ds:8108 */
extern int gImgW;               /*@ ds:FE48 */
extern int gImgRow;             /*@ ds:FE4A */
extern int gImgX;               /*@ ds:FE4C */
extern int gImgH;               /*@ ds:FE46 */
extern int gImgY;               /*@ ds:FE44 */

extern void (far *gDraw80C2)(int a, void far *p, int x, int y);        /*@ ds:80C2 */
extern void (far *gDraw80A6)(int a, void far *p, int x, int y);        /*@ ds:80A6 */
extern void (far *gDraw80D6)(int a, void far *p, int x, int y);        /*@ ds:80D6 */
extern void (far *gDraw80DA)(int a, void far *p, int x, int y);        /*@ ds:80DA */
extern void (far *gDraw809A)(int a, void far *p, int x, int y, int z); /*@ ds:809A */
extern void (far *gDraw8096)(int a, void far *p, int x, int y, int z); /*@ ds:8096 */
extern void (far *gSetImage)(int w, int x, int h, int y);              /*@ ds:80B6 */

void f1A29_0189(int img);

/*@ 1A29:000F */ void f1A29_000F(int a, void far *p, int x, int y, int img)
{
    if (!p)
        return;
    if (img != gImage)
        f1A29_0189(img);
    gDraw80C2(a, p, x + gImgRow, y + gImgH);
}

/*@ 1A29:004D */ void f1A29_004D(int a, void far *p, int x, int y, int img)
{
    if (!p)
        return;
    if (img != gImage)
        f1A29_0189(img);
    gDraw80A6(a, p, x + gImgRow, y + gImgH);
}

/*@ 1A29:008B */ void f1A29_008B(int a, void far *p, int x, int y, int img)
{
    if (!p)
        return;
    if (img != gImage)
        f1A29_0189(img);
    gDraw80D6(a, p, x + gImgRow, y + gImgH);
}

/*@ 1A29:00C9 */ void f1A29_00C9(int a, void far *p, int x, int y, int img)
{
    if (!p)
        return;
    if (img != gImage)
        f1A29_0189(img);
    gDraw80DA(a, p, x + gImgRow, y + gImgH);
}

/*@ 1A29:0107 */ void f1A29_0107(int a, void far *p, int x, int y, int img, int z)
{
    if (!p)
        return;
    if (img != gImage)
        f1A29_0189(img);
    gDraw809A(a, p, x + gImgRow, y + gImgH, z);
}

/*@ 1A29:0148 */ void f1A29_0148(int a, void far *p, int x, int y, int img, int z)
{
    if (!p)
        return;
    if (img != gImage)
        f1A29_0189(img);
    gDraw8096(a, p, x + gImgRow, y + gImgH, z);
}

/*@ 1A29:0189 */ void f1A29_0189(int img)
{
    gImage = img;
    gImgW = gImages[img].w;
    gImgRow = gImgW * 8;
    gImgX = gImgW + gImages[img].dx;
    gImgH = gImages[img].h;
    gImgY = gImgH + gImages[img].dy;
    gSetImage(gImgW, gImgX, gImgH, gImgY);
}
