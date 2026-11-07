/* Module d1b82 of EOB.EXE: data only, ds:8104 to ds:8176 (114 bytes, 4 relocations), and the
   empty code segment at 1B82:0009 (TLINK's segment table lists it). The file error handlers
   (gFatalHandler, gRetryHandler) and the allocation failure hook as the image starts them, the
   text output state (gScrollOn, gDrawPage, gScrollHook, the two text attributes, gTextRegion), the
   overlay request RunOverlayRequest runs, then the mouse and pointer state the INT 33h glue keeps
   (src/E1A72.ASM). Compiled: its data relocations fall in the relocation table, as Turbo C writes
   a module's fixups (TASM writes them upward; playbook 80, step D). Drafted by make data-skeleton;
   the names other files of src/ declare are used, the rest are named by their address until a
   later round names them. */

#include "EOB.H"

/*@data */
/* what the pointers name that the file does not declare before them: */
extern unsigned gHitTestY; /*@ ds:8028 */
extern int far DisplayFileError(int code, char far *name); /*@ 1C0E:0ADD */
extern int far OutOfMemory(void); /*@ 19BC:02B6 */
extern int far ShowFileError(int code, char far *name); /*@ 1C0E:0D9B */

/* ShowFileError returns int (src/E1C0E.C:665); E1C0E.C:70 declares the handler void. */
void (far *gFatalHandler)(int code, char far *name) = (void (far *)())ShowFileError; /*@ ds:8104 */
int gCurRegion = -1; /*@ ds:8108 */
int (far *gAllocFailHook)(void) = OutOfMemory; /*@ ds:810A */
unsigned gAllocCount = 0; /*@ ds:810E */
char gScrollOn = 1; /*@ ds:8110 */
int (far *gRetryHandler)(int mode, char far *name) = DisplayFileError; /*@ ds:8111 */
int gDrawPage = 0; /*@ ds:8115 */
/* src/E1DE7.C declares ds:8117 a far function pointer and calls it; EOV5.C writes it as two words,
   gScrollHook the offset and gScrollHookSeg the segment, so both names are defined here. */
int gScrollHook = 0; /*@ ds:8117 */
int gScrollHookSeg = 0; /*@ ds:8119 */
int gTextAttr2 = 12; /*@ ds:811B */
int gTextAttr1 = 15; /*@ ds:811D */
int gTextRegion = 0; /*@ ds:811F */
/* table: ds:802A, the driver slot table of d1aad (src/D1AAD.ASM), which names it by a label
   only, so it is reached here as the word after gHitTestY (ds:8028). */
struct OverlayRequest gOverlayRequest = { 0, (char far *)&gHitTestY + 2 }; /*@ ds:8121 */
unsigned char g8129[1] = { 0x00 }; /*@ ds:8129 */
unsigned gBufSeg = 0; /*@ ds:812A */
unsigned gBufSize = 0; /*@ ds:812C */
unsigned gPointerBusy = 0; /*@ ds:812E */
unsigned gLastMouseButtons = 0; /*@ ds:8130 */
unsigned gMiddleButtonBits = 2; /*@ ds:8132 */
int gUnreadButtonChanges = 0; /*@ ds:8134 */
unsigned gMouseButtonSeen = 0; /*@ ds:8136 */
int gCursorX = 0; /*@ ds:8138 */
int gCursorY = 0; /*@ ds:813A */
unsigned gMouseEventX = 0; /*@ ds:813C */
unsigned gMouseEventY = 0; /*@ ds:813E */
int gClickX = 0; /*@ ds:8140 */
int gClickY = 0; /*@ ds:8142 */
unsigned gMouseXDoubled = 0; /*@ ds:8144 */
int gHideDepth = 0; /*@ ds:8146 */
int gMouseHandlerInstalled = 0; /*@ ds:8148 */
int gMouseDisabled = 0; /*@ ds:814A */
unsigned char g814C[2] = { 0x00, 0x00 }; /*@ ds:814C */
unsigned gCursorMaxX = 0; /*@ ds:814E */
unsigned char g8150[2] = { 0x00, 0x00 }; /*@ ds:8150 */
unsigned gCursorMaxY = 0; /*@ ds:8152 */
int gPointerHeight = 0; /*@ ds:8154 */
int gPointerWidth8 = 0; /*@ ds:8156 */
unsigned gPointerHotX = 0; /*@ ds:8158 */
unsigned gPointerHotY = 0; /*@ ds:815A */
unsigned gDrawnX = 0; /*@ ds:815C */
unsigned gDrawnY = 0; /*@ ds:815E */
unsigned gPointerState = 0; /*@ ds:8160 */
unsigned gZoneLeft = 0; /*@ ds:8162 */
unsigned gZoneTop = 0; /*@ ds:8164 */
unsigned gZoneRight = 0; /*@ ds:8166 */
unsigned gZoneBottom = 0; /*@ ds:8168 */
unsigned gPointerBackOff = 0; /*@ ds:816A */
unsigned gPointerBackSeg = 0; /*@ ds:816C */
unsigned gPointerShapeOff = 0; /*@ ds:816E */
unsigned gPointerShapeSeg = 0; /*@ ds:8170 */
unsigned gEgaRegShadowOff = 0; /*@ ds:8172 */
unsigned gEgaRegShadowSeg = 0; /*@ ds:8174 */
