/* NEGATIVE FIXTURE (make match-gate): src/E08E6.C with one defect, a near call to the wrong sibling stub (same bytes but the target). The matcher must
   report a difference; a build error or a match fails the gate. */
/* Unit at EOB.EXE 08E6:000B (6331 bytes, 36 functions); one function decompiled so far, its
   siblings are stubs. Placeholder names. */

struct Obj28 {              /* 28-byte records, 30 of them at ds:8A82 */
    char b0;
    signed char kind;       /* +1 */
    char pad2[4];
    char toggle;            /* +6 */
    char pad7;
    signed char type;       /* +8 */
    char pad9[5];
    int count;              /* +E */
    char padE[12];
};

extern struct Obj28 gObjs[]; /*@ ds:8A82 */
extern signed char gArg3237[]; /*@ ds:3237 */
extern signed char gArg3234[]; /*@ ds:3234 */

extern void f1414_0D63(void); /*@ 1414:0D63 */

/*@ 08E6:00C3 stub */ void Obj00C3(struct Obj28 far *o) {}
/*@ 08E6:03FE stub */ void Obj03FE(struct Obj28 far *o) {}
/*@ 08E6:0AB8 stub */ void Obj0AB8(struct Obj28 far *o) {}
/*@ 08E6:0B34 stub */ void Obj0B34(struct Obj28 far *o, int arg) {}
/*@ 08E6:0B94 stub */ void Obj0B94(struct Obj28 far *o, int arg) {}
/*@ 08E6:0CA6 stub */ void Obj0CA6(struct Obj28 far *o) {}

/* 08E6:0CC8: update every object of a kind; dispatch on its type through an 11-entry table */
/*@ 08E6:0CC8 */ void UpdateObjects(int kind)
{
    struct Obj28 far *o;
    int i;

    for (o = gObjs, i = 0; i < 30; i++, o++) {
        if (o->kind != kind || o->count <= 0)
            continue;
        Obj00C3(o);
        if (o->type > 0)
            Obj03FE(o);
        switch (o->type) {
        case 0:
            Obj0CA6(o);
            break;
        case 1:
        case 2:
        case 3:
            Obj0B34(o, gArg3237[o->type]);
            break;
        case 5:
        case 6:
            Obj0B94(o, gArg3234[o->type]);
            break;
        case 7:
        case 10:
            Obj0CA6(o);
            break;
        }
        if (o->type != 4 && o->type != 7)
            o->toggle ^= 1;
    }
    f1414_0D63();
}
