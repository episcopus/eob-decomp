/* NEGATIVE FIXTURE (make match-gate): src/E0EF3.C's CastBurningHands with one defect, the positive
   branch indexes gFlameShapes[n] where the original reads entry n - 1, a reference 4 bytes higher
   (the original's ds:FB34 base below the table's start, ds:FB38). The matcher must report a
   difference; a build error or a match fails the gate. */

/*@flags -ml -O -Z -v */

#include "EOB.H"

extern signed char gPanelMember;        /*@ ds:DF52 */
extern int gPartyDir;                       /*@ ds:FBCC */
extern int gPartyCell;                  /*@ ds:FBCA */
extern char gViewDirty;                      /*@ ds:DF4C */
extern signed char gBurningHandsShapes[];    /*@ ds:571B */
extern int gBurningHandsX[];                /*@ ds:5721 */
extern signed char gBurningHandsY[];        /*@ ds:572D */
extern void far *gFlameShapes[];        /*@ ds:FB38 */
extern void DrawShapeClipped(int page, void far *src, int x, int y, int region); /*@ 082C:063B */
extern void DrawShapeClippedFlipped(int page, void far *src, int x, int y, int region); /*@ 082C:0681 */
extern void DamageNearRowMonsters(int key, char count, char sides, char bonus, char flag); /*@ 08E6:16D6 */
extern void WaitTicks(int n); /*@ 082C:0425 */
extern int GetCasterLevel(int who); /*@ 0EF3:0498 */
extern int StepCell(int cell, int dir); /*@ 0D1C:00DA */

/*@ 0EF3:0F99 */ void CastBurningHands(void)
{
    int value;
    int i;
    int target;

    for (i = 0; i < 6; i++) {
        if (gBurningHandsShapes[i] < 0)
            DrawShapeClippedFlipped(0, gFlameShapes[(gBurningHandsShapes[i] < 0 ? -gBurningHandsShapes[i] : gBurningHandsShapes[i]) - 1],
                           gBurningHandsX[i], gBurningHandsY[i], 0);
        else
            DrawShapeClipped(0, gFlameShapes[gBurningHandsShapes[i]], gBurningHandsX[i], gBurningHandsY[i], 0);
    }
    WaitTicks(2);
    value = GetCasterLevel(gPanelMember);
    target = StepCell(gPartyCell, gPartyDir);
    DamageNearRowMonsters(target, 1, 3, value << 1, 1);
    gViewDirty = 1;
}
