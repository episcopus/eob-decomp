/* NEGATIVE FIXTURE (make match-gate): src/E0CF7.C with one defect, a global annotated one byte off (ds:F3BA for ds:F3B9). The matcher must
   report a difference; a build error or a match fails the gate. */
/* Module at EOB.EXE 0CF7:0007 (591 bytes, the whole code segment): builds lists from tables of
   far pointers. Go/no-go spike for the toolchain (SPEC 4.2): Turbo C++ 1.00, -ml -Z -O.
   Names are placeholders until the callers and the tables are understood. */

struct Node { int a; int b; };

extern char gFlag; /*@ ds:F3BA */
extern struct Node far *gList; /*@ ds:FB48 */
extern void far *gTable1a[]; /*@ ds:5102 */
extern void far *gTable1b[]; /*@ ds:506A */
extern void far *gTable2a[]; /*@ ds:52DA */
extern void far *gTable2b[]; /*@ ds:51D2 */
extern void far *gTable3a[]; /*@ ds:528E */
extern void far *gTable3b[]; /*@ ds:53C6 */
extern void far *gTable4[]; /*@ ds:5446 */
extern void far *gTable5[]; /*@ ds:5462 */

extern struct Node far *ListAdd(struct Node far *list, void far *item); /*@ 1658:001E */

/*@ 0CF7:0007 */ void ListInit(struct Node far *head)
{
    gList = head;
    gList->a = 0;
    gList->b = 0;
}

/*@ 0CF7:0028 */ void BuildList1(void)
{
    int i;

    if (gFlag) {
        ListInit(gTable1a[0]);
        for (i = 1; i < 52; i++)
            gList = ListAdd(gList, gTable1a[i]);
    } else {
        ListInit(gTable1b[0]);
        for (i = 1; i < 38; i++)
            gList = ListAdd(gList, gTable1b[i]);
    }
}

/*@ 0CF7:00B4 */ void BuildList2(void)
{
    int i;

    if (gFlag) {
        ListInit(gTable2a[0]);
        for (i = 1; i < 59; i++)
            gList = ListAdd(gList, gTable2a[i]);
    } else {
        ListInit(gTable2b[0]);
        for (i = 1; i < 47; i++)
            gList = ListAdd(gList, gTable2b[i]);
    }
}

/*@ 0CF7:0140 */ void BuildList3(void)
{
    int i;

    if (gFlag) {
        ListInit(gTable3a[0]);
        for (i = 1; i < 19; i++)
            gList = ListAdd(gList, gTable3a[i]);
    } else {
        ListInit(gTable3b[0]);
        for (i = 1; i < 32; i++)
            gList = ListAdd(gList, gTable3b[i]);
    }
}

/*@ 0CF7:01CC */ void BuildList4(void)
{
    int i;

    ListInit(gTable4[0]);
    for (i = 1; i < 7; i++)
        gList = ListAdd(gList, gTable4[i]);
}

/*@ 0CF7:0211 */ void BuildList5(void)
{
    int i;

    ListInit(gTable5[0]);
    for (i = 1; i < 7; i++)
        gList = ListAdd(gList, gTable5[i]);
}
