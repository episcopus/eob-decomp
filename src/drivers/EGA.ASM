; EGA.OVL: the graphics driver for the EGA choice, 320 by 200 pixels in 16 colours (BIOS mode 0Dh), planar: the
; colour of a pixel is one bit in each of four bit planes at A000h, eight pixels a byte (leftmost in bit 7), 40
; bytes a line in each plane. The game loads and links it as it does MCGA.OVL (whitepaper 2.5): the same entry
; jump, the same slot table of seven imports and forty exports (cs:0003), the same calls. The pages the draws
; write are screen pages, 8000 bytes in each plane (ClearPage clears 4000 words through all four planes),
; 200h paragraphs apart (SwapPageWithScreen shows page segment s as display page (s SHR 9) AND 7); the art pages
; CopyPage and GetSpriteCell read hold one pixel a byte, 320 a line, as on the other drivers.
;
; The registers. Every write to the sequencer (port 3C4h) or the graphics controller (3CEh) goes through a
; shadow: the value into its copy at cs:00C1 (the two index registers, the five sequencer and the nine graphics
; controller registers, 16 bytes), then out, between cli and sti but in SaveDrawState and RestoreDrawState, so
; the copy always holds what the hardware holds. LinkDriver hands
; the game a far pointer to the copy (SetFarPointer, import slot 6; the game's gEgaRegShadowOff, ds:8172), the
; only driver that calls that import. The game passes the pointer to SaveDrawState and calls RestoreDrawState
; after (its pointer redraws, 1BBE): SaveDrawState keeps the copy at cs:00D1, stores the latches at A000:FFFF
; and sets the state the draws expect; RestoreDrawState loads the latches back and writes every register whose
; value differs, so a pointer drawn in the middle of a draw leaves that draw's registers as they were.
;
; Between calls the graphics controller rests in write mode 2 (SetWriteMode2), its index at the bit mask (8),
; the map mask at 0Fh: a byte written to the screen is a colour, and the bit mask, one out to port 3CFh, picks
; the pixels it goes to. The draws read the byte before they write it (the read loads the latches, so the pixels
; outside the mask keep their colours). Routines that change the mode or the index put this state back.
;
; Cells. GetSpriteCell (exports 15 and 16) cuts 16-colour art into a format for this mode: a 4-byte header (0,
; rows, cols, rows), then for each 8-pixel group of each row, one (mask, colour) pair per colour 1 to 15 present
; in the group (mask the bits of its pixels), or (0, 0) for a group that is all colour 0; the last colour byte of
; a group has bit 7 set, of a row bit 6, of the cell bit 5. DrawShape and its flipped and remapped forms draw
; that format, a bit mask write and a colour write per pair.
;
; Seven exports do nothing here: DrawIndexedTile, MirrorCell16, SetTileSet, DrawMaskedCell16, DrawShapeMasked
; and its mirror, and SetTileFormat return at once (push bp, mov bp,sp, pop bp, retf), with no body after them.
; The other routines are MCGA.OVL's for planes, at other offsets; the comments say where they differ. Routines
; are far (retf) unless their comment says near. TASM 2.0 and TLINK 3.0 build the whole file from this source
; (make rebuild PROGRAM=EGA.OVL).

SLOT_COUNT	equ	47		; slots in the table at cs:0003: 7 imports and 40 exports
SCREEN_WIDTH	equ	320		; pixels in a line, and bytes in a line of an art page
SCREEN_HEIGHT	equ	200
SCREEN_COLS	equ	40		; 8-pixel columns in a line, and bytes in a line of a plane
PLANE_BYTES	equ	8000		; bytes of a page in each plane
FONT_HEIGHT	equ	102h		; the font's byte offsets: its height in lines
FONT_WIDTH	equ	103h		; and its width in pixels
VIDEO_SEG	equ	0A000h		; the EGA's video segment, page 0
LATCH_SAVE	equ	0FFFFh		; the last byte of the video segment, past every page: where the latches are kept
SEQ_INDEX	equ	3C4h		; the sequencer's index port (data at 3C5h)
GC_INDEX	equ	3CEh		; the graphics controller's index port
GC_DATA		equ	3CFh		; and its data port
SEQ_RESET	equ	0		; the sequencer's registers
SEQ_CLOCKING	equ	1
SEQ_MAP_MASK	equ	2		; the planes a write goes to
SEQ_CHAR_MAP	equ	3
SEQ_MEMORY_MODE	equ	4
GC_SET_RESET	equ	0		; the graphics controller's registers
GC_ENABLE_SET_RESET	equ	1
GC_COLOUR_COMPARE	equ	2
GC_DATA_ROTATE	equ	3		; the rotate count and the function (18h: XOR) of a write
GC_READ_MAP	equ	4		; the plane a read comes from
GC_MODE		equ	5		; the write mode (bits 0 and 1)
GC_MISC		equ	6
GC_COLOUR_DONT_CARE	equ	7
GC_BIT_MASK	equ	8		; the pixels of the byte a write changes
ROTATE_XOR	equ	18h		; GC_DATA_ROTATE: XOR the written colour into the latches

DRIVER	SEGMENT BYTE PUBLIC 'CODE'
	ASSUME	cs:DRIVER, ds:NOTHING, es:NOTHING, ss:NOTHING

; EGA.OVL 0000: the image starts with a jump to DriverEntry; the game calls segment:0000 far after loading
Start	PROC	NEAR
	jmp	DriverEntry
Start	ENDP

; cs:0003: the slot table LinkDriver exchanges with the game's table at ds:802A, slot for slot. The
; first seven are zero here: the game's table holds its own far pointers there, and LinkDriver copies
; them in (the imports). The forty after them are the driver's exports: the game's slots are zero, and
; LinkDriver writes these pointers into them, so export i is the game's far pointer at ds:8046 + 4 * i.
gCurFontPtr	dd	0			; slot 0, ds:802A: ds:FE4E, the far pointer to the current font (gCurFont)
gPageSegsPtr	dd	0			; slot 1, ds:802E: ds:FC0C, the eight page segments (gPageSegs)
gDrawPagePtr	dd	0			; slot 2, ds:8032: ds:8115, the page the driver draws to (gDrawPage)
gMemAllocPtr	dd	0			; slot 3, ds:8036: 19BC:0007 MemAlloc(size low, size high, flags), dx:ax the block
gMemFreePtr	dd	0			; slot 4, ds:803A: 19BC:0137 MemFree
gGetFarPointerPtr	dd	0			; slot 5, ds:803E: 1A72:033A GetFarPointer
gSetFarPointerPtr	dd	0			; slot 6, ds:8042: 1A72:02FE SetFarPointer
gExports LABEL DWORD
	dd	CopyPage		; export 0, slot 7, ds:8046 gCopyPage
	dd	RestoreRectToPage		; export 1, slot 8, ds:804A gRestoreRectToPage
	dd	RestoreRect		; export 2, slot 9, ds:804E gRestoreRect
	dd	CopyRegion		; export 3, slot 10, ds:8052 gCopyRegion
	dd	ClearPage		; export 4, slot 11, ds:8056 gClearPage
	dd	DrawCell8x8		; export 5, slot 12, ds:805A gDrawCell8x8
	dd	DrawIndexedTile		; export 6, slot 13, ds:805E gDrawIndexedTile
	dd	DrawLine		; export 7, slot 14, ds:8062 gDrawLine
	dd	XorRect		; export 8, slot 15, ds:8066 gXorRect
	dd	FillRect		; export 9, slot 16, ds:806A gFillRect
	dd	ReadCell8x8		; export 10, slot 17, ds:806E
	dd	GetPixel		; export 11, slot 18, ds:8072 gGetPixel
	dd	MirrorCell16		; export 12, slot 19, ds:8076
	dd	SetTileSet		; export 13, slot 20, ds:807A
	dd	SaveRectFromPage		; export 14, slot 21, ds:807E gSaveRectFromPage
	dd	GetSpriteCell		; export 15, slot 22, ds:8082 gGetSpriteCellRle
	dd	GetSpriteCell		; export 16, slot 23, ds:8086 gGetSpriteCell
	dd	DrawMaskedCell16		; export 17, slot 24, ds:808A
	dd	CopyToPlanar		; export 18, slot 25, ds:808E gCopyToPlanar
	dd	SwapPageWithScreen		; export 19, slot 26, ds:8092
	dd	DrawShapeMaskedFlipped		; export 20, slot 27, ds:8096 gDrawShapeMaskedFlipped
	dd	DrawShapeMasked		; export 21, slot 28, ds:809A gDrawShapeMasked
	dd	PutPixel		; export 22, slot 29, ds:809E gPutPixel
	dd	RestoreDrawState		; export 23, slot 30, ds:80A2 gRestoreDrawState
	dd	DrawShapeFlipped		; export 24, slot 31, ds:80A6 gDrawShapeFlipped
	dd	SaveDrawState		; export 25, slot 32, ds:80AA gSaveDrawState
	dd	SaveRect		; export 26, slot 33, ds:80AE gSaveRect
	dd	SetLineClip		; export 27, slot 34, ds:80B2
	dd	SetRegion		; export 28, slot 35, ds:80B6 gSetRegion
	dd	SetPalette		; export 29, slot 36, ds:80BA gSetPalette
	dd	SetTileFormat		; export 30, slot 37, ds:80BE
	dd	DrawShape		; export 31, slot 38, ds:80C2 gDrawShape
	dd	SaveRectSize		; export 32, slot 39, ds:80C6 gSaveRectSize
	dd	DrawText		; export 33, slot 40, ds:80CA gDrawText
	dd	gRowOffsets		; export 34, slot 41, ds:80CE gRowOffsetTableOff
	dd	SetRemapTable		; export 35, slot 42, ds:80D2 gSetRemapTable
	dd	DrawShapeRemapped		; export 36, slot 43, ds:80D6 gDrawShapeRemapped
	dd	DrawShapeFlippedRemapped		; export 37, slot 44, ds:80DA gDrawShapeFlippedRemapped
	dd	gMirrorBits		; export 38, slot 45, ds:80DE
	dd	gRemapTable		; export 39, slot 46, ds:80E2 gRemapTable
gSlotCount	dw	SLOT_COUNT		; the slots LinkDriver walks

; cs:00C1: the register shadow, what the sequencer and graphics controller hold (each write to them goes
; through it); LinkDriver gives the game its far pointer (SetFarPointer), and the game passes it to SaveDrawState
gSeqIndex	db	0		; the sequencer's index (3C4h), then its five registers
gSeqReset	db	0
gSeqClocking	db	0
gMapMask	db	0
gSeqCharMap	db	0
gSeqMemoryMode	db	0
gGcIndex	db	0		; the graphics controller's index (3CEh), then its nine registers
gSetReset	db	0
gEnableSetReset	db	0
gColourCompare	db	0
gDataRotate	db	0
gReadMap	db	0
gGcMode	db	0
gGcMisc	db	0
gColourDontCare	db	0
gBitMask	db	0

; cs:00D1: the shadow as SaveDrawState found it, which RestoreDrawState writes back
gSavedSeqIndex	db	0		; the same 16 bytes
gSavedSeqReset	db	0
gSavedSeqClocking	db	0
gSavedMapMask	db	0
gSavedSeqCharMap	db	0
gSavedSeqMemoryMode	db	0
gSavedGcIndex	db	0		; the graphics controller's index (3CEh), then its nine registers
gSavedSetReset	db	0
gSavedEnableSetReset	db	0
gSavedColourCompare	db	0
gSavedDataRotate	db	0
gSavedReadMap	db	0
gSavedGcMode	db	0
gSavedGcMisc	db	0
gSavedColourDontCare	db	0
gSavedBitMask	db	0

; cs:00E1: SaveDrawState's region, DrawGlyph's and FillRect's state
gSavedLeft	dw	0		; SaveDrawState keeps the region here (in columns and lines)
gSavedRight	dw	0
gSavedTop	dw	0
gSavedBottom	dw	0
gGlyphRows	db	0		; DrawGlyph: lines of the glyph left
gGlyphWidth	db	0		; and its width in pixels
gTextRight	dw	0		; DrawText: the x a character may not pass (SCREEN_WIDTH)
gSpanLeft	dw	0		; FillRect: the span's first and last x on the current line
gSpanRight	dw	0

; cs:00F1: FillRect's masks by x AND 7: the bits from pixel x to the right end of its byte, then the same
; with only the even or the odd bits of a checkerboard (not read); the bits from the left end to pixel x,
; and their checkerboard halves (not read)
gLeftMasks	db	0FFh, 7Fh, 3Fh, 1Fh, 0Fh, 7, 3, 1		; 0FFh SHR (x AND 7)
gLeftMasksEven	db	55h, 55h, 15h, 15h, 5, 5, 1, 1		; AND 55h
gLeftMasksOdd	db	0AAh, 2Ah, 2Ah, 0Ah, 0Ah, 2, 2, 0		; AND 0AAh
gRightMasks	db	80h, 0C0h, 0E0h, 0F0h, 0F8h, 0FCh, 0FEh, 0FFh		; 0FF80h SHR (x AND 7)
gRightMasksEven	db	0, 40h, 40h, 50h, 50h, 54h, 54h, 55h		; AND 55h
gRightMasksOdd	db	80h, 80h, 0A0h, 0A0h, 0A8h, 0A8h, 0AAh, 0AAh		; AND 0AAh

; cs:0121: XorRect's, CopyRegion's, DrawCell8x8's, FillRect's, SetLineClip's, GetSpriteCell's and the
; shape draws' state
gXorLeftMask	db	0		; XorRect: x AND 7 of the left edge, then the mask of the first byte
gXorRightMask	db	0		; the mask of the last byte, 0 when the first is the only partial one
gXorFirstPixels	dw	0		; the pixels in the first byte (up to 8)
gXorMiddleBytes	db	0		; the whole bytes after it
gXorRows	db	0
gXorTop	db	0		; the top line
gXorColour	db	0		; the colour, AND 0Fh
gXorRowSkip	dw	0		; screen bytes from a row's end to the next row
	db	2 dup (0)		; 012B: not used
gCopySrcSeg	dw	0		; CopyRegion: the source page's segment
gCopySrcCol	dw	0
gCopyDstCol	dw	0
gCopySrcY	dw	0
gCopyDstY	dw	0
gCopyCols	dw	0
gCopyRows	dw	0
gCopySrcOff	dw	0		; the source offset
	db	2 dup (0)		; 013D: not used
gCell8x8Seg	dw	0		; DrawCell8x8: the source segment
gFillRowOff	dw	0		; FillRect: the offset of the current line
gFillRows	dw	0		; lines left
gFillColour	db	0		; the colour, AND 0Fh
gFillLeft	dw	0		; the clamped x of the two sides
gFillRight	dw	0
gLineClipLeft	dw	0		; the line clip rectangle, in pixels (SetLineClip)
gLineClipTop	dw	0
gLineClipRight	dw	13Fh
gLineClipBottom	dw	0C7h
gCellSrcSeg	dw	0		; GetSpriteCell: the art page's segment
gCellDataStart	dw	0		; the cell's first byte after the header
gCellRowsLeft	db	0		; rows left
gCellCols	dw	0		; 8-pixel groups in a row
gCellColsLeft	db	0		; groups left in the row
gColourBits	db	16 dup (0)		; for each colour, the bits of the group's pixels in that colour
gPixelHigh	db	0		; the high pixel of a pair being packed
gCellSizing	db	0		; 1 while the first pass measures the cell, 0 while the second writes it
gCellCol	dw	0		; the arguments: column, line, columns, rows
gCellY	dw	0
gCellColsArg	dw	0
gCellRowsArg	dw	0
gCellSrcSkip	dw	0		; art bytes from a row's end to the next row
gRegionLeft	dw	0		; the drawing region SetRegion sets: left and right in 8-pixel columns, top and bottom in lines
gRegionRight	dw	0
gRegionTop	dw	0
gRegionBottom	dw	0
gShapeSeg	dw	0		; DrawShapeRemapped: the shape's segment
gShapeCol	dw	0		; x SHR 3, the column
gShapeX	dw	0		; x
gShapeClipLeft	dw	0		; gRegionLeft - 1 and gRegionRight - 1, the columns drawn are above the one and up to the other
gShapeClipRight	dw	0
gShapeEnd	dw	0		; the offset of the region's bottom line: rows stop there
	db	2 dup (0)		; 018A: not used
gShapeRows	db	0		; rows left
gShapeRowStart	dw	0		; the offset of the row being drawn

; cs:018F (export 39, the game's gRemapTable): SetRemapTable's 16 colours, and the same with each of the flag
; nibbles 10h to F0h: the remapped draws translate a cell's colour byte through it, keeping its flag bits
gRemapTable	db	256 dup (0)

; cs:028F: DrawShapeFlipped's and DrawShapeFlippedRemapped's state, as DrawShapeRemapped's
gFlipSeg	dw	0
gFlipCol	dw	0
gFlipX	dw	0
gFlipCols	db	0		; the shape's columns: it is drawn from its right end
gFlipClipLeft	dw	0
gFlipClipRight	dw	0
gFlipEnd	dw	0		; the offset of the region's bottom line
gFlipBottom	dw	0
gFlipTop	dw	0
gFlipRows	db	0		; rows left

; cs:02A1 (export 38): each byte with its eight bits in the other order, the pixels of a mask mirrored, for
; the flipped draws
gMirrorBits LABEL BYTE
Bits = 0
Mirrored = 0
	REPT	256
	db	Mirrored
Bits = Bits + 1
Mirrored = ((Bits AND 1) SHL 7) OR ((Bits AND 2) SHL 5) OR ((Bits AND 4) SHL 3) OR ((Bits AND 8) SHL 1)
Mirrored = Mirrored OR ((Bits AND 10h) SHR 1) OR ((Bits AND 20h) SHR 3) OR ((Bits AND 40h) SHR 5) OR (Bits SHR 7)
	ENDM

; cs:03A1: the bit of pixel x in its byte, by x AND 7 (GetPixel, PutPixel)
gPixelMasks	db	80h, 40h, 20h, 10h, 8, 4, 2, 1

; cs:03A9 (export 34, the game's gRowOffsetTableOff): the offset of each screen line in a plane, y * 40, for
; y = 0 to 200 (one past the last line)
gRowOffsets LABEL WORD
RowOffset = 0
	REPT	SCREEN_HEIGHT + 1
	dw	RowOffset
RowOffset = RowOffset + SCREEN_COLS
	ENDM

; EGA.OVL 053B (export 0, gCopyPage): CopyPage(srcSeg, dstSeg) turns a 64000-byte art page (one 16-colour pixel
; a byte) into a screen page: for each screen byte, the bit mask takes each of its eight bits in turn and the
; pixel's byte is written to it in write mode 2. The CGA colour map the game passes after the two segments is
; not read.
CopyPage	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	di
	push	ds
	push	dx
	push	si
	push	es
	mov	ds, word ptr [bp + 6]
	mov	es, word ptr [bp + 8]
	cld
	xor	si, si
	mov	di, si
	mov	dx, GC_DATA
	mov	bl, 80h
L_0555:
	mov	al, bl
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	lodsb
	xchg	byte ptr es:[di], al
	ror	bl, 1
	jae	L_0555
	inc	di
	cmp	di, PLANE_BYTES
	jb	L_0555
	mov	al, 0FFh
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	pop	es
	pop	si
	pop	dx
	pop	ds
	pop	di
	pop	bx
	pop	ax
	pop	bp
	retf
CopyPage	ENDP

; EGA.OVL 057F (export 1, gRestoreRectToPage): RestoreRect to the page gDrawPage names instead of page 0
RestoreRectToPage	PROC	NEAR
	push	bp
	mov	bp, sp
	sub	sp, 4
	push	ax
	push	ds
	push	si
	mov	ds, word ptr cs:[gDrawPagePtr+2]
	mov	si, word ptr cs:[gDrawPagePtr]
	lodsw
	and	ax, 1Fh
	pop	si
	pop	ds
	mov	word ptr [bp - 2], ax
	jmp	short RestoreRectBody
	pop	bp		; QUIRK 059D: dead code, the jmp above skips it
	retf
RestoreRectToPage	ENDP

; EGA.OVL 059F (export 2, gRestoreRect): RestoreRect(col, y, cols, rows, void far *buf) writes a buffer SaveRect
; filled back to page 0: four bytes for each byte of the rectangle, its planes 0 to 3, each through the map mask
; in write mode 0
RestoreRect	PROC	NEAR
	push	bp
	mov	bp, sp
	sub	sp, 4
	push	ax
	mov	ax, 0
	mov	word ptr [bp - 2], ax
RestoreRectBody:
	push	bx
	push	cx
	push	dx
	push	ds
	push	es
	push	di
	push	si
	cld
	mov	di, word ptr [bp + 6]
	cmp	di, 0
	jge	L_05BE
	xor	di, di
L_05BE:
	cmp	di, SCREEN_COLS
	jl	L_05C6
	mov	di, SCREEN_COLS - 1
L_05C6:
	mov	dx, word ptr [bp + 8]
	cmp	dx, 0
	jge	L_05D0
	xor	dx, dx
L_05D0:
	cmp	dx, SCREEN_HEIGHT
	jl	L_05D9
	mov	dx, SCREEN_HEIGHT - 1
L_05D9:
	mov	bx, word ptr [bp + 0Ah]
	cmp	bx, 0
	jle	L_0633
	cmp	bx, SCREEN_COLS
	jle	L_05E9
	mov	bx, SCREEN_COLS
L_05E9:
	mov	cx, word ptr [bp + 0Ch]
	cmp	cx, 0
	jle	L_0633
	cmp	cx, SCREEN_HEIGHT
	jle	L_05FA
	mov	cx, SCREEN_HEIGHT
L_05FA:
	mov	si, word ptr [bp + 0Eh]
	mov	ax, word ptr [bp + 10h]
	mov	word ptr [bp - 4], ax
	cmp	bx, SCREEN_COLS
	jle	L_060B
	mov	bx, SCREEN_COLS
L_060B:
	or	bx, bx
	je	L_0633
	cmp	cx, SCREEN_HEIGHT
	jle	L_0618
	mov	cx, SCREEN_HEIGHT
L_0618:
	or	cx, cx
	je	L_0633
	push	bx
	mov	bx, dx
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	di, ax
	pop	bx
	mov	ax, word ptr [bp - 2]
	call	PageSeg
	mov	es, ax
	jmp	short L_0636
L_0633:
	jmp	L_06BB
L_0636:
	mov	dx, GC_INDEX
	mov	ax, (0 SHL 8) OR GC_MODE
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gGcMode], al
	out	dx, al
	sti
	mov	bh, bl
	mov	dl, LOW SEQ_INDEX
	mov	al, SEQ_MAP_MASK
	cli
	mov	byte ptr cs:[gSeqIndex], al
	out	dx, al
	sti
	inc	dx
	mov	ax, word ptr [bp - 4]
	mov	ds, ax
	xor	ah, ah
L_0660:
	mov	al, 1
	cli
	mov	byte ptr cs:[gMapMask], al
	out	dx, al
	sti
	lodsb
	xchg	byte ptr es:[di], al
	mov	al, 2
	cli
	mov	byte ptr cs:[gMapMask], al
	out	dx, al
	sti
	lodsb
	xchg	byte ptr es:[di], al
	mov	al, 4
	cli
	mov	byte ptr cs:[gMapMask], al
	out	dx, al
	sti
	lodsb
	xchg	byte ptr es:[di], al
	mov	al, 8
	cli
	mov	byte ptr cs:[gMapMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di]
	movsb
	dec	bl
	jne	L_0660
	mov	bl, bh
	mov	al, bl
	sub	di, ax
	add	di, SCREEN_COLS
	loop	L_0660
	mov	al, 0Fh
	cli
	mov	byte ptr cs:[gMapMask], al
	out	dx, al
	sti
	call	SetWriteMode2
	mov	dx, GC_INDEX
	mov	al, GC_BIT_MASK
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	sti
L_06BB:
	pop	si
	pop	di
	pop	es
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	mov	sp, bp
	pop	bp
	retf
RestoreRect	ENDP

; EGA.OVL 06C7 (export 3, gCopyRegion): CopyRegion(srcCol, srcY, dstCol, dstY, cols, rows, srcPage, dstPage):
; the rectangle from one page to the other in write mode 1, which copies the four planes through the latches with
; each byte moved
CopyRegion	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	di
	push	si
	push	ds
	push	es
	cld
	mov	ax, word ptr [bp + 6]
	cmp	ax, 0
	jge	L_06DD
	xor	ax, ax
L_06DD:
	cmp	ax, SCREEN_COLS
	jge	L_0747
	mov	word ptr cs:[gCopySrcCol], ax
	mov	ax, word ptr [bp + 8]
	cmp	ax, 0
	jge	L_06F0
	xor	ax, ax
L_06F0:
	cmp	ax, SCREEN_HEIGHT
	jge	L_0747
	mov	word ptr cs:[gCopySrcY], ax
	mov	ax, word ptr [bp + 0Ah]
	cmp	ax, 0
	jge	L_0703
	xor	ax, ax
L_0703:
	cmp	ax, SCREEN_COLS
	jge	L_0747
	mov	bx, word ptr cs:[gCopySrcCol]
	cmp	ax, bx		; QUIRK 070D: the compare and exchange leave both columns as they were
	jg	L_0718
	xchg	bx, ax
	mov	word ptr cs:[gCopySrcCol], ax
	mov	ax, bx
L_0718:
	mov	word ptr cs:[gCopyDstCol], ax
	mov	ax, word ptr [bp + 0Ch]
	mov	cx, ax
	cmp	ax, 0
	jge	L_0728
	xor	ax, ax
L_0728:
	cmp	ax, SCREEN_HEIGHT
	jge	L_0747
	mov	word ptr cs:[gCopyDstY], ax
	mov	ax, word ptr [bp + 0Eh]
	cmp	ax, 1
	jge	L_073C
	mov	ax, 1
L_073C:
	cmp	ax, SCREEN_COLS
	jg	L_0747
	mov	word ptr cs:[gCopyCols], ax
	jmp	short L_074A
L_0747:
	jmp	L_07F1
L_074A:
	mov	ax, word ptr [bp + 10h]
	add	cx, ax
	cmp	cx, SCREEN_HEIGHT
	jbe	L_075B
	sub	cx, SCREEN_HEIGHT - 1
	sub	ax, cx
L_075B:
	or	ax, ax
	js	L_0747
	mov	word ptr cs:[gCopyRows], ax
	mov	ax, word ptr [bp + 12h]
	call	PageSeg
	mov	word ptr cs:[gCopySrcOff], 0
	mov	word ptr cs:[gCopySrcSeg], ax
	mov	ax, word ptr [bp + 14h]
	call	PageSeg
	mov	di, 0
	mov	es, ax
	mov	bx, word ptr cs:[gCopySrcY]
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	ax, word ptr cs:[gCopySrcCol]
	add	word ptr cs:[gCopySrcOff], ax
	mov	bx, word ptr cs:[gCopyDstY]
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	ax, word ptr cs:[gCopyDstCol]
	add	di, ax
	mov	si, word ptr cs:[gCopySrcOff]
	mov	dx, GC_INDEX
	mov	ax, (1 SHL 8) OR GC_MODE
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gGcMode], al
	out	dx, al
	sti
	mov	bx, word ptr cs:[gCopyCols]
	mov	dx, word ptr cs:[gCopyRows]
	mov	ax, word ptr cs:[gCopySrcSeg]
	mov	ds, ax
L_07D2:
	mov	cx, bx
	rep movsb
	mov	ax, SCREEN_COLS
	sub	ax, bx
	add	si, ax
	add	di, ax
	dec	dx
	jne	L_07D2
	call	SetWriteMode2
	mov	dx, GC_INDEX
	mov	al, GC_BIT_MASK
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	sti
L_07F1:
	pop	es
	pop	ds
	pop	si
	pop	di
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
CopyRegion	ENDP

; EGA.OVL 07FB (export 4, gClearPage): fills the draw page with colour 0 (write mode 2, all four planes)
ClearPage	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	cx
	push	dx
	push	di
	push	es
	call	DrawPageSeg
	mov	es, ax
	mov	di, 0
	mov	cx, PLANE_BYTES / 2
	mov	ax, di
	rep stosw
	pop	es
	pop	di
	pop	dx
	pop	cx
	pop	ax
	pop	bp
	retf
ClearPage	ENDP

; EGA.OVL 0819 (export 5, gDrawCell8x8): DrawCell8x8(void far *src, col, y) writes 8 rows of 4 bytes (planes 0 to
; 3) from src to the draw page at column col, line y (0 to 192), through the map mask in write mode 0
DrawCell8x8	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	di
	push	si
	push	ds
	push	es
	call	DrawPageSeg
	mov	es, ax
	mov	di, 0
	mov	si, word ptr [bp + 6]
	mov	ax, word ptr [bp + 8]
	mov	word ptr cs:[gCell8x8Seg], ax
	mov	ax, word ptr [bp + 0Ah]
	cmp	ax, 0
	jge	L_0840
	xor	ax, ax
L_0840:
	cmp	ax, SCREEN_COLS
	jl	L_0848
	mov	ax, SCREEN_COLS - 1
L_0848:
	add	di, ax
	mov	ax, word ptr [bp + 0Ch]
	cmp	ax, 0
	jge	L_0854
	xor	ax, ax
L_0854:
	cmp	ax, SCREEN_HEIGHT - 7
	jl	L_085C
	mov	ax, SCREEN_HEIGHT - 8
L_085C:
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	di, ax
	mov	ds, word ptr cs:[gCell8x8Seg]
	mov	dx, GC_INDEX
	mov	ax, (0 SHL 8) OR GC_MODE
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gGcMode], al
	out	dx, al
	sti
	mov	cx, 8
	mov	dx, SEQ_INDEX
	mov	al, SEQ_MAP_MASK
	cli
	mov	byte ptr cs:[gSeqIndex], al
	out	dx, al
	sti
	inc	dx
L_0891:
	mov	al, 1
	cli
	mov	byte ptr cs:[gMapMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di]
	movsb
	dec	di
	mov	al, 2
	cli
	mov	byte ptr cs:[gMapMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di]
	movsb
	dec	di
	mov	al, 4
	cli
	mov	byte ptr cs:[gMapMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di]
	movsb
	dec	di
	mov	al, 8
	cli
	mov	byte ptr cs:[gMapMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di]
	movsb
	add	di, SCREEN_COLS - 1
	loop	L_0891
	mov	al, 0Fh
	cli
	mov	byte ptr cs:[gMapMask], al
	out	dx, al
	sti
	call	SetWriteMode2
	mov	dl, LOW GC_INDEX
	mov	al, GC_BIT_MASK
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	sti
	pop	es
	pop	ds
	pop	si
	pop	di
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
DrawCell8x8	ENDP

; EGA.OVL 08EE (export 6, gDrawIndexedTile): returns at once
DrawIndexedTile	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
DrawIndexedTile	ENDP

; EGA.OVL 08F3 (export 7, gDrawLine): DrawLine(x1, y1, x2, y2, colour), clipped to SetLineClip's rectangle as
; in MCGA.OVL (by outcodes, through gClipByOutcode), then drawn at DrawLineInside
DrawLine	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bp
	push	bx
	push	cx
	push	di
	push	dx
	push	es
	push	si
	mov	ax, word ptr [bp + 0Eh]
	and	al, 0Fh
	mov	byte ptr cs:[gLineColour], al
	mov	ax, word ptr [bp + 6]
	mov	bx, word ptr [bp + 8]
	mov	cx, word ptr [bp + 0Ah]
	mov	dx, word ptr [bp + 0Ch]
	cmp	ax, word ptr cs:[gLineClipLeft]
	jl	L_094B
	cmp	ax, word ptr cs:[gLineClipRight]
	jg	L_094B
	cmp	bx, word ptr cs:[gLineClipTop]
	jl	L_094B
	cmp	bx, word ptr cs:[gLineClipBottom]
	jg	L_094B
	cmp	cx, word ptr cs:[gLineClipLeft]
	jl	L_094B
	cmp	cx, word ptr cs:[gLineClipRight]
	jg	L_094B
	cmp	dx, word ptr cs:[gLineClipTop]
	jl	L_094B
	cmp	dx, word ptr cs:[gLineClipBottom]
	jle	L_0975
L_094B:
	call	LineOutcode
	xchg	cx, ax
	xchg	bx, dx
	mov	di, si
	call	LineOutcode
	mov	bp, di
	or	bp, si
	je	L_0975
	test	di, si
	jne	L_0978
	shl	si, 1
	call	word ptr cs:[si + gClipByOutcode]
	jb	L_094B
	xchg	cx, ax
	xchg	bx, dx
	shl	di, 1
	call	word ptr cs:[di + gClipByOutcode]
	jmp	short L_094B
L_0975:
	jmp	DrawLineInside
L_0978:
	jmp	L_0B8E
gClipByOutcode	LABEL	WORD
	dw	ClipKeep, ClipToTop, ClipToBottom, ClipKeep
	dw	ClipToLeft, ClipToLeft, ClipToBottom, ClipKeep
	dw	ClipToRight, ClipToTop, ClipToRight, ClipKeep
	dw	ClipKeep, ClipKeep, ClipKeep, ClipKeep
DrawLine	ENDP

; EGA.OVL 099B (near): outcode 0 and impossible codes: nothing to move, carry clear
ClipKeep	PROC	NEAR
	clc
	ret
ClipKeep	ENDP

; EGA.OVL 099D (near): moves the end (ax, bx) to the clip top along the line, carry set
ClipToTop	PROC	NEAR
	mov	si, word ptr cs:[gLineClipTop]
	call	MoveToLineY
	stc
	ret
ClipToTop	ENDP

; EGA.OVL 09A7 (near): moves the end to the clip bottom (by mirroring y and using MoveToLineY)
ClipToBottom	PROC	NEAR
	mov	si, word ptr cs:[gLineClipBottom]
	neg	si
	neg	bx
	neg	dx
	call	MoveToLineY
	neg	bx
	neg	dx
	stc
	ret
ClipToBottom	ENDP

; EGA.OVL 09BB (near): the x where the line from (ax, bx) to (cx, dx) meets y = si; returns ax = that x, bx = si
MoveToLineY	PROC	NEAR
	push	dx
	push	ax
	mov	bp, dx
	sub	bp, bx
	neg	ax
	add	ax, cx
	mov	dx, si
	sub	dx, bx
	imul	dx
	idiv	bp
	pop	dx
	add	ax, dx
	pop	dx
	mov	bx, si
	ret
MoveToLineY	ENDP

; EGA.OVL 09D4 (near): moves the end to the clip left edge, carry set
ClipToLeft	PROC	NEAR
	mov	si, word ptr cs:[gLineClipLeft]
	call	MoveToLineX
	stc
	ret
ClipToLeft	ENDP

; EGA.OVL 09DE (near): moves the end to the clip right edge (by mirroring x and using MoveToLineX)
ClipToRight	PROC	NEAR
	mov	si, word ptr cs:[gLineClipRight]
	neg	ax
	neg	cx
	neg	si
	call	MoveToLineX
	neg	ax
	neg	cx
	stc
	ret
ClipToRight	ENDP

; EGA.OVL 09F2 (near): the y where the line meets x = si; returns bx = that y, ax = si
MoveToLineX	PROC	NEAR
	push	dx
	mov	bp, cx
	sub	bp, ax
	sub	dx, bx
	neg	ax
	add	ax, si
	imul	dx
	idiv	bp
	add	bx, ax
	pop	dx
	mov	ax, si
	ret
MoveToLineX	ENDP

; EGA.OVL 0A07 (near): the outcode of the point (ax, bx) in si: 1 above the clip rectangle, 2 below, 4 left of
; it, 8 right of it; DrawLineInside (0A32) follows
LineOutcode	PROC	NEAR
	xor	si, si
	cmp	bx, word ptr cs:[gLineClipTop]
	jge	L_0A13
	or	si, 1
L_0A13:
	cmp	bx, word ptr cs:[gLineClipBottom]
	jle	L_0A1D
	or	si, 2
L_0A1D:
	cmp	ax, word ptr cs:[gLineClipLeft]
	jge	L_0A27
	or	si, 4
L_0A27:
	cmp	ax, word ptr cs:[gLineClipRight]
	jle	L_0A31
	or	si, 8
L_0A31:
	ret
; EGA.OVL 0A32: DrawLineInside, the rest of DrawLine (reached by a jmp, it ends with DrawLine's retf): the line
; from (ax, bx) to (cx, dx) within the rectangle, in one of four loops (gLineSteps) by its direction. The pixels
; of a run in one byte gather in al; each byte is written once, its bit mask then the colour (write mode 2).
DrawLineInside:
	cmp	dx, bx
	jge	L_0A39
	xchg	dx, bx
	xchg	cx, ax
L_0A39:
	xor	si, si
	sub	dx, bx
	sub	cx, ax
	jns	L_0A46
	neg	cx
	or	si, 1
L_0A46:
	cmp	cx, dx
	jge	L_0A4F
	xchg	cx, dx
	or	si, 2
L_0A4F:
	mov	word ptr cs:[gLineMajor], cx
	shr	cx, 1
	mov	bp, cx
	mov	word ptr cs:[gLineMinor], dx
	mov	cl, 3
	xchg	bx, ax
	push	bx
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	pop	bx
	mov	di, bx
	shr	di, cl
	add	di, ax
	and	bx, 7
	mov	bl, byte ptr cs:[bx + gLineBits]
	mov	cx, word ptr cs:[gLineMajor]
	inc	cx
	mov	dx, GC_DATA
	call	DrawPageSeg
	mov	es, ax
	mov	bh, byte ptr cs:[gLineColour]
	xor	al, al
	shl	si, 1
	jmp	word ptr cs:[si + gLineSteps]
L_0A95:
	mov	si, word ptr cs:[gLineMinor]
L_0A9A:
	or	al, bl
	sub	bp, si
	jns	L_0AB7
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	xor	al, al
	mov	ah, byte ptr es:[di]
	mov	byte ptr es:[di], bh
	add	di, SCREEN_COLS
	add	bp, word ptr cs:[gLineMajor]
L_0AB7:
	clc
	rcr	bl, 1
	jae	L_0ACE
	rcr	bl, 1
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	xor	al, al
	mov	ah, byte ptr es:[di]
	mov	byte ptr es:[di], bh
	inc	di
L_0ACE:
	loop	L_0A9A
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	ah, byte ptr es:[di]
	mov	byte ptr es:[di], bh
	jmp	L_0B8E
L_0AE0:
	mov	si, word ptr cs:[gLineMinor]
L_0AE5:
	or	al, bl
	sub	bp, si
	jns	L_0B02
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	xor	al, al
	mov	ah, byte ptr es:[di]
	mov	byte ptr es:[di], bh
	add	di, SCREEN_COLS
	add	bp, word ptr cs:[gLineMajor]
L_0B02:
	clc
	rcl	bl, 1
	jae	L_0B19
	rcl	bl, 1
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	xor	al, al
	mov	ah, byte ptr es:[di]
	mov	byte ptr es:[di], bh
	dec	di
L_0B19:
	loop	L_0AE5
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	ah, byte ptr es:[di]
	mov	byte ptr es:[di], bh
	jmp	short L_0B8E
L_0B2A:
	mov	si, word ptr cs:[gLineMinor]
	mov	al, bl
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
L_0B38:
	mov	ah, byte ptr es:[di]
	mov	byte ptr es:[di], bh
	add	di, SCREEN_COLS
	sub	bp, si
	jns	L_0B59
	clc
	rcr	al, 1
	jae	L_0B4D
	rcr	al, 1
	inc	di
L_0B4D:
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	add	bp, word ptr cs:[gLineMajor]
L_0B59:
	loop	L_0B38
	jmp	short L_0B8E
L_0B5D:
	mov	si, word ptr cs:[gLineMinor]
	mov	al, bl
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
L_0B6B:
	mov	ah, byte ptr es:[di]
	mov	byte ptr es:[di], bh
	add	di, SCREEN_COLS
	sub	bp, si
	jns	L_0B8C
	clc
	rcl	al, 1
	jae	L_0B80
	rcl	al, 1
	dec	di
L_0B80:
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	add	bp, word ptr cs:[gLineMajor]
L_0B8C:
	loop	L_0B6B
L_0B8E:
	mov	al, 0FFh
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	pop	si
	pop	es
	pop	dx
	pop	di
	pop	cx
	pop	bx
	pop	bp
	pop	ax
	pop	bp
	retf
LineOutcode	ENDP

; cs:0BA1: DrawLine's state
	db	2 dup (0)		; 0BA1: not used
gLineMajor	dw	0		; the larger of the two distances: pixels in the line, less one
gLineMinor	dw	0		; the smaller
gLineBits	db	80h, 40h, 20h, 10h, 8, 4, 2, 1	; the bit of a pixel, by x AND 7

gLineSteps	LABEL	WORD
	dw	L_0A95, L_0AE0, L_0B2A, L_0B5D
gLineColour	db	0		; DrawLine's colour, AND 0Fh, kept inside the code (the retf above ends the line)

; EGA.OVL 0BB8 (export 9, gFillRect): FillRect(x1, y1, x2, y2, colour) fills the rectangle on the draw page with
; colour AND 0Fh: each line's first and last bytes through gLeftMasks and gRightMasks, the bytes between with the
; bit mask 0FFh
FillRect	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	bp
	push	di
	push	es
	mov	ax, word ptr [bp + 8]
	cmp	ax, 0
	jl	L_0BD1
	cmp	ax, SCREEN_HEIGHT
	jge	L_0C31
	jmp	short L_0BD3
L_0BD1:
	xor	ax, ax
L_0BD3:
	mov	cx, word ptr [bp + 0Ch]
	cmp	cx, 0
	jl	L_0BE3
	cmp	cx, SCREEN_HEIGHT
	jge	L_0C31
	jmp	short L_0BE5
L_0BE3:
	xor	cx, cx
L_0BE5:
	cmp	cx, ax
	jge	L_0BEA
	xchg	cx, ax
L_0BEA:
	sub	cx, ax
	inc	cx
	mov	word ptr cs:[gFillRows], cx
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	mov	di, ax
	mov	word ptr cs:[gFillRowOff], di
	call	DrawPageSeg
	mov	es, ax
	mov	ax, word ptr [bp + 6]
	cmp	ax, 0
	jl	L_0C16
	cmp	ax, SCREEN_WIDTH
	jge	L_0C31
	jmp	short L_0C18
L_0C16:
	xor	ax, ax
L_0C18:
	mov	bx, word ptr [bp + 0Ah]
	cmp	bx, 0
	jl	L_0C28
	cmp	bx, SCREEN_WIDTH
	jge	L_0C31
	jmp	short L_0C2A
L_0C28:
	xor	bx, bx
L_0C2A:
	cmp	bx, ax
	jge	L_0C34
	xchg	bx, ax
	jmp	short L_0C34
L_0C31:
	jmp	L_0D03
L_0C34:
	mov	cx, word ptr [bp + 0Eh]
	xor	ch, ch
	and	cl, 0Fh
	mov	byte ptr cs:[gFillColour], cl
	mov	word ptr cs:[gFillLeft], ax
	mov	word ptr cs:[gFillRight], bx
	mov	cx, word ptr cs:[gFillRows]
L_0C4F:
	push	cx
	mov	cl, byte ptr cs:[gFillColour]
	mov	ax, word ptr cs:[gFillLeft]
	mov	bx, word ptr cs:[gFillRight]
	mov	word ptr cs:[gSpanLeft], ax
	mov	word ptr cs:[gSpanRight], bx
	mov	ch, cl
	mov	cl, 3
	shr	ax, cl
	shr	bx, cl
	add	di, ax
	sub	bx, ax
	mov	dx, GC_DATA
	mov	ah, ch
	mov	cx, bx
	or	cx, cx
	je	L_0CCA
	mov	bx, word ptr cs:[gSpanLeft]
	and	bx, 7
	mov	al, byte ptr cs:[bx + gLeftMasks]
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di]
	mov	byte ptr es:[di], ah
	inc	di
	dec	cx
	je	L_0CAE
	cld
	mov	al, 0FFh
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, ah
	rep stosb
	jmp	short L_0CAE
L_0CAC:
	jmp	short L_0C4F
L_0CAE:
	mov	bx, word ptr cs:[gSpanRight]
	and	bx, 7
	mov	al, byte ptr cs:[bx + gRightMasks]
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di]
	mov	byte ptr es:[di], ah
	jmp	short L_0CF1
L_0CCA:
	mov	bx, word ptr cs:[gSpanLeft]
	and	bx, 7
	mov	al, byte ptr cs:[bx + gLeftMasks]
	mov	bx, word ptr cs:[gSpanRight]
	and	bx, 7
	and	al, byte ptr cs:[bx + gRightMasks]
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di]
	mov	byte ptr es:[di], ah
L_0CF1:
	pop	cx
	push	ax
	mov	ax, word ptr cs:[gFillRowOff]
	add	ax, SCREEN_COLS
	mov	di, ax
	mov	word ptr cs:[gFillRowOff], ax
	pop	ax
	loop	L_0CAC
L_0D03:
	mov	al, 0FFh
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	pop	es
	pop	di
	pop	bp
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
FillRect	ENDP

; EGA.OVL 0D15 (export 10, ds:806E): ReadCell8x8(page, void far *dst, col, y) copies 8 rows of a page's byte
; at column col into dst, 4 bytes a row (planes 0 to 3, through the read map); no call of ds:806E was found in
; EOB.EXE or INTRO.EXE
ReadCell8x8	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	ds
	push	di
	push	si
	push	es
	mov	dx, GC_DATA
	mov	al, 0		; the bit mask 0 while the planes are read
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	dec	dx
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	ds, ax
	mov	ax, word ptr [bp + 8]
	mov	di, ax
	mov	es, word ptr [bp + 0Ah]
	mov	si, word ptr [bp + 0Ch]
	cmp	si, 0
	jge	L_0D47
	xor	si, si
L_0D47:
	cmp	si, SCREEN_COLS
	jl	L_0D4F
	mov	si, SCREEN_COLS - 1
L_0D4F:
	mov	bx, word ptr [bp + 0Eh]
	cmp	bx, 0
	jge	L_0D59
	xor	bx, bx
L_0D59:
	cmp	bx, SCREEN_HEIGHT - 8
	jle	L_0D62
	mov	bx, SCREEN_HEIGHT - 8
L_0D62:
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	si, ax
	mov	cx, 8
	mov	al, GC_READ_MAP
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	sti
	inc	dx
L_0D78:
	xor	ax, ax
	cli
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	sti
	mov	al, byte ptr [si]
	stosb
	mov	ax, 1
	cli
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	sti
	mov	al, byte ptr [si]
	stosb
	mov	ax, 2
	cli
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	sti
	mov	al, byte ptr [si]
	stosb
	mov	ax, 3
	cli
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	sti
	mov	al, byte ptr [si]
	stosb
	add	si, SCREEN_COLS
	loop	L_0D78
	call	ResetBitMask
	pop	es
	pop	si
	pop	di
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
ReadCell8x8	ENDP

; EGA.OVL 0DBD (near): ax = the segment of the page after the draw page (gDrawPage + 1)
NextPageSeg	PROC	NEAR
	push	ds
	push	si
	mov	ds, word ptr cs:[gDrawPagePtr+2]
	mov	si, word ptr cs:[gDrawPagePtr]
	lodsw
	inc	ax
	and	ax, 1Fh
	shl	ax, 1
	mov	ds, word ptr cs:[gPageSegsPtr+2]
	mov	si, word ptr cs:[gPageSegsPtr]
	add	si, ax
	lodsw
	pop	si
	pop	ds
	ret
NextPageSeg	ENDP

; EGA.OVL 0DE0 (near): ax = the segment of the draw page, gPageSegs[gDrawPage & 1Fh]
DrawPageSeg	PROC	NEAR
	push	ds
	push	si
	mov	ds, word ptr cs:[gDrawPagePtr+2]
	mov	si, word ptr cs:[gDrawPagePtr]
	lodsw
	and	ax, 1Fh
	shl	ax, 1
	mov	ds, word ptr cs:[gPageSegsPtr+2]
	mov	si, word ptr cs:[gPageSegsPtr]
	add	si, ax
	lodsw
	pop	si
	pop	ds
	ret
DrawPageSeg	ENDP

; EGA.OVL 0E02 (near): ax = the segment of page ax, gPageSegs[ax & 1Fh]
PageSeg	PROC	NEAR
	push	ds
	push	si
	and	ax, 1Fh
	shl	ax, 1
	mov	ds, word ptr cs:[gPageSegsPtr+2]
	mov	si, word ptr cs:[gPageSegsPtr]
	add	si, ax
	lodsw
	pop	si
	pop	ds
	ret
PageSeg	ENDP

; EGA.OVL 0E19 (export 11, gGetPixel): GetPixel(x, y) returns the colour (0 to 15) of the draw page's pixel,
; its bit read from each plane in turn
GetPixel	PROC	NEAR
	push	bp
	mov	bp, sp
	push	bx
	push	cx
	push	dx
	push	ds
	push	si
	mov	bx, word ptr [bp + 8]
	or	bx, bx
	js	L_0E41
	cmp	bx, SCREEN_HEIGHT
	jge	L_0E41
	shl	bx, 1
	mov	si, word ptr cs:[bx + gRowOffsets]
	mov	ax, word ptr [bp + 6]
	or	ax, ax
	js	L_0E41
	cmp	ax, SCREEN_WIDTH
	jl	L_0E43
L_0E41:
	jmp	short L_0EC1
L_0E43:
	mov	bx, ax
	shr	ax, 1
	shr	ax, 1
	shr	ax, 1
	add	si, ax
	call	DrawPageSeg
	mov	ds, ax
	and	bx, 7
	mov	cx, bx
	inc	cl
	mov	bl, byte ptr cs:[bx + gPixelMasks]
	mov	dx, GC_INDEX
	mov	al, GC_READ_MAP
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	inc	dx
	xor	al, al
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	sti
	mov	al, byte ptr [si]
	and	ax, bx
	shl	ax, cl
	inc	cl
	or	ch, ah
	mov	al, 1
	cli
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	sti
	mov	al, byte ptr [si]
	and	ax, bx
	shl	ax, cl
	inc	cl
	or	ch, ah
	mov	al, 2
	cli
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	sti
	mov	al, byte ptr [si]
	and	ax, bx
	shl	ax, cl
	inc	cl
	or	ch, ah
	mov	al, 3
	cli
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	sti
	mov	al, byte ptr [si]
	and	ax, bx
	shl	ax, cl
	or	ah, ch
	dec	dx
	mov	al, GC_BIT_MASK
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	sti
	xor	al, al
	jmp	short L_0EC3
L_0EC1:
	xor	ax, ax
L_0EC3:
	xchg	al, ah
	pop	si
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	bp
	retf
GetPixel	ENDP

; EGA.OVL 0ECC (export 12, ds:8076): MirrorCell16 returns at once; no call of ds:8076 was found
MirrorCell16	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
MirrorCell16	ENDP

; EGA.OVL 0ED1 (near): the graphics controller's index to the bit mask, and the bit mask 0FFh
ResetBitMask	PROC	NEAR
	push	ax
	push	dx
	mov	dx, GC_INDEX
	mov	ax, (0FFh SHL 8) OR GC_BIT_MASK
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	pop	dx
	pop	ax
	ret
ResetBitMask	ENDP

; EGA.OVL 0EEB: LinkDriver(void far *table), as MCGA.OVL 0BE6: exchanges the slot tables (imports in, exports
; out); carry set when an export slot of the driver is zero. Then, unlike the other drivers, it calls the game's
; SetFarPointer (import slot 6) with the far pointer of its register shadow, cs:00C1.
LinkDriver	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	di
	push	ds
	push	dx
	push	es
	push	si
	cld
	mov	si, word ptr [bp + 6]
	mov	ds, word ptr [bp + 8]
	mov	di, 3
	mov	ax, cs
	mov	es, ax
	mov	cx, word ptr cs:[gSlotCount]
L_0F09:
	xor	bx, bx
	lodsw
	mov	dx, ax
	lodsw
	or	bx, ax
	or	bx, dx
	je	L_0F1D
	xchg	dx, ax
	stosw
	xchg	dx, ax
	stosw
	loop	L_0F09
	jmp	short L_0F36
L_0F1D:
	sub	si, 4
	mov	dx, word ptr es:[di]
	add	di, 4
	or	dx, dx
	je	L_0F39
	mov	word ptr [si], dx
	add	si, 2
	mov	word ptr [si], es
	add	si, 2
	loop	L_0F09
L_0F36:
	clc
	jmp	short L_0F3A
L_0F39:
	stc
L_0F3A:
	push	cs
	mov	ax, offset gSeqIndex
	push	ax
	call	dword ptr cs:[gSetFarPointerPtr]
	add	sp, 4
	pop	si
	pop	es
	pop	dx
	pop	ds
	pop	di
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
LinkDriver	ENDP

; EGA.OVL 0F51 (near): sets the sequencer as mode 0Dh has it (reset 3, clocking 0Bh, character map 0, memory mode
; 6, map mask 0Fh), then write mode 2 and the bit mask 0FFh, the registers and their shadow the same
InitRegisters	PROC	NEAR
	push	ax
	push	dx
	mov	dx, SEQ_INDEX
	mov	ax, (3 SHL 8) OR SEQ_RESET
	cli
	mov	byte ptr cs:[gSeqIndex], al
	out	dx, al
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gSeqReset], al
	out	dx, al
	sti
	dec	dx
	mov	ax, (0Bh SHL 8) OR SEQ_CLOCKING
	cli
	mov	byte ptr cs:[gSeqIndex], al
	out	dx, al
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gSeqClocking], al
	out	dx, al
	sti
	dec	dx
	mov	ax, (0 SHL 8) OR SEQ_CHAR_MAP
	cli
	mov	byte ptr cs:[gSeqIndex], al
	out	dx, al
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gSeqCharMap], al
	out	dx, al
	sti
	dec	dx
	mov	ax, (6 SHL 8) OR SEQ_MEMORY_MODE
	cli
	mov	byte ptr cs:[gSeqIndex], al
	out	dx, al
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gSeqMemoryMode], al
	out	dx, al
	sti
	dec	dx
	mov	ax, (0Fh SHL 8) OR SEQ_MAP_MASK
	cli
	mov	byte ptr cs:[gSeqIndex], al
	out	dx, al
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gMapMask], al
	out	dx, al
	sti
	call	SetWriteMode2
	call	ResetBitMask
	pop	dx
	pop	ax
	ret
InitRegisters	ENDP

; EGA.OVL 0FBD: DriverEntry, the far call the game makes at segment:0000 once the driver is loaded: sets the
; registers (InitRegisters), returns bx = the driver's size in paragraphs and ax = the offset of LinkDriver
DriverEntry	PROC	NEAR
	push	bp
	mov	bp, sp
	push	cx
	call	InitRegisters
	mov	bx, offset DriverEnd
	mov	cl, 4
	shr	bx, cl
	inc	bx
	mov	ax, offset LinkDriver
	pop	cx
	pop	bp
	retf
DriverEntry	ENDP

; EGA.OVL 0FD2 (near): the graphics controller's mode register to write mode 2, the driver's resting state
SetWriteMode2	PROC	NEAR
	push	ax
	push	dx
	mov	dx, GC_INDEX
	mov	ax, (2 SHL 8) OR GC_MODE
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gGcMode], al
	out	dx, al
	sti
	pop	dx
	pop	ax
	ret
SetWriteMode2	ENDP

; EGA.OVL 0FEC (export 13, ds:807A): SetTileSet returns at once; no call of ds:807A was found
SetTileSet	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
SetTileSet	ENDP

; EGA.OVL 0FF1 (exports 15 and 16, gGetSpriteCellRle and gGetSpriteCell): GetSpriteCell(col, y, cols, rows,
; map) cuts the rectangle at column col, line y out of the art page after the draw page into a cell from
; MemAlloc, in the run format the header comment gives: for each 8-pixel group, gColourBits gathers the pixels
; of each colour, and a (mask, colour) pair is written for each colour present. It runs twice: the first pass
; (gCellSizing 1) only counts the bytes, MemAlloc gets that size, the second writes. Returns dx:ax the block.
; The colour map argument is not read.
GetSpriteCell	PROC	NEAR
	push	bp
	mov	bp, sp
	push	bx
	push	cx
	push	di
	push	es
	push	si
	push	ds
	cld
	mov	word ptr cs:[gCellOff], 0
	mov	byte ptr cs:[gCellSizing], 1
L_1008:
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gCellColsArg], ax
	mov	bx, word ptr [bp + 0Ch]
	mov	word ptr cs:[gCellRowsArg], bx
	call	NextPageSeg
	mov	ds, ax
	mov	cl, 3
	mov	si, word ptr [bp + 6]
	mov	word ptr cs:[gCellCol], si
	shl	si, cl
	mov	ax, word ptr [bp + 8]
	mov	word ptr cs:[gCellY], ax
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	si, ax
	cmp	byte ptr cs:[gCellSizing], 0
	jne	L_1054
	xor	ax, ax
	stosb
	mov	ax, word ptr [bp + 0Ch]
	mov	dh, al
	stosb
	mov	ax, word ptr [bp + 0Ah]
	mov	dl, al
	stosb
	mov	al, dh
	stosb
L_1054:
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	ds
	push	es
	push	bp
	mov	bx, word ptr cs:[gCellY]
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	bx, word ptr cs:[gCellCol]
	shl	bl, 1
	shl	bl, 1
	shl	bx, 1
	add	ax, bx
	mov	si, ax
	call	NextPageSeg
	mov	word ptr cs:[gCellSrcSeg], ax
	mov	ds, ax
	mov	di, word ptr cs:[gCellOff]
	add	di, 4
	mov	word ptr cs:[gCellDataStart], di
	mov	es, word ptr cs:[gCellSeg]
	mov	ax, word ptr cs:[gCellRowsArg]
	mov	byte ptr cs:[gCellRowsLeft], al
	push	ax
	mov	ax, word ptr cs:[gCellColsArg]
	mov	dx, ax
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	bx, SCREEN_WIDTH
	sub	bx, ax
	mov	word ptr cs:[gCellSrcSkip], bx
	pop	ax
	push	ds
	mov	bp, si
	mov	word ptr cs:[gCellCols], dx
	mov	ax, cs
	mov	ds, ax
L_10C4:
	mov	dx, word ptr cs:[gCellCols]
	mov	byte ptr cs:[gCellColsLeft], dl
L_10CE:
	mov	si, bp
	push	ds
	mov	ax, word ptr cs:[gCellSrcSeg]
	mov	ds, ax
	lodsb
	shl	al, 1
	shl	al, 1
	shl	al, 1
	shl	al, 1
	mov	ah, al
	lodsb
	and	al, 0Fh
	or	ah, al
	mov	bh, ah
	lodsb
	shl	al, 1
	shl	al, 1
	shl	al, 1
	shl	al, 1
	mov	ah, al
	lodsb
	and	al, 0Fh
	or	al, ah
	mov	bl, al
	lodsb
	shl	al, 1
	shl	al, 1
	shl	al, 1
	shl	al, 1
	mov	ah, al
	lodsb
	and	al, 0Fh
	or	ah, al
	lodsb
	shl	al, 1
	shl	al, 1
	shl	al, 1
	shl	al, 1
	mov	byte ptr cs:[gPixelHigh], al
	lodsb
	and	al, 0Fh
	or	al, byte ptr cs:[gPixelHigh]
	mov	bp, si
	pop	ds
	mov	si, offset gColourBits
	mov	cx, 10h
L_1129:
	mov	byte ptr [si], ch
	inc	si
	loop	L_1129
	mov	ch, 80h
L_1130:
	call	NextPixelColour
	mov	si, offset gColourBits
	add	si, dx
	or	byte ptr [si], ch
	shr	ch, 1
	je	L_1147
	cmp	ch, 8
	jne	L_1130
	mov	bx, ax
	jmp	short L_1130
L_1147:
	mov	si, offset gColourBits
	mov	bx, 0Fh
	xor	al, al
L_114F:
	or	al, byte ptr [bx + si]
	dec	bx
	jne	L_114F
	or	al, al
	je	L_117A
	mov	cx, 1
	inc	si
L_115C:
	lodsb
	or	al, al
	je	L_1172
	cmp	byte ptr cs:[gCellSizing], 0
	je	L_116E
	add	di, 2
	jmp	short L_1172
L_116E:
	stosb
	mov	al, cl
	stosb
L_1172:
	inc	cx
	cmp	cx, 10h
	jb	L_115C
	jae	L_1189
L_117A:
	cmp	byte ptr cs:[gCellSizing], 0
	je	L_1187
	add	di, 2
	jmp	short L_1189
L_1187:
	stosb
	stosb
L_1189:
	cmp	byte ptr cs:[gCellSizing], 0
	jne	L_1198
	dec	di
	mov	al, 80h
	or	byte ptr es:[di], al
	inc	di
L_1198:
	dec	byte ptr cs:[gCellColsLeft]
	je	L_11A2
	jmp	L_10CE
L_11A2:
	cmp	byte ptr cs:[gCellSizing], 0
	jne	L_11B1
	dec	di
	mov	al, 40h
	or	byte ptr es:[di], al
	inc	di
L_11B1:
	add	bp, word ptr cs:[gCellSrcSkip]
	dec	byte ptr cs:[gCellRowsLeft]
	je	L_11C0
	jmp	L_10C4
L_11C0:
	cmp	byte ptr cs:[gCellSizing], 0
	jne	L_11CF
	dec	di
	mov	al, 20h
	or	byte ptr es:[di], al
	inc	di
L_11CF:
	pop	ds
	jmp	short L_11E5
; EGA.OVL 11D2 (near): dx = the top four bits of bx, shifted out: the next pixel of a packed pair word
NextPixelColour:
	xor	dx, dx
	shl	bx, 1
	rcl	dl, 1
	shl	bx, 1
	rcl	dl, 1
	shl	bx, 1
	rcl	dl, 1
	shl	bx, 1
	rcl	dl, 1
	ret
L_11E5:
	pop	bp
	pop	es
	pop	ds
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	cmp	byte ptr cs:[gCellSizing], 0
	je	L_1222
	pop	ds
	push	ds
	mov	ax, 0
	push	ax
	xor	ax, ax
	push	ax
	sub	di, word ptr cs:[gCellOff]
	push	di
	call	dword ptr cs:[gMemAllocPtr]
	add	sp, 6
	mov	word ptr cs:[gCellOff], ax
	mov	word ptr cs:[gCellSeg], dx
	mov	es, dx
	mov	di, ax
	mov	byte ptr cs:[gCellSizing], 0
	jmp	L_1008
L_1222:
	mov	dx, word ptr cs:[gCellSeg]
	mov	ax, word ptr cs:[gCellOff]
	pop	ds
	pop	si
	pop	es
	pop	di
	pop	cx
	pop	bx
	pop	bp
	retf
GetSpriteCell	ENDP

gCellSeg	dw	0		; GetSpriteCell's cell: segment, kept inside the code (the retf above ends GetSpriteCell)
gCellOff	dw	0		; and offset

; EGA.OVL 1237 (export 17, ds:808A): DrawMaskedCell16 returns at once
DrawMaskedCell16	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
DrawMaskedCell16	ENDP

; EGA.OVL 123C (export 18, gCopyToPlanar): CopyToPlanar(srcOff, srcSeg, dstSeg, bytes) writes a buffer of
; 4-bit pixels (two a byte, the high nibble first) to the planes at dstSeg:0000, eight pixels to a byte, each
; through its bit in the bit mask (write mode 2); LoadWallTextures calls it in EGA mode only
CopyToPlanar	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	ds
	push	si
	push	es
	push	di
	mov	si, word ptr [bp + 6]
	mov	es, word ptr [bp + 8]
	mov	ds, word ptr [bp + 0Ah]
	mov	di, 0
	mov	bp, word ptr [bp + 0Ch]
	shr	bp, 1
	shr	bp, 1
	mov	dx, GC_DATA
	mov	cl, 4
L_125F:
	mov	ax, 80h
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, byte ptr [di]
	mov	bl, byte ptr es:[si]
	mov	al, bl
	shr	al, cl
	mov	byte ptr [di], al
	mov	al, byte ptr [di]
	mov	ax, 40h
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	byte ptr [di], bl
	inc	si
	mov	ax, 20h
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, byte ptr [di]
	mov	bl, byte ptr es:[si]
	mov	al, bl
	shr	al, cl
	mov	byte ptr [di], al
	mov	al, byte ptr [di]
	mov	ax, 10h
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	byte ptr [di], bl
	inc	si
	mov	ax, 8
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, byte ptr [di]
	mov	bl, byte ptr es:[si]
	mov	al, bl
	shr	al, cl
	mov	byte ptr [di], al
	mov	al, byte ptr [di]
	mov	ax, 4
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	byte ptr [di], bl
	inc	si
	mov	ax, 2
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, byte ptr [di]
	mov	bl, byte ptr es:[si]
	mov	al, bl
	shr	al, cl
	mov	byte ptr [di], al
	mov	al, byte ptr [di]
	mov	ax, 1
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	byte ptr [di], bl
	inc	si
	inc	di
	dec	bp
	je	L_12F6
	jmp	L_125F
L_12F6:
	mov	al, 0FFh
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	pop	di
	pop	es
	pop	si
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
CopyToPlanar	ENDP

; EGA.OVL 1309 (export 19, ds:8092): SwapPageWithScreen(page) shows the page (INT 10h function 5, display page
; (segment SHR 9) AND 7) and exchanges its entry in gPageSegs with page 0's, a page flip instead of MCGA.OVL's copy;
; no call of ds:8092 was found
SwapPageWithScreen	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	ds
	push	si
	push	es
	push	di
	mov	ax, word ptr [bp + 6]
	push	ax
	call	PageSeg
	mov	al, ah
	shr	al, 1
	and	al, 7
	mov	ah, 5
	int	10h
	pop	ax
	shl	ax, 1
	mov	ds, word ptr cs:[gPageSegsPtr+2]
	mov	si, word ptr cs:[gPageSegsPtr]
	add	si, ax
	mov	ax, 0
	shl	ax, 1
	mov	es, word ptr cs:[gPageSegsPtr+2]
	mov	di, word ptr cs:[gPageSegsPtr]
	add	di, ax
	mov	bx, word ptr [si]
	mov	ax, word ptr es:[di]
	mov	word ptr [si], ax
	mov	word ptr es:[di], bx
	pop	di
	pop	es
	pop	si
	pop	ds
	pop	bx
	pop	ax
	pop	bp
	retf
SwapPageWithScreen	ENDP

; EGA.OVL 1355 (export 20, gDrawShapeMaskedFlipped): returns at once
DrawShapeMaskedFlipped	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
DrawShapeMaskedFlipped	ENDP

; EGA.OVL 135A (export 21, gDrawShapeMasked): returns at once
DrawShapeMasked	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
DrawShapeMasked	ENDP

; EGA.OVL 135F (export 22, gPutPixel): PutPixel(x, y, colour) sets the draw page's pixel (write mode 2, its bit
; in the bit mask); off the screen it does nothing
PutPixel	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	dx
	push	es
	push	di
	mov	bx, word ptr [bp + 8]
	or	bx, bx
	js	L_13B7
	cmp	bx, SCREEN_HEIGHT
	jge	L_13B7
	shl	bx, 1
	mov	di, word ptr cs:[bx + gRowOffsets]
	mov	ax, word ptr [bp + 6]
	or	ax, ax
	js	L_13B7
	cmp	ax, SCREEN_WIDTH
	jge	L_13B7
	mov	bx, ax
	shr	ax, 1
	shr	ax, 1
	shr	ax, 1
	add	di, ax
	call	DrawPageSeg
	mov	es, ax
	mov	dx, GC_DATA
	and	bx, 7
	mov	al, byte ptr cs:[bx + gPixelMasks]
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	ax, word ptr [bp + 0Ah]
	xchg	byte ptr es:[di], al
	mov	al, 0FFh
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
L_13B7:
	pop	di
	pop	es
	pop	dx
	pop	bx
	pop	ax
	pop	bp
	retf
PutPixel	ENDP

; EGA.OVL 13BE (export 23, gRestoreDrawState): puts back the region SaveDrawState kept, loads the latches from
; A000:FFFF and writes each register whose kept value (cs:00D1) differs from the shadow, the indexes last
RestoreDrawState	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	dx
	push	es
	push	si
	mov	ax, word ptr cs:[gSavedLeft]
	mov	word ptr cs:[gRegionLeft], ax
	mov	ax, word ptr cs:[gSavedRight]
	mov	word ptr cs:[gRegionRight], ax
	mov	ax, word ptr cs:[gSavedTop]
	mov	word ptr cs:[gRegionTop], ax
	mov	ax, word ptr cs:[gSavedBottom]
	mov	word ptr cs:[gRegionBottom], ax
	mov	ax, VIDEO_SEG
	mov	es, ax
	mov	si, LATCH_SAVE
	mov	al, byte ptr es:[si]
	mov	dx, SEQ_INDEX
	mov	ah, byte ptr cs:[gSavedMapMask]
	cmp	ah, byte ptr cs:[gMapMask]
	je	L_1417
	cmp	byte ptr cs:[gSeqIndex], SEQ_MAP_MASK
	je	L_140E
	mov	al, SEQ_MAP_MASK
	mov	byte ptr cs:[gSeqIndex], al
	out	dx, al
L_140E:
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gMapMask], al
	out	dx, al
	dec	dx
L_1417:
	mov	al, byte ptr cs:[gSavedSeqIndex]
	cmp	al, byte ptr cs:[gSeqIndex]
	je	L_1427
	mov	byte ptr cs:[gSeqIndex], al
	out	dx, al
L_1427:
	mov	dl, LOW GC_INDEX
	mov	ah, byte ptr cs:[gSavedSetReset]
	cmp	ah, byte ptr cs:[gSetReset]
	je	L_144D
	cmp	byte ptr cs:[gGcIndex], GC_SET_RESET
	je	L_1444
	mov	al, GC_SET_RESET
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_1444:
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gSetReset], al
	out	dx, al
	dec	dx
L_144D:
	mov	ah, byte ptr cs:[gSavedEnableSetReset]
	cmp	ah, byte ptr cs:[gEnableSetReset]
	je	L_1471
	cmp	byte ptr cs:[gGcIndex], GC_ENABLE_SET_RESET
	je	L_1468
	mov	al, GC_ENABLE_SET_RESET
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_1468:
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gEnableSetReset], al
	out	dx, al
	dec	dx
L_1471:
	mov	ah, byte ptr cs:[gSavedColourCompare]
	cmp	ah, byte ptr cs:[gColourCompare]
	je	L_1495
	cmp	byte ptr cs:[gGcIndex], GC_COLOUR_COMPARE
	je	L_148C
	mov	al, GC_COLOUR_COMPARE
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_148C:
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gColourCompare], al
	out	dx, al
	dec	dx
L_1495:
	mov	ah, byte ptr cs:[gSavedDataRotate]
	cmp	ah, byte ptr cs:[gDataRotate]
	je	L_14B9
	cmp	byte ptr cs:[gGcIndex], GC_DATA_ROTATE
	je	L_14B0
	mov	al, GC_DATA_ROTATE
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_14B0:
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gDataRotate], al
	out	dx, al
	dec	dx
L_14B9:
	mov	ah, byte ptr cs:[gSavedReadMap]
	cmp	ah, byte ptr cs:[gReadMap]
	je	L_14DD
	cmp	byte ptr cs:[gGcIndex], GC_READ_MAP
	je	L_14D4
	mov	al, GC_READ_MAP
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_14D4:
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	dec	dx
L_14DD:
	mov	ah, byte ptr cs:[gSavedGcMode]
	cmp	ah, byte ptr cs:[gGcMode]
	je	L_1501
	cmp	byte ptr cs:[gGcIndex], GC_MODE
	je	L_14F8
	mov	al, GC_MODE
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_14F8:
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gGcMode], al
	out	dx, al
	dec	dx
L_1501:
	mov	ah, byte ptr cs:[gSavedGcMisc]
	cmp	ah, byte ptr cs:[gGcMisc]
	je	L_1525
	cmp	byte ptr cs:[gGcIndex], GC_MISC
	je	L_151C
	mov	al, GC_MISC
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_151C:
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gGcMisc], al
	out	dx, al
	dec	dx
L_1525:
	mov	ah, byte ptr cs:[gSavedColourDontCare]
	cmp	ah, byte ptr cs:[gColourDontCare]
	je	L_1549
	cmp	byte ptr cs:[gGcIndex], GC_COLOUR_DONT_CARE
	je	L_1540
	mov	al, GC_COLOUR_DONT_CARE
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_1540:
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gColourDontCare], al
	out	dx, al
	dec	dx
L_1549:
	mov	ah, byte ptr cs:[gSavedBitMask]
	cmp	ah, byte ptr cs:[gBitMask]
	je	L_156D
	cmp	byte ptr cs:[gGcIndex], GC_BIT_MASK
	je	L_1564
	mov	al, GC_BIT_MASK
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_1564:
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	dec	dx
L_156D:
	mov	al, byte ptr cs:[gSavedGcIndex]
	cmp	al, byte ptr cs:[gGcIndex]
	je	L_157D
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_157D:
	pop	si
	pop	es
	pop	dx
	pop	ax
	pop	bp
	retf
RestoreDrawState	ENDP

; EGA.OVL 1583 (export 24, gDrawShapeFlipped): DrawShape mirrored left to right: the cell's groups from the
; right end, each mask through gMirrorBits
DrawShapeFlipped	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	ds
	push	es
	push	di
	push	si
	mov	ax, word ptr cs:[gRegionBottom]
	mov	word ptr cs:[gFlipBottom], ax
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	mov	word ptr cs:[gFlipEnd], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gFlipTop], ax
	xor	di, di
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	es, ax
	mov	si, word ptr [bp + 8]
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gFlipSeg], ax
	mov	ax, word ptr [bp + 0Ch]
	mov	word ptr cs:[gFlipX], ax
	sar	ax, 1
	sar	ax, 1
	sar	ax, 1
	mov	word ptr cs:[gFlipCol], ax
	mov	dx, word ptr [bp + 0Eh]
	mov	ax, word ptr cs:[gRegionLeft]
	dec	ax
	mov	word ptr cs:[gFlipClipLeft], ax
	mov	ax, word ptr cs:[gRegionRight]
	dec	ax
	mov	word ptr cs:[gFlipClipRight], ax
	mov	bp, ds
	mov	bx, word ptr cs:[gFlipSeg]
	mov	ds, bx
	inc	si
	lodsw
	mov	ds, bp
	mov	byte ptr cs:[gFlipCols], ah
	mov	ds, bx
	mov	bx, ax
	and	bx, 0FFh
	inc	bx
	mov	byte ptr cs:[gFlipRows], bl
	inc	si
	cmp	dx, word ptr cs:[gFlipBottom]
	jge	L_161C
	mov	cx, dx
	sub	cx, word ptr cs:[gFlipTop]
	jns	L_1633
	neg	cx
	cmp	cx, bx
	jl	L_161F
L_161C:
	jmp	L_173C
L_161F:
	lodsw
	test	ah, 40h
	je	L_161F
	dec	byte ptr cs:[gFlipRows]
	je	L_161C
	loop	L_161F
	mov	dx, word ptr cs:[gFlipTop]
L_1633:
	mov	ds, bp
	push	bx
	mov	bx, dx
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	pop	bx
	add	di, ax
	add	di, word ptr cs:[gFlipCol]
	mov	al, byte ptr cs:[gFlipCols]
	xor	ah, ah
	dec	ax
	add	di, ax
	mov	cx, word ptr cs:[gFlipX]
	and	cx, 7
	mov	bx, word ptr cs:[gFlipCol]
	add	bx, ax
	mov	ax, word ptr cs:[gFlipSeg]
	jmp	short L_1668
L_1665:
	lodsb
	jmp	short L_169D
L_1668:
	push	ds
	mov	ds, ax
	mov	dx, GC_DATA
	mov	bp, di
L_1670:
	push	bx
	mov	ch, bl
	mov	bx, offset gMirrorBits
L_1676:
	lodsb
	or	al, al
	je	L_16D0
	or	cl, cl
	jne	L_16E4
	cmp	ch, byte ptr cs:[gFlipClipRight]
	jg	L_1665
	cmp	ch, byte ptr cs:[gFlipClipLeft]
	jle	L_1665
	xlat	byte ptr cs:[bx]
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di]
	lodsb
	mov	byte ptr es:[di], al
L_169D:
	shl	al, 1
	jae	L_1676
	dec	di
	dec	ch
	shl	al, 1
	jae	L_1676
	shl	al, 1
	jb	L_16E2
L_16AC:
	dec	byte ptr cs:[gFlipRows]
	je	L_16E2
	add	bp, SCREEN_COLS
	push	ax
	mov	ax, word ptr cs:[gFlipEnd]
	cmp	bp, ax
	pop	ax
	jae	L_16E2
	mov	di, bp
	pop	bx
	mov	ax, bp
	sub	ax, bx
	cmp	ax, word ptr cs:[gFlipEnd]
	jae	L_1732
	jmp	short L_1670
L_16D0:
	lodsb
	shl	al, 1
	jae	L_1676
	dec	di
	dec	ch
	shl	al, 1
	jae	L_1676
	shl	al, 1
	jb	L_16E2
	jmp	short L_16AC
L_16E2:
	jmp	short L_1731
L_16E4:
	xlat	byte ptr cs:[bx]
	mov	ah, al
	xor	al, al
	shr	ax, cl
	xchg	ah, al
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	cmp	ch, byte ptr cs:[gFlipClipRight]
	jg	L_172E
	cmp	ch, byte ptr cs:[gFlipClipLeft]
	jle	L_172E
	mov	al, byte ptr es:[di]
	lodsb
	mov	byte ptr es:[di], al
L_170A:
	cmp	ch, byte ptr cs:[gFlipClipRight]
	jge	L_172B
	cmp	ch, byte ptr cs:[gFlipClipLeft]
	jl	L_172B
	xchg	ah, al
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di + 1]
	xchg	ah, al
	mov	byte ptr es:[di + 1], al
L_172B:
	jmp	L_169D
L_172E:
	lodsb
	jmp	short L_170A
L_1731:
	pop	bx
L_1732:
	mov	al, 0FFh
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	pop	ds
L_173C:
	pop	si
	pop	di
	pop	es
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
DrawShapeFlipped	ENDP

; EGA.OVL 1746 (export 25, gSaveDrawState): SaveDrawState(void far *regs) keeps the region and sets it to the
; whole screen (0, 40, 0, 200) as MCGA.OVL's does; then keeps the 16 register bytes at regs (the game passes
; cs:00C1, the pointer LinkDriver gave it) at cs:00D1, and copies them to the shadow when regs is another
; pointer. It sets the map mask 0Fh, data rotate 0, read map 0, write mode 2, writes A000:FFFF with the bit mask
; 0 (the latches go to that byte), and leaves the bit mask 0FFh.
SaveDrawState	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	cx
	push	dx
	push	ds
	push	es
	push	di
	push	si
	mov	ax, word ptr cs:[gRegionLeft]
	mov	word ptr cs:[gSavedLeft], ax
	mov	word ptr cs:[gRegionLeft], 0
	mov	ax, word ptr cs:[gRegionRight]
	mov	word ptr cs:[gSavedRight], ax
	mov	word ptr cs:[gRegionRight], SCREEN_COLS
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gSavedTop], ax
	mov	word ptr cs:[gRegionTop], 0
	mov	ax, word ptr cs:[gRegionBottom]
	mov	word ptr cs:[gSavedBottom], ax
	mov	word ptr cs:[gRegionBottom], SCREEN_HEIGHT
	mov	si, word ptr [bp + 6]
	mov	cx, word ptr [bp + 8]
	mov	ds, cx
	mov	di, offset gSavedSeqIndex
	mov	ax, cs
	mov	es, ax
	xor	ax, cx
	mov	cx, offset gSeqIndex
	xor	cx, si
	or	ax, cx
	mov	cx, 10h
	rep movsb
	je	L_17BA
	mov	ax, cs
	mov	ds, ax
	mov	si, offset gSavedSeqIndex
	mov	di, offset gSeqIndex
	mov	cx, 10h
	rep movsb
L_17BA:
	mov	dx, SEQ_INDEX
	cmp	byte ptr cs:[gMapMask], 0Fh
	je	L_17DC
	cmp	byte ptr cs:[gSeqIndex], SEQ_MAP_MASK
	je	L_17D4
	mov	al, SEQ_MAP_MASK
	mov	byte ptr cs:[gSeqIndex], al
	out	dx, al
L_17D4:
	inc	dx
	mov	al, 0Fh
	mov	byte ptr cs:[gMapMask], al
	out	dx, al
L_17DC:
	mov	dl, LOW GC_INDEX
	cmp	byte ptr cs:[gDataRotate], 0
	je	L_17FE
	cmp	byte ptr cs:[gGcIndex], GC_DATA_ROTATE
	je	L_17F5
	mov	al, GC_DATA_ROTATE
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_17F5:
	inc	dx
	mov	al, 0
	mov	byte ptr cs:[gDataRotate], al
	out	dx, al
	dec	dx
L_17FE:
	cmp	byte ptr cs:[gReadMap], 0
	je	L_181E
	cmp	byte ptr cs:[gGcIndex], GC_READ_MAP
	je	L_1815
	mov	al, GC_READ_MAP
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_1815:
	inc	dx
	mov	al, 0
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	dec	dx
L_181E:
	cmp	byte ptr cs:[gGcMode], 2
	je	L_183E
	cmp	byte ptr cs:[gGcIndex], GC_MODE
	je	L_1835
	mov	al, GC_MODE
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_1835:
	inc	dx
	mov	al, 2
	mov	byte ptr cs:[gGcMode], al
	out	dx, al
	dec	dx
L_183E:
	cmp	byte ptr cs:[gBitMask], 0
	je	L_185E
	cmp	byte ptr cs:[gGcIndex], GC_BIT_MASK
	je	L_1855
	mov	al, GC_BIT_MASK
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_1855:
	inc	dx
	mov	al, 0
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	dec	dx
L_185E:
	mov	ax, VIDEO_SEG
	mov	es, ax
	mov	di, LATCH_SAVE
	mov	byte ptr es:[di], al
	cmp	byte ptr cs:[gGcIndex], GC_BIT_MASK
	je	L_1878
	mov	al, GC_BIT_MASK
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
L_1878:
	inc	dx
	mov	al, 0FFh
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	pop	si
	pop	di
	pop	es
	pop	ds
	pop	dx
	pop	cx
	pop	ax
	pop	bp
	retf
SaveDrawState	ENDP

; EGA.OVL 1889 (export 14, gSaveRectFromPage): SaveRect from the page gDrawPage names instead of page 0
SaveRectFromPage	PROC	NEAR
	push	bp
	mov	bp, sp
	sub	sp, 4
	push	ds
	push	si
	mov	ds, word ptr cs:[gDrawPagePtr+2]
	mov	si, word ptr cs:[gDrawPagePtr]
	lodsw
	and	ax, 1Fh
	pop	si
	pop	ds
	mov	word ptr [bp - 2], ax
	jmp	short SaveRectBody
	pop	bp		; QUIRK 18A6: dead code, the jmp above skips it
	retf
SaveRectFromPage	ENDP

; EGA.OVL 18A8 (export 26, gSaveRect): SaveRect(col, y, cols, rows, void far *buf) copies a rectangle of page 0
; into buf, four bytes for each byte (planes 0 to 3, through the read map); returns the bytes written
SaveRect	PROC	NEAR
	push	bp
	mov	bp, sp
	sub	sp, 4
	mov	ax, 0
	mov	word ptr [bp - 2], ax
SaveRectBody:
	push	bx
	push	cx
	push	dx
	push	di
	push	si
	push	ds
	push	es
	cld
	mov	si, word ptr [bp + 6]
	cmp	si, 0
	jge	L_18C6
	xor	si, si
L_18C6:
	cmp	si, SCREEN_COLS
	jl	L_18CE
	mov	si, SCREEN_COLS - 1
L_18CE:
	mov	dx, word ptr [bp + 8]
	cmp	dx, 0
	jge	L_18D8
	xor	dx, dx
L_18D8:
	cmp	dx, SCREEN_HEIGHT
	jl	L_18E1
	mov	dx, SCREEN_HEIGHT - 1
L_18E1:
	mov	bx, word ptr [bp + 0Ah]
	cmp	bx, 0
	jg	L_18EE
L_18E9:
	xor	ax, ax
	jmp	L_1999
L_18EE:
	cmp	bx, SCREEN_COLS
	jle	L_18F6
	mov	bx, SCREEN_COLS
L_18F6:
	mov	cx, word ptr [bp + 0Ch]
	cmp	cx, 0
	jle	L_18E9
	cmp	cx, SCREEN_HEIGHT
	jle	L_1907
	mov	cx, SCREEN_HEIGHT
L_1907:
	mov	di, word ptr [bp + 0Eh]
	push	di
	mov	es, word ptr [bp + 10h]
	cmp	bx, SCREEN_COLS
	jle	L_1916
	mov	bx, SCREEN_COLS
L_1916:
	or	bx, bx
	jne	L_191C
L_191A:
	jmp	short L_1994
L_191C:
	cmp	cx, SCREEN_HEIGHT
	jbe	L_1925
	mov	cx, SCREEN_HEIGHT
L_1925:
	or	cx, cx
	je	L_191A
	push	bx
	mov	bx, dx
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	si, ax
	pop	bx
	mov	ax, word ptr [bp - 2]
	call	PageSeg
	mov	ds, ax
	mov	dx, GC_INDEX
	mov	al, GC_READ_MAP
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	sti
	inc	dx
	mov	ah, cl
L_194D:
	mov	cl, bl
L_194F:
	xor	al, al
	cli
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	sti
	mov	al, byte ptr [si]
	stosb
	mov	al, 1
	cli
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	sti
	mov	al, byte ptr [si]
	stosb
	mov	al, 2
	cli
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	sti
	mov	al, byte ptr [si]
	stosb
	mov	al, 3
	cli
	mov	byte ptr cs:[gReadMap], al
	out	dx, al
	sti
	movsb
	dec	cl
	jne	L_194F
	sub	si, bx
	add	si, SCREEN_COLS
	dec	ah
	jne	L_194D
	dec	dx
	mov	al, GC_BIT_MASK
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	sti
L_1994:
	mov	ax, di
	pop	di
	sub	ax, di
L_1999:
	pop	es
	pop	ds
	pop	si
	pop	di
	pop	dx
	pop	cx
	pop	bx
	mov	sp, bp
	pop	bp
	retf
SaveRect	ENDP

; EGA.OVL 19A4 (export 27, ds:80B2): SetLineClip(left, top, right, bottom) sets the rectangle DrawLine clips
; to, in pixels; no call of ds:80B2 was found
SetLineClip	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	mov	ax, word ptr [bp + 6]
	mov	word ptr cs:[gLineClipLeft], ax
	mov	ax, word ptr [bp + 8]
	mov	word ptr cs:[gLineClipTop], ax
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gLineClipRight], ax
	mov	ax, word ptr [bp + 0Ch]
	mov	word ptr cs:[gLineClipBottom], ax
	pop	ax
	pop	bp
	retf
SetLineClip	ENDP

; EGA.OVL 19C7 (export 28, gSetRegion): SetRegion(left, right, top, bottom): the region the shape draws clip
; to, left and right in 8-pixel columns, top and bottom in lines
SetRegion	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	mov	ax, word ptr [bp + 6]
	mov	word ptr cs:[gRegionLeft], ax
	mov	ax, word ptr [bp + 8]
	mov	word ptr cs:[gRegionRight], ax
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gRegionTop], ax
	mov	ax, word ptr [bp + 0Ch]
	mov	word ptr cs:[gRegionBottom], ax
	pop	ax
	pop	bp
	retf
SetRegion	ENDP

; EGA.OVL 19EA (export 29, gSetPalette): SetPalette(void far *colours) loads the 16 palette registers of the
; attribute controller (port 3C0h) from 16 bytes, values above 7 moved up by 8 (to the intense colours 10h to
; 17h); the flip-flop is reset by a read of the input status port (the CRTC's base from 0040:0063, + 6), and 20h
; turns the display back on after each
SetPalette	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	ds
	push	es
	mov	si, word ptr [bp + 6]
	mov	ds, word ptr [bp + 8]
	mov	cx, 0
L_19FD:
	push	cx
	mov	bx, cx
	mov	cl, byte ptr [si]
	mov	ch, 0
	cmp	cl, 7
	jle	L_1A0C
	add	cl, 8
L_1A0C:
	inc	si
	mov	ax, 40h
	mov	es, ax
	mov	dx, word ptr es:[63h]
	add	dl, 6
	push	dx
	cli
	in	al, dx
	mov	dl, SCREEN_HEIGHT - 8
	mov	al, bl
	out	dx, al
	mov	al, cl
	out	dx, al
	pop	dx
	in	al, dx
	mov	dl, SCREEN_HEIGHT - 8
	mov	al, 20h
	out	dx, al
	sti
	pop	cx
	inc	cx
	cmp	cx, 10h
	jl	L_19FD
	pop	es
	pop	ds
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
SetPalette	ENDP

; EGA.OVL 1A3D (export 30, ds:80BE): SetTileFormat returns at once; no call of ds:80BE was found
SetTileFormat	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
SetTileFormat	ENDP

; EGA.OVL 1A42 (export 31, gDrawShape): DrawShape(page, void far *shape, x, y) draws a GetSpriteCell cell at
; pixel (x, y), clipped to the region: for each (mask, colour) pair the bit mask, a read of the byte, the colour;
; at an x that is not a multiple of 8 each mask is shifted over two bytes. Rows above the region are skipped by
; their end flags.
DrawShape	PROC	NEAR
	push	bp
	mov	bp, sp
	sub	sp, 10h
	push	ax
	push	bx
	push	cx
	push	dx
	push	ds
	push	es
	push	di
	push	si
	cld
	mov	ax, word ptr cs:[gRegionBottom]
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	mov	word ptr [bp - 4], ax
	xor	di, di
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	es, ax
	mov	si, word ptr [bp + 8]
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr [bp - 0Ah], ax
	mov	ax, word ptr [bp + 0Ch]
	mov	word ptr [bp - 6], ax
	sar	ax, 1
	sar	ax, 1
	sar	ax, 1
	mov	word ptr [bp - 8], ax
	mov	dx, word ptr [bp + 0Eh]
	mov	ax, word ptr cs:[gRegionLeft]
	dec	ax
	mov	word ptr [bp - 10h], ax
	mov	ax, word ptr cs:[gRegionRight]
	dec	ax
	mov	word ptr [bp - 0Eh], ax
	mov	word ptr [bp - 0Ch], ds
	mov	ax, word ptr [bp - 0Ah]
	mov	ds, ax
	inc	si
	lodsw
	mov	bx, ax
	and	bx, 0FFh
	inc	bx
	mov	byte ptr [bp - 2], bl
	inc	si
	cmp	dx, word ptr cs:[gRegionBottom]
	jge	L_1AC2
	mov	cx, dx
	sub	cx, word ptr cs:[gRegionTop]
	jns	L_1AD7
	neg	cx
	dec	bx
	cmp	cx, bx
	jl	L_1AC5
L_1AC2:
	jmp	L_1BB8
L_1AC5:
	lodsw
	test	ah, 40h
	je	L_1AC5
	dec	byte ptr [bp - 2]
	je	L_1AC2
	loop	L_1AC5
	mov	dx, word ptr cs:[gRegionTop]
L_1AD7:
	mov	ds, word ptr [bp - 0Ch]
	mov	bx, dx
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	di, ax
	add	di, word ptr [bp - 8]
	mov	cx, word ptr [bp - 6]
	and	cx, 7
	mov	bx, word ptr [bp - 8]
	mov	dx, GC_DATA
	mov	ax, word ptr [bp - 0Ah]
	push	ds
	mov	ds, ax
	mov	word ptr [bp - 0Ch], di
L_1AFD:
	push	bx
L_1AFE:
	lodsb
	or	al, al
	je	L_1B52
	or	cl, cl
	jne	L_1B66
	cmp	bx, word ptr [bp - 0Eh]
	jg	L_1B63
	cmp	bx, word ptr [bp - 10h]
	jle	L_1B63
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di]
	lodsb
	mov	byte ptr es:[di], al
L_1B1F:
	shl	al, 1
	jae	L_1AFE
	inc	di
	inc	bx
	shl	al, 1
	jae	L_1AFE
	shl	al, 1
	jb	L_1B50
L_1B2D:
	dec	byte ptr [bp - 2]
	je	L_1B50
	add	word ptr [bp - 0Ch], SCREEN_COLS
	push	ax
	mov	ax, word ptr [bp - 0Ch]
	cmp	ax, word ptr [bp - 4]
	pop	ax
	jae	L_1B50
	mov	di, word ptr [bp - 0Ch]
	pop	bx
	mov	ax, word ptr [bp - 0Ch]
	sub	ax, bx
	cmp	ax, word ptr [bp - 4]
	jae	L_1BAE
	jmp	short L_1AFD
L_1B50:
	jmp	short L_1BAD
L_1B52:
	lodsb
	shl	al, 1
	jae	L_1AFE
	inc	di
	inc	bx
	shl	al, 1
	jae	L_1AFE
	shl	al, 1
	jb	L_1BAD
	jmp	short L_1B2D
L_1B63:
	lodsb
	jmp	short L_1B1F
L_1B66:
	xor	ah, ah
	xchg	ah, al
	shr	ax, cl
	cmp	bx, word ptr [bp - 0Eh]
	jg	L_1BA8
	cmp	bx, word ptr [bp - 10h]
	jle	L_1BA8
	mov	ch, al
	mov	al, byte ptr es:[di]
	mov	al, ah
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	lodsb
	mov	byte ptr es:[di], al
L_1B88:
	cmp	bx, word ptr [bp - 0Eh]
	jge	L_1B1F
	cmp	bx, word ptr [bp - 10h]
	jl	L_1B1F
	mov	ah, byte ptr es:[di + 1]
	xchg	ch, al
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	xchg	ch, al
	mov	byte ptr es:[di + 1], al
	jmp	L_1B1F
L_1BA8:
	mov	ch, al
	lodsb
	jmp	short L_1B88
L_1BAD:
	pop	bx
L_1BAE:
	mov	al, 0FFh
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	pop	ds
L_1BB8:
	pop	si
	pop	di
	pop	es
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	mov	sp, bp
	pop	bp
	retf
DrawShape	ENDP

; EGA.OVL 1BC4 (export 32, gSaveRectSize): SaveRectSize(cols, rows) returns cols * rows * 4, the bytes SaveRect
; needs (cols clamped to 1 to 40, rows to 1 to 200)
SaveRectSize	PROC	NEAR
	push	bp
	mov	bp, sp
	push	dx
	mov	ax, word ptr [bp + 6]
	cmp	ax, 1
	jge	L_1BD3
	mov	ax, 1
L_1BD3:
	cmp	ax, SCREEN_COLS
	jle	L_1BDB
	mov	ax, SCREEN_COLS
L_1BDB:
	mov	dx, word ptr [bp + 8]
	cmp	dx, 1
	jge	L_1BE6
	mov	dx, 1
L_1BE6:
	cmp	dx, SCREEN_HEIGHT
	jle	L_1BEF
	mov	dx, SCREEN_HEIGHT
L_1BEF:
	mul	dx
	shl	ax, 1
	shl	ax, 1
	pop	dx
	pop	bp
	retf
SaveRectSize	ENDP

; cs:1BF8: DrawGlyph's and DrawText's state
gTextRowSkip	dw	0		; screen bytes from a glyph row's end to the next
	db	6 dup (0)		; 1BFA: not used
gGlyphRowBytes	dw	0		; the glyph's bytes in a row
	db	2 dup (0)		; 1C02: not used
gGlyphX	db	0		; x, then x AND 7
gGlyphY	db	0		; y
gTextColour	db	0		; DrawText's colours, AND 0Fh
gBackColour	db	0
	db	4 dup (0)		; 1C08: not used
gTextLeft	dw	0		; DrawText: the x a line starts at
gLineHeight	dw	0		; the font's height
gCharWidth	dw	0		; the character's width
	db	2 dup (0)		; 1C12: not used
gGlyphBytes	db	0		; DrawGlyph: the glyph's bytes in a row, less one
gFontSeg	dw	0		; the font's segment

; cs:1C17: for each width of 1 to 24 pixels, a set of eight bit masks, one per starting bit, MSB first:
; gBitMaskTable[w - 1] -> gMaskSet<w>, whose word p points at the mask bytes for a span of w bits
; starting at bit p (DrawGlyph's glyph rows, XorRect's first byte)
gBitMaskTable LABEL WORD
	dw	gMaskSet1, gMaskSet2, gMaskSet3, gMaskSet4, gMaskSet5, gMaskSet6
	dw	gMaskSet7, gMaskSet8, gMaskSet9, gMaskSet10, gMaskSet11, gMaskSet12
	dw	gMaskSet13, gMaskSet14, gMaskSet15, gMaskSet16, gMaskSet17, gMaskSet18
	dw	gMaskSet19, gMaskSet20, gMaskSet21, gMaskSet22, gMaskSet23, gMaskSet24
gMaskSet1	dw	Mask1_0, Mask1_1, Mask1_2, Mask1_3, Mask1_4, Mask1_5, Mask1_6, Mask1_7
Mask1_0	db	80h
Mask1_1	db	40h
Mask1_2	db	20h
Mask1_3	db	10h
Mask1_4	db	8
Mask1_5	db	4
Mask1_6	db	2
Mask1_7	db	1
gMaskSet2	dw	Mask2_0, Mask2_1, Mask2_2, Mask2_3, Mask2_4, Mask2_5, Mask2_6, Mask2_7
Mask2_0	db	0C0h, 0
Mask2_1	db	60h, 0
Mask2_2	db	30h, 0
Mask2_3	db	18h, 0
Mask2_4	db	0Ch, 0
Mask2_5	db	6, 0
Mask2_6	db	3, 0
Mask2_7	db	1, 80h
gMaskSet3	dw	Mask3_0, Mask3_1, Mask3_2, Mask3_3, Mask3_4, Mask3_5, Mask3_6, Mask3_7
Mask3_0	db	0E0h, 0
Mask3_1	db	70h, 0
Mask3_2	db	38h, 0
Mask3_3	db	1Ch, 0
Mask3_4	db	0Eh, 0
Mask3_5	db	7, 0
Mask3_6	db	3, 80h
Mask3_7	db	1, 0C0h
gMaskSet4	dw	Mask4_0, Mask4_1, Mask4_2, Mask4_3, Mask4_4, Mask4_5, Mask4_6, Mask4_7
Mask4_0	db	0F0h, 0
Mask4_1	db	78h, 0
Mask4_2	db	3Ch, 0
Mask4_3	db	1Eh, 0
Mask4_4	db	0Fh, 0
Mask4_5	db	7, 80h
Mask4_6	db	3, 0C0h
Mask4_7	db	1, 0E0h
gMaskSet5	dw	Mask5_0, Mask5_1, Mask5_2, Mask5_3, Mask5_4, Mask5_5, Mask5_6, Mask5_7
Mask5_0	db	0F8h, 0
Mask5_1	db	7Ch, 0
Mask5_2	db	3Eh, 0
Mask5_3	db	1Fh, 0
Mask5_4	db	0Fh, 80h
Mask5_5	db	7, 0C0h
Mask5_6	db	3, 0E0h
Mask5_7	db	1, 0F0h
gMaskSet6	dw	Mask6_0, Mask6_1, Mask6_2, Mask6_3, Mask6_4, Mask6_5, Mask6_6, Mask6_7
Mask6_0	db	0FCh, 0
Mask6_1	db	7Eh, 0
Mask6_2	db	3Fh, 0
Mask6_3	db	1Fh, 80h
Mask6_4	db	0Fh, 0C0h
Mask6_5	db	7, 0E0h
Mask6_6	db	3, 0F0h
Mask6_7	db	1, 0F8h
gMaskSet7	dw	Mask7_0, Mask7_1, Mask7_2, Mask7_3, Mask7_4, Mask7_5, Mask7_6, Mask7_7
Mask7_0	db	0FEh, 0
Mask7_1	db	7Fh, 0
Mask7_2	db	3Fh, 80h
Mask7_3	db	1Fh, 0C0h
Mask7_4	db	0Fh, 0E0h
Mask7_5	db	7, 0F0h
Mask7_6	db	3, 0F8h
Mask7_7	db	1, 0FCh
gMaskSet8	dw	Mask8_0, Mask8_1, Mask8_2, Mask8_3, Mask8_4, Mask8_5, Mask8_6, Mask8_7
Mask8_0	db	0FFh, 0
Mask8_1	db	7Fh, 80h
Mask8_2	db	3Fh, 0C0h
Mask8_3	db	1Fh, 0E0h
Mask8_4	db	0Fh, 0F0h
Mask8_5	db	7, 0F8h
Mask8_6	db	3, 0FCh
Mask8_7	db	1, 0FEh
gMaskSet9	dw	Mask9_0, Mask9_1, Mask9_2, Mask9_3, Mask9_4, Mask9_5, Mask9_6, Mask9_7
Mask9_0	db	0FFh, 80h
Mask9_1	db	7Fh, 0C0h
Mask9_2	db	3Fh, 0E0h
Mask9_3	db	1Fh, 0F0h
Mask9_4	db	0Fh, 0F8h
Mask9_5	db	7, 0FCh
Mask9_6	db	3, 0FEh
Mask9_7	db	1, 0FFh
gMaskSet10	dw	Mask10_0, Mask10_1, Mask10_2, Mask10_3, Mask10_4, Mask10_5, Mask10_6, Mask10_7
Mask10_0	db	0FFh, 0C0h, 0
Mask10_1	db	7Fh, 0E0h, 0
Mask10_2	db	3Fh, 0F0h, 0
Mask10_3	db	1Fh, 0F8h, 0
Mask10_4	db	0Fh, 0FCh, 0
Mask10_5	db	7, 0FEh, 0
Mask10_6	db	3, 0FFh, 0
Mask10_7	db	1, 0FFh, 80h
gMaskSet11	dw	Mask11_0, Mask11_1, Mask11_2, Mask11_3, Mask11_4, Mask11_5, Mask11_6, Mask11_7
Mask11_0	db	0FFh, 0E0h, 0
Mask11_1	db	7Fh, 0F0h, 0
Mask11_2	db	3Fh, 0F8h, 0
Mask11_3	db	1Fh, 0FCh, 0
Mask11_4	db	0Fh, 0FEh, 0
Mask11_5	db	7, 0FFh, 0
Mask11_6	db	3, 0FFh, 80h
Mask11_7	db	1, 0FFh, 0C0h
gMaskSet12	dw	Mask12_0, Mask12_1, Mask12_2, Mask12_3, Mask12_4, Mask12_5, Mask12_6, Mask12_7
Mask12_0	db	0FFh, 0F0h, 0
Mask12_1	db	7Fh, 0F8h, 0
Mask12_2	db	3Fh, 0FCh, 0
Mask12_3	db	1Fh, 0FEh, 0
Mask12_4	db	0Fh, 0FFh, 0
Mask12_5	db	7, 0FFh, 80h
Mask12_6	db	3, 0FFh, 0C0h
Mask12_7	db	1, 0FFh, 0E0h
gMaskSet13	dw	Mask13_0, Mask13_1, Mask13_2, Mask13_3, Mask13_4, Mask13_5, Mask13_6, Mask13_7
Mask13_0	db	0FFh, 0F8h, 0
Mask13_1	db	7Fh, 0FCh, 0
Mask13_2	db	3Fh, 0FEh, 0
Mask13_3	db	1Fh, 0FFh, 0
Mask13_4	db	0Fh, 0FFh, 80h
Mask13_5	db	7, 0FFh, 0C0h
Mask13_6	db	3, 0FFh, 0E0h
Mask13_7	db	1, 0FFh, 0F0h
gMaskSet14	dw	Mask14_0, Mask14_1, Mask14_2, Mask14_3, Mask14_4, Mask14_5, Mask14_6, Mask14_7
Mask14_0	db	0FFh, 0FCh, 0
Mask14_1	db	7Fh, 0FEh, 0
Mask14_2	db	3Fh, 0FFh, 0
Mask14_3	db	1Fh, 0FFh, 80h
Mask14_4	db	0Fh, 0FFh, 0C0h
Mask14_5	db	7, 0FFh, 0E0h
Mask14_6	db	3, 0FFh, 0F0h
Mask14_7	db	1, 0FFh, 0F8h
gMaskSet15	dw	Mask15_0, Mask15_1, Mask15_2, Mask15_3, Mask15_4, Mask15_5, Mask15_6, Mask15_7
Mask15_0	db	0FFh, 0FEh, 0
Mask15_1	db	7Fh, 0FFh, 0
Mask15_2	db	3Fh, 0FFh, 80h
Mask15_3	db	1Fh, 0FFh, 0C0h
Mask15_4	db	0Fh, 0FFh, 0E0h
Mask15_5	db	7, 0FFh, 0F0h
Mask15_6	db	3, 0FFh, 0F8h
Mask15_7	db	1, 0FFh, 0FCh
gMaskSet16	dw	Mask16_0, Mask16_1, Mask16_2, Mask16_3, Mask16_4, Mask16_5, Mask16_6, Mask16_7
Mask16_0	db	0FFh, 0FFh, 0
Mask16_1	db	7Fh, 0FFh, 80h
Mask16_2	db	3Fh, 0FFh, 0C0h
Mask16_3	db	1Fh, 0FFh, 0E0h
Mask16_4	db	0Fh, 0FFh, 0F0h
Mask16_5	db	7, 0FFh, 0F8h
Mask16_6	db	3, 0FFh, 0FCh
Mask16_7	db	1, 0FFh, 0FEh
gMaskSet17	dw	Mask17_0, Mask17_1, Mask17_2, Mask17_3, Mask17_4, Mask17_5, Mask17_6, Mask17_7
Mask17_0	db	0FFh, 0FFh, 80h
Mask17_1	db	7Fh, 0FFh, 0C0h
Mask17_2	db	3Fh, 0FFh, 0E0h
Mask17_3	db	1Fh, 0FFh, 0F0h
Mask17_4	db	0Fh, 0FFh, 0F8h
Mask17_5	db	7, 0FFh, 0FCh
Mask17_6	db	3, 0FFh, 0FEh
Mask17_7	db	1, 0FFh, 0FFh
gMaskSet18	dw	Mask18_0, Mask18_1, Mask18_2, Mask18_3, Mask18_4, Mask18_5, Mask18_6, Mask18_7
Mask18_0	db	0FFh, 0FFh, 0C0h, 0
Mask18_1	db	7Fh, 0FFh, 0E0h, 0
Mask18_2	db	3Fh, 0FFh, 0F0h, 0
Mask18_3	db	1Fh, 0FFh, 0F8h, 0
Mask18_4	db	0Fh, 0FFh, 0FCh, 0
Mask18_5	db	7, 0FFh, 0FEh, 0
Mask18_6	db	3, 0FFh, 0FFh, 0
Mask18_7	db	1, 0FFh, 0FFh, 80h
gMaskSet19	dw	Mask19_0, Mask19_1, Mask19_2, Mask19_3, Mask19_4, Mask19_5, Mask19_6, Mask19_7
Mask19_0	db	0FFh, 0FFh, 0E0h, 0
Mask19_1	db	7Fh, 0FFh, 0F0h, 0
Mask19_2	db	3Fh, 0FFh, 0F8h, 0
Mask19_3	db	1Fh, 0FFh, 0FCh, 0
Mask19_4	db	0Fh, 0FFh, 0FEh, 0
Mask19_5	db	7, 0FFh, 0FFh, 0
Mask19_6	db	3, 0FFh, 0FFh, 80h
Mask19_7	db	1, 0FFh, 0FFh, 0C0h
gMaskSet20	dw	Mask20_0, Mask20_1, Mask20_2, Mask20_3, Mask20_4, Mask20_5, Mask20_6, Mask20_7
Mask20_0	db	0FFh, 0FFh, 0F0h, 0
Mask20_1	db	7Fh, 0FFh, 0F8h, 0
Mask20_2	db	3Fh, 0FFh, 0FCh, 0
Mask20_3	db	1Fh, 0FFh, 0FEh, 0
Mask20_4	db	0Fh, 0FFh, 0FFh, 0
Mask20_5	db	7, 0FFh, 0FFh, 80h
Mask20_6	db	3, 0FFh, 0FFh, 0C0h
Mask20_7	db	1, 0FFh, 0FFh, 0E0h
gMaskSet21	dw	Mask21_0, Mask21_1, Mask21_2, Mask21_3, Mask21_4, Mask21_5, Mask21_6, Mask21_7
Mask21_0	db	0FFh, 0FFh, 0F8h, 0
Mask21_1	db	7Fh, 0FFh, 0FCh, 0
Mask21_2	db	3Fh, 0FFh, 0FEh, 0
Mask21_3	db	1Fh, 0FFh, 0FFh, 0
Mask21_4	db	0Fh, 0FFh, 0FFh, 80h
Mask21_5	db	7, 0FFh, 0FFh, 0C0h
Mask21_6	db	3, 0FFh, 0FFh, 0E0h
Mask21_7	db	1, 0FFh, 0FFh, 0F0h
gMaskSet22	dw	Mask22_0, Mask22_1, Mask22_2, Mask22_3, Mask22_4, Mask22_5, Mask22_6, Mask22_7
Mask22_0	db	0FFh, 0FFh, 0FCh, 0
Mask22_1	db	7Fh, 0FFh, 0FEh, 0
Mask22_2	db	3Fh, 0FFh, 0FFh, 0
Mask22_3	db	1Fh, 0FFh, 0FFh, 80h
Mask22_4	db	0Fh, 0FFh, 0FFh, 0C0h
Mask22_5	db	7, 0FFh, 0FFh, 0E0h
Mask22_6	db	3, 0FFh, 0FFh, 0F0h
Mask22_7	db	1, 0FFh, 0FFh, 0F8h
gMaskSet23	dw	Mask23_0, Mask23_1, Mask23_2, Mask23_3, Mask23_4, Mask23_5, Mask23_6, Mask23_7
Mask23_0	db	0FFh, 0FFh, 0FEh, 0
Mask23_1	db	7Fh, 0FFh, 0FFh, 0
Mask23_2	db	3Fh, 0FFh, 0FFh, 80h
Mask23_3	db	1Fh, 0FFh, 0FFh, 0C0h
Mask23_4	db	0Fh, 0FFh, 0FFh, 0E0h
Mask23_5	db	7, 0FFh, 0FFh, 0F0h
Mask23_6	db	3, 0FFh, 0FFh, 0F8h
Mask23_7	db	1, 0FFh, 0FFh, 0FCh
gMaskSet24	dw	Mask24_0, Mask24_1, Mask24_2, Mask24_3, Mask24_4, Mask24_5, Mask24_6, Mask24_7
Mask24_0	db	0FFh, 0FFh, 0FFh, 0
Mask24_1	db	7Fh, 0FFh, 0FFh, 80h
Mask24_2	db	3Fh, 0FFh, 0FFh, 0C0h
Mask24_3	db	1Fh, 0FFh, 0FFh, 0E0h
Mask24_4	db	0Fh, 0FFh, 0FFh, 0F0h
Mask24_5	db	7, 0FFh, 0FFh, 0F8h
Mask24_6	db	3, 0FFh, 0FFh, 0FCh
Mask24_7	db	1, 0FFh, 0FFh, 0FEh

; EGA.OVL 1FEF (near): DrawGlyph(ch, x, y), as MCGA.OVL 241E: each glyph row's bits, shifted by x AND 7 and
; cut to the glyph's width by gBitMaskTable, are the bit mask for gTextColour, their complement for gBackColour,
; each drawn unless its colour is 0
DrawGlyph	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	ds
	push	di
	push	si
	push	es
	mov	es, word ptr cs:[gCurFontPtr+2]
	mov	si, word ptr cs:[gCurFontPtr]
	mov	ax, word ptr es:[si + 2]
	mov	si, word ptr es:[si]
	mov	es, ax
	mov	bx, word ptr [bp + 4]
	inc	bx
	shl	bx, 1
	mov	bx, word ptr es:[bx + si]
	mov	dx, word ptr [bp + 6]
	cmp	dx, SCREEN_WIDTH
	jb	L_2021
	jmp	short L_207F
L_2021:
	mov	ax, dx
	mov	byte ptr cs:[gGlyphX], al
	mov	cl, byte ptr es:[si + FONT_WIDTH]
	mov	byte ptr cs:[gGlyphWidth], cl
	add	ax, cx
	cmp	ax, SCREEN_WIDTH
	ja	L_207F
	mov	di, dx
	mov	dx, word ptr [bp + 8]
	cmp	dx, SCREEN_HEIGHT
	jae	L_207F
	mov	ax, dx
	mov	byte ptr cs:[gGlyphY], al
	mov	cl, byte ptr es:[si + FONT_HEIGHT]
	mov	byte ptr cs:[gGlyphRows], cl
	add	ax, cx
	cmp	ax, SCREEN_HEIGHT
	ja	L_207F
	add	si, bx
	xor	ah, ah
	mov	al, byte ptr cs:[gGlyphWidth]
	dec	al
	cmp	al, 18h
	jae	L_207F
	mov	bp, ax
	add	al, 7
	shr	al, 1
	shr	al, 1
	shr	al, 1
	mov	byte ptr cs:[gGlyphBytes], al
	shl	bp, 1
	mov	bp, word ptr cs:[bp + gBitMaskTable]
	jmp	short L_2082
L_207F:
	jmp	L_213F
L_2082:
	and	byte ptr cs:[gGlyphX], 7
	xor	ah, ah
	mov	al, byte ptr cs:[gGlyphX]
	shl	ax, 1
	add	bp, ax
	mov	bp, word ptr cs:[bp]
	shr	di, 1
	shr	di, 1
	shr	di, 1
	mov	ax, dx
	push	bx
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	pop	bx
	add	di, ax
	mov	ax, SCREEN_COLS
	sub	al, byte ptr cs:[gGlyphBytes]
	mov	word ptr cs:[gTextRowSkip], ax
	xor	ah, ah
	mov	al, byte ptr cs:[gGlyphBytes]
	mov	word ptr cs:[gGlyphRowBytes], ax
	mov	dx, es
	mov	ds, dx
	call	DrawPageSeg
	mov	es, ax
	mov	bl, byte ptr cs:[gTextColour]
	mov	bh, byte ptr cs:[gBackColour]
L_20D4:
	mov	ch, byte ptr cs:[gGlyphBytes]
	xor	ax, ax
L_20DB:
	mov	dl, ah
	xor	al, al
	mov	ah, byte ptr [si]
	mov	cl, byte ptr cs:[gGlyphX]
	shr	ax, cl
	xchg	al, ah
	or	al, dl
	mov	dx, GC_DATA
	or	bl, bl
	je	L_2103
	and	al, byte ptr cs:[bp]
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	cl, bl
	xchg	byte ptr es:[di], cl
L_2103:
	or	bh, bh
	je	L_2119
	not	al
	and	al, byte ptr cs:[bp]
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	cl, bh
	xchg	byte ptr es:[di], cl
L_2119:
	cmp	ch, 0
	je	L_2125
	inc	di
	inc	si
	inc	bp
	dec	ch
	jmp	short L_20DB
L_2125:
	sub	bp, word ptr cs:[gGlyphRowBytes]
	add	di, word ptr cs:[gTextRowSkip]
	dec	byte ptr cs:[gGlyphRows]
	jne	L_20D4
	mov	al, 0FFh
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
L_213F:
	pop	es
	pop	si
	pop	di
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	ret
DrawGlyph	ENDP

; EGA.OVL 2149 (export 33, gDrawText): DrawText(char far *text, x, y, colour, background), as MCGA.OVL's: a
; carriage return goes back to x on the next line, a character that would pass the right edge starts a new
; line
DrawText	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	dx
	push	ds
	push	es
	push	di
	push	si
	push	cx
	mov	word ptr cs:[gTextRight], SCREEN_WIDTH
	mov	es, word ptr cs:[gCurFontPtr+2]
	mov	di, word ptr cs:[gCurFontPtr]
	mov	ax, word ptr es:[di + 2]
	mov	di, word ptr es:[di]
	mov	es, ax
	mov	word ptr cs:[gFontSeg], es
	xor	ah, ah
	mov	al, byte ptr es:[di + FONT_HEIGHT]
	mov	word ptr cs:[gLineHeight], ax
	mov	si, word ptr [bp + 6]
	mov	ax, word ptr [bp + 8]
	mov	es, ax
	mov	bx, word ptr [bp + 0Ah]
	cmp	bx, 0
	jl	L_2196
	cmp	bx, SCREEN_WIDTH
	jge	L_219F
	jmp	short L_2198
L_2196:
	xor	bx, bx
L_2198:
	cmp	bx, word ptr cs:[gTextRight]
	jb	L_21A2
L_219F:
	jmp	L_222A
L_21A2:
	mov	word ptr cs:[gTextLeft], bx
	mov	cx, word ptr [bp + 0Ch]
	cmp	cx, 0
	jl	L_21B7
	cmp	cx, SCREEN_HEIGHT
	jge	L_222A
	jmp	short L_21B9
L_21B7:
	xor	cx, cx
L_21B9:
	mov	al, byte ptr [bp + 0Eh]
	and	al, 0Fh
	mov	byte ptr cs:[gTextColour], al
	mov	al, byte ptr [bp + 10h]
	and	al, 0Fh
	mov	byte ptr cs:[gBackColour], al
L_21CB:
	cmp	byte ptr es:[si], 0
	je	L_222A
	cmp	byte ptr es:[si], 0Dh
	jne	L_21E4
	mov	bx, word ptr cs:[gTextLeft]
	add	cx, word ptr cs:[gLineHeight]
	inc	si
	jmp	short L_21CB
L_21E4:
	push	es
	mov	es, word ptr cs:[gFontSeg]
	xor	ah, ah
	mov	al, byte ptr es:[di + FONT_WIDTH]
	mov	word ptr cs:[gCharWidth], ax
	mov	dx, bx
	add	dx, ax
	pop	es
	cmp	dx, word ptr cs:[gTextRight]
	jbe	L_220B
	mov	bx, word ptr cs:[gTextLeft]
	add	cx, word ptr cs:[gLineHeight]
L_220B:
	cmp	cx, SCREEN_HEIGHT
	jae	L_222A
	push	cx
	push	bx
	mov	al, byte ptr es:[si]
	and	ax, 7Fh
	push	ax
	call	DrawGlyph
	add	sp, 2
	pop	bx
	pop	cx
	add	bx, word ptr cs:[gCharWidth]
	inc	si
	jmp	short L_21CB
L_222A:
	pop	cx
	pop	si
	pop	di
	pop	es
	pop	ds
	pop	dx
	pop	bx
	pop	ax
	pop	bp
	retf
DrawText	ENDP

; EGA.OVL 2234 (export 8, gXorRect): XorRect(x1, y1, x2, y2, colour) XORs colour into every pixel of the
; rectangle on the draw page: the data rotate register's XOR function, write mode 2, the first and last bytes
; through gBitMaskTable
XorRect	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	di
	push	es
	mov	di, word ptr [bp + 6]
	cmp	di, 0
	jge	L_2247
	xor	di, di
L_2247:
	cmp	di, SCREEN_WIDTH
	jl	L_2250
	mov	di, SCREEN_WIDTH - 1
L_2250:
	mov	dx, word ptr [bp + 8]
	cmp	dx, 0
	jge	L_225A
	xor	dx, dx
L_225A:
	cmp	dx, SCREEN_HEIGHT
	jl	L_2263
	mov	dx, SCREEN_HEIGHT - 1
L_2263:
	mov	ax, word ptr [bp + 0Ah]
	cmp	ax, 0
	jge	L_226D
	xor	ax, ax
L_226D:
	cmp	ax, SCREEN_WIDTH
	jl	L_2275
	mov	ax, SCREEN_WIDTH - 1
L_2275:
	cmp	ax, di
	jge	L_227A
	xchg	di, ax
L_227A:
	mov	bx, di
	and	bl, 7
	mov	byte ptr cs:[gXorLeftMask], bl
	mov	bh, al
	and	bh, 7
	sub	ax, di
	inc	ax
	mov	cx, ax
	cmp	ax, 8
	jle	L_229D
	xor	ch, ch
	mov	cl, 8
	sub	cl, bl
	add	cl, bh
	inc	cl
L_229D:
	mov	word ptr cs:[gXorFirstPixels], cx
	sub	ax, cx
	shr	ax, 1
	shr	ax, 1
	shr	ax, 1
	mov	byte ptr cs:[gXorMiddleBytes], al
	mov	ax, word ptr [bp + 0Ch]
	cmp	ax, 0
	jge	L_22B8
	xor	ax, ax
L_22B8:
	cmp	ax, SCREEN_HEIGHT
	jl	L_22C0
	mov	ax, SCREEN_HEIGHT - 1
L_22C0:
	cmp	ax, dx
	jge	L_22C5
	xchg	dx, ax
L_22C5:
	mov	byte ptr cs:[gXorTop], dl
	sub	ax, dx
	inc	ax
	mov	byte ptr cs:[gXorRows], al
	mov	ax, word ptr [bp + 0Eh]
	and	al, 0Fh
	mov	byte ptr cs:[gXorColour], al
	mov	cx, word ptr cs:[gXorFirstPixels]
	dec	cx
	mov	bp, cx
	add	cl, 7
	shr	cl, 1
	shr	cl, 1
	shr	cl, 1
	shl	bp, 1
	mov	bp, word ptr cs:[bp + gBitMaskTable]
	shr	di, 1
	shr	di, 1
	shr	di, 1
	xor	ah, ah
	mov	al, byte ptr cs:[gXorLeftMask]
	shl	ax, 1
	add	bp, ax
	mov	bp, word ptr cs:[bp]
	mov	al, byte ptr cs:[bp]
	mov	byte ptr cs:[gXorLeftMask], al
	mov	al, 0
	or	cl, cl
	je	L_2319
	inc	bp
	mov	al, byte ptr cs:[bp]
L_2319:
	mov	byte ptr cs:[gXorRightMask], al
	mov	ax, dx
	push	bx
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	pop	bx
	add	di, ax
	xor	ah, ah
	mov	al, SCREEN_COLS - 1
	sub	al, byte ptr cs:[gXorMiddleBytes]
	cmp	byte ptr cs:[gXorRightMask], 0
	je	L_233F
	dec	al
L_233F:
	mov	word ptr cs:[gXorRowSkip], ax
	add	di, 0		; adds nothing
	call	DrawPageSeg
	mov	es, ax
	mov	dx, GC_INDEX
	mov	ax, (ROTATE_XOR SHL 8) OR GC_DATA_ROTATE
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gDataRotate], al
	out	dx, al
	sti
	dec	dx
	mov	al, GC_BIT_MASK
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	sti
	inc	dx
	mov	ch, byte ptr cs:[gXorMiddleBytes]
	mov	bh, byte ptr cs:[gXorRows]
	mov	bl, byte ptr cs:[gXorColour]
L_237A:
	mov	al, byte ptr cs:[gXorLeftMask]
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, bl
	xchg	byte ptr es:[di], al
	inc	di
	or	ch, ch
	je	L_23A4
	mov	al, 0FFh
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	cl, ch
L_239A:
	mov	al, bl
	xchg	byte ptr es:[di], al
	inc	di
	dec	cl
	jne	L_239A
L_23A4:
	mov	al, byte ptr cs:[gXorRightMask]
	or	al, al
	je	L_23B9
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, bl
	xchg	byte ptr es:[di], al
	inc	di
L_23B9:
	add	di, word ptr cs:[gXorRowSkip]
	dec	bh
	jne	L_237A
	mov	dx, GC_INDEX
	mov	ax, (0 SHL 8) OR GC_DATA_ROTATE
	cli
	mov	byte ptr cs:[gGcIndex], al
	out	dx, al
	inc	dx
	mov	al, ah
	mov	byte ptr cs:[gDataRotate], al
	out	dx, al
	sti
	call	ResetBitMask
	pop	es
	pop	di
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
XorRect	ENDP

; EGA.OVL 23E2 (export 36, gDrawShapeRemapped): DrawShape with each colour byte translated through gRemapTable
; (its flag bits kept)
DrawShapeRemapped	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	ds
	push	es
	push	di
	push	si
	mov	bx, word ptr cs:[gRegionBottom]
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	mov	word ptr cs:[gShapeEnd], ax
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	es, ax
	mov	si, word ptr [bp + 8]
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gShapeSeg], ax
	mov	ax, word ptr [bp + 0Ch]
	mov	word ptr cs:[gShapeX], ax
	mov	cl, 3
	sar	ax, cl
	mov	word ptr cs:[gShapeCol], ax
	mov	dx, word ptr [bp + 0Eh]
	mov	ax, word ptr cs:[gRegionLeft]
	dec	ax
	mov	word ptr cs:[gShapeClipLeft], ax
	mov	ax, word ptr cs:[gRegionRight]
	dec	ax
	mov	word ptr cs:[gShapeClipRight], ax
	mov	bp, ds
	mov	ax, word ptr cs:[gShapeSeg]
	mov	ds, ax
	inc	si
	lodsw
	mov	bx, ax
	and	bx, 0FFh
	mov	byte ptr cs:[gShapeRows], bl
	inc	si
	cmp	dx, word ptr cs:[gRegionBottom]
	jge	L_245F
	mov	cx, dx
	sub	cx, word ptr cs:[gRegionTop]
	jns	L_2476
	neg	cx
	cmp	cx, bx
	jl	L_2462
L_245F:
	jmp	L_255F
L_2462:
	lodsw
	test	ah, 40h
	je	L_2462
	dec	byte ptr cs:[gShapeRows]
	je	L_245F
	loop	L_2462
	mov	dx, word ptr cs:[gRegionTop]
L_2476:
	mov	ds, bp
	mov	bx, dx
	shl	bx, 1
	mov	di, word ptr cs:[bx + gRowOffsets]
	add	di, word ptr cs:[gShapeCol]
	mov	cx, word ptr cs:[gShapeX]
	and	cx, 7
	mov	bp, word ptr cs:[gShapeCol]
	mov	dx, GC_DATA
	mov	ax, word ptr cs:[gShapeSeg]
	push	ds
	mov	ds, ax
	mov	word ptr cs:[gShapeRowStart], di
	mov	bx, offset gRemapTable
L_24A5:
	push	bp
L_24A6:
	lodsb
	or	cl, cl
	jne	L_2501
	cmp	bp, word ptr cs:[gShapeClipRight]
	jg	L_24FE
	cmp	bp, word ptr cs:[gShapeClipLeft]
	jle	L_24FE
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di]
	lodsb
	xlat	byte ptr cs:[bx]
	mov	byte ptr es:[di], al
L_24C9:
	shl	al, 1
	jae	L_24A6
	inc	di
	inc	bp
	shl	al, 1
	jae	L_24A6
	shl	al, 1
	jb	L_2554
	dec	byte ptr cs:[gShapeRows]
	je	L_2554
	add	word ptr cs:[gShapeRowStart], SCREEN_COLS
	mov	di, word ptr cs:[gShapeRowStart]
	cmp	di, word ptr cs:[gShapeEnd]
	jae	L_2554
	pop	bp
	mov	ax, di
	sub	ax, bp
	cmp	ax, word ptr cs:[gShapeEnd]
	jae	L_2555
	jmp	short L_24A5
L_24FE:
	lodsb
	jmp	short L_24C9
L_2501:
	xor	ah, ah
	xchg	ah, al
	shr	ax, cl
	cmp	bp, word ptr cs:[gShapeClipRight]
	jg	L_254D
	cmp	bp, word ptr cs:[gShapeClipLeft]
	jle	L_254D
	mov	ch, al
	mov	al, ah
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di]
	lodsb
	xlat	byte ptr cs:[bx]
	mov	byte ptr es:[di], al
L_2529:
	cmp	bp, word ptr cs:[gShapeClipRight]
	jge	L_24C9
	cmp	bp, word ptr cs:[gShapeClipLeft]
	jl	L_24C9
	mov	ah, byte ptr es:[di + 1]
	xchg	ch, al
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	xchg	ch, al
	mov	byte ptr es:[di + 1], al
	jmp	L_24C9
L_254D:
	mov	ch, al
	lodsb
	xlat	byte ptr cs:[bx]
	jmp	short L_2529
L_2554:
	pop	bx
L_2555:
	pop	ds
	mov	al, 0FFh
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
L_255F:
	pop	si
	pop	di
	pop	es
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
DrawShapeRemapped	ENDP

; EGA.OVL 2569 (export 35, gSetRemapTable): SetRemapTable(void far *colours) copies 16 colours into gRemapTable
; and 15 more times with the flag nibbles 10h to F0h ORed in
SetRemapTable	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	cx
	push	dx
	push	si
	push	di
	push	ds
	push	es
	mov	si, word ptr [bp + 6]
	mov	ax, word ptr [bp + 8]
	mov	ds, ax
	mov	di, offset gRemapTable
	mov	ax, cs
	mov	es, ax
	mov	cx, 8
	rep movsw
	mov	dl, 10h
L_2589:
	sub	si, 10h
	mov	cx, 8
L_258F:
	lodsw
	or	al, dl
	or	ah, dl
	stosw
	loop	L_258F
	add	dl, 10h
	jne	L_2589
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	ax
	pop	bp
	retf
SetRemapTable	ENDP

; EGA.OVL 25A5 (export 37, gDrawShapeFlippedRemapped): DrawShapeRemapped mirrored left to right
DrawShapeFlippedRemapped	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	ds
	push	es
	push	di
	push	si
	mov	dx, SCREEN_COLS
	mov	ax, word ptr cs:[gRegionBottom]
	mov	word ptr cs:[gFlipBottom], ax
	mul	dl
	mov	word ptr cs:[gFlipEnd], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gFlipTop], ax
	xor	di, di
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	es, ax
	mov	si, word ptr [bp + 8]
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gFlipSeg], ax
	mov	ax, word ptr [bp + 0Ch]
	mov	word ptr cs:[gFlipX], ax
	mov	cl, 3
	sar	ax, cl
	mov	word ptr cs:[gFlipCol], ax
	mov	dx, word ptr [bp + 0Eh]
	mov	ax, word ptr cs:[gRegionLeft]
	dec	ax
	mov	word ptr cs:[gFlipClipLeft], ax
	mov	ax, word ptr cs:[gRegionRight]
	dec	ax
	mov	word ptr cs:[gFlipClipRight], ax
	mov	bp, ds
	mov	bx, word ptr cs:[gFlipSeg]
	mov	ds, bx
	inc	si
	lodsw
	mov	ds, bp
	mov	byte ptr cs:[gFlipCols], ah
	mov	ds, bx
	mov	bx, ax
	and	bx, 0FFh
	inc	bx
	mov	byte ptr cs:[gFlipRows], bl
	inc	si
	cmp	dx, word ptr cs:[gFlipBottom]
	jge	L_2638
	mov	cx, dx
	sub	cx, word ptr cs:[gFlipTop]
	jns	L_264F
	neg	cx
	cmp	cx, bx
	jl	L_263B
L_2638:
	jmp	L_2758
L_263B:
	lodsw
	test	ah, 40h
	je	L_263B
	dec	byte ptr cs:[gFlipRows]
	je	L_2638
	loop	L_263B
	mov	dx, word ptr cs:[gFlipTop]
L_264F:
	mov	ds, bp
	mov	bx, dx
	shl	bx, 1
	add	di, word ptr cs:[bx + gRowOffsets]
	add	di, word ptr cs:[gFlipCol]
	mov	al, byte ptr cs:[gFlipCols]
	xor	ah, ah
	dec	ax
	add	di, ax
	mov	cx, word ptr cs:[gFlipX]
	and	cx, 7
	mov	bp, word ptr cs:[gFlipCol]
	add	bp, ax
	mov	ax, word ptr cs:[gFlipSeg]
	jmp	short L_2680
L_267D:
	lodsb
	jmp	short L_26B9
L_2680:
	push	ds
	mov	ds, ax
	mov	dx, GC_DATA
	mov	word ptr cs:[gShapeRowStart], di
	mov	bx, offset gRemapTable
L_268E:
	push	bp
L_268F:
	lodsb
	or	cl, cl
	jne	L_26F3
	cmp	bp, word ptr cs:[gFlipClipRight]
	jg	L_267D
	cmp	bp, word ptr cs:[gFlipClipLeft]
	jle	L_267D
	push	bx
	mov	bx, offset gMirrorBits
	xlat	byte ptr cs:[bx]
	pop	bx
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	mov	al, byte ptr es:[di]
	lodsb
	xlat	byte ptr cs:[bx]
	mov	byte ptr es:[di], al
L_26B9:
	shl	al, 1
	jae	L_268F
	dec	di
	dec	bp
	shl	al, 1
	jae	L_268F
	shl	al, 1
	jb	L_26F1
	dec	byte ptr cs:[gFlipRows]
	je	L_26F1
	add	word ptr cs:[gShapeRowStart], SCREEN_COLS
	mov	di, word ptr cs:[gShapeRowStart]
	cmp	di, word ptr cs:[gFlipEnd]
	jae	L_26F1
	pop	bp
	mov	ax, di
	sub	ax, bp
	cmp	ax, word ptr cs:[gFlipEnd]
	jae	L_274E
	jmp	short L_268E
	lodsb		; QUIRK 26EE: dead code, the jmp above skips it
	jmp	short L_26B9
L_26F1:
	jmp	short L_274D
L_26F3:
	push	bx
	mov	bx, offset gMirrorBits
	xlat	byte ptr cs:[bx]
	pop	bx
	mov	ah, al
	xor	al, al
	shr	ax, cl
	cmp	bp, word ptr cs:[gFlipClipRight]
	jg	L_2746
	cmp	bp, word ptr cs:[gFlipClipLeft]
	jle	L_2746
	mov	ch, al
	mov	al, byte ptr es:[di]
	mov	al, ah
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	lodsb
	xlat	byte ptr cs:[bx]
	mov	byte ptr es:[di], al
L_2722:
	cmp	bp, word ptr cs:[gFlipClipRight]
	jge	L_26B9
	cmp	bp, word ptr cs:[gFlipClipLeft]
	jl	L_2743
	mov	ah, byte ptr es:[di + 1]
	xchg	ch, al
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
	xchg	ch, al
	mov	byte ptr es:[di + 1], al
L_2743:
	jmp	L_26B9
L_2746:
	mov	ch, al
	lodsb
	xlat	byte ptr cs:[bx]
	jmp	short L_2722
L_274D:
	pop	bx
L_274E:
	pop	ds
	mov	al, 0FFh
	cli
	mov	byte ptr cs:[gBitMask], al
	out	dx, al
	sti
L_2758:
	pop	si
	pop	di
	pop	es
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
DrawShapeFlippedRemapped	ENDP

DriverEnd	LABEL	BYTE		; 2762: the end of the image, DriverEntry's size
DRIVER	ENDS
	END
