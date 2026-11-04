; MCGA.OVL: the graphics driver for the VGA and MCGA choice, 320 by 200 pixels in 256 colours (BIOS mode 13h),
; one byte per pixel, a screen line 320 bytes, a page 64000 bytes. The game loads it with DOS 4B03h at a segment
; of its choosing and links it to its own slot table at ds:802A (whitepaper 2.5): DriverEntry (0C3F, reached by
; the jmp at 0000) returns the driver's size and LinkDriver (0BE6), which exchanges far pointers with the
; game's table: seven imports from the game (gPageSegs, gDrawPage, MemAlloc among them, cs:0003) and forty
; exports, the routines and tables the game calls through ds:8046 to ds:80E5 (cs:001F).
;
; Pages. A page number (0 is the screen at A000h) is turned into a segment through the game's gPageSegs
; (PageSeg, 0B3F); DrawPageSeg (0B1D) reads the game's gDrawPage first. Coordinates are pixels (x 0 to 319,
; y 0 to 199) or, where a comment says so, 8-pixel columns (0 to 39). The shape draws clip to the region
; SetRegion sets (cs:00CA, in columns and lines), the line draw to SetLineClip's rectangle (cs:0204).
;
; In address order: the entry jump; the slot table (0003); variables and tables (00C1 to 04F7): the region,
; the shape draws' state, the nibble swap table, the line clip rectangle, the colour map and remap table, the
; row offsets; the routines (04F8 to 2026); the text and XOR rectangle state and their bit mask table (2027 to
; 241D); more routines (241E to 2C37). Routines are far (retf) unless their comment says near; each saves the
; registers it uses but ax where it returns a value. The bytes are the file's own; TASM 2.0 and TLINK 3.0 build
; the whole file from this source (make rebuild PROGRAM=MCGA.OVL).
;
; The source names routines and the slot table after the game's names for its slots (re/globals.toml) and
; names the rest from the code. Dead code the original keeps (an early return before a routine's body, a
; jump never taken) is written as the instructions it is; QUIRK marks it.

SLOT_COUNT	equ	47		; slots in the table at cs:0003: 7 imports and 40 exports
SCREEN_WIDTH	equ	320		; bytes in a screen line
SCREEN_HEIGHT	equ	200
SCREEN_COLS	equ	40		; 8-pixel columns in a line
PAGE_WORDS	equ	32000		; words in a 64000-byte page
MASK_PLANE	equ	8000		; bytes in one bit plane of page 4 (320 * 200 / 8)
FONT_HEIGHT	equ	102h		; the font's byte offsets: its height in lines
FONT_WIDTH	equ	103h		; and its width in pixels

DRIVER	SEGMENT BYTE PUBLIC 'CODE'
	ASSUME	cs:DRIVER, ds:NOTHING, es:NOTHING, ss:NOTHING

; MCGA.OVL 0000: the image starts with a jump to DriverEntry; the game calls segment:0000 far after loading
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
	dd	GetSpriteCellRle		; export 15, slot 22, ds:8082 gGetSpriteCellRle
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
	dd	gNibbleSwap		; export 38, slot 45, ds:80DE
	dd	gRemapTable		; export 39, slot 46, ds:80E2 gRemapTable
gSlotCount	dw	SLOT_COUNT		; the slots LinkDriver walks

; cs:00C1: the region and the shape draws' state
gSavedLeft	dw	0		; SaveDrawState keeps the region here (in columns and lines)
gSavedRight	dw	0
gSavedTop	dw	0
gSavedBottom	dw	0
gFillColour	db	0		; FillRect's colour
gRegionLeft	dw	0		; the drawing region SetRegion sets: left and right in 8-pixel columns
gRegionRight	dw	0
gRegionTop	dw	0		; top and bottom in lines
gRegionBottom	dw	0
gShapeSeg	dw	0		; the state of the shape draws that keep it here (the masked and flipped ones): the shape's segment
gShapeX	dw	0		; x in pixels from the region's left edge
gShapeY	dw	0		; y; then the screen bytes between two rows
gShapeRows	dw	0		; rows to draw
gShapeWidth	dw	0		; visible width in pixels
gShapeFullWidth	dw	0		; the shape's width in pixels
gRegionCols	dw	0		; the region's width in columns
gSkipLeft	dw	0		; pixels cut at the left; DrawShapeFlippedRemapped: 1 when the left edge cuts
gSkipRight	dw	0		; pixels cut at the right; DrawShapeFlippedRemapped: 1 when the right edge cuts
gVisibleWidth	dw	0
gRowBytes	dw	0
gSrcRowSkip	dw	0
gSrcRowStart	dw	0
gDstRowSkip	dw	0
gRowsLeft	dw	0
gDstSeg	dw	0
gShapeUnused	dw	0		; written 0 by DrawShapeFlipped and DrawShapeFlippedRemapped, read by nothing
gOddX	dw	0		; the low bit of x: the shape starts on an odd pixel

; cs:00F6 (export 38): each byte with its two nibbles swapped, the two pixels of a 4-bit shape byte in the other
; order; DrawShapeFlippedRemapped reads a mirrored shape through it (2BA8)
gNibbleSwap LABEL BYTE
NibbleByte = 0
	REPT	256
	db	((NibbleByte AND 0Fh) SHL 4) OR (NibbleByte SHR 4)
NibbleByte = NibbleByte + 1
	ENDM

; cs:01F6: the masked draws' pages, the sprite cutters' counts, SetLineClip's rectangle
gMaskPlaneSeg	dw	0		; the segment of page 4, the mask planes
gBackPageSeg	dw	0		; the segment of page 2, the background
gMaskBit	db	0		; the bit of the pixel in a mask plane byte
gMaskLevel	dw	0		; the shape's depth: planes below it cover it
gCellBytes	dw	0		; bytes the sprite cell needs, header excluded
	db	4 dup (0)		; 01FF: not used
gNibbleHigh	db	0		; the cutters' nibble phase (odd: low nibble)
gLineClipLeft	dw	0		; the line clip rectangle, in pixels (SetLineClip)
gLineClipTop	dw	0
gLineClipRight	dw	13Fh
gLineClipBottom	dw	0C7h

; cs:020C: GetSpriteCell's colour map, one byte per screen colour: its 4-bit code in the cell, or FFh while unused
gColourMap	db	256 dup (0)

; cs:030C (export 39, the game's gRemapTable): the 16 colours SetRemapTable copies in, which the remapped shape
; draws translate each 4-bit pixel through
gRemapTable	db	16 dup (0)

; cs:031C: the sprite cutters' state
gCellSeg	dw	0		; the allocated cell: segment
gCellOff	dw	0		; and offset
	db	2 dup (0)		; 0320: not used
gCellSrcRow	dw	0		; the source row in the page
gCellRowStart	dw	0		; the start of the row being coded
gCellRows	db	0		; rows left
gColoursUsed	db	0		; colours given a code so far

; cs:0328 (export 34, the game's gRowOffsetTableOff): the offset of each screen line, y * 320, for y = 0 to
; 200 (one past the last line)
gRowOffsets LABEL WORD
RowOffset = 0
	REPT	SCREEN_HEIGHT + 1
	dw	RowOffset
RowOffset = RowOffset + SCREEN_WIDTH
	ENDM

; cs:04BA: the tile draws' state (SetTileSet, SetTileFormat, DrawIndexedTile)
	db	12 dup (0)		; 04BA: not used
gTileColoursPtr	dd	0		; far: the 16-byte colour rows, one per tile kind
gTileMapPtr	dd	0		; far: the tile kind of each tile number
gTileDstSkip	dw	0		; screen bytes after a tile row
gTileRows	dw	0		; lines in a tile
gTileRowBytes	dw	0		; bytes in a tile row
gTileHeightCells	dw	0
gTileParas	dw	0		; paragraphs per tile in the pixel data
gTilePixelsPtr	dd	0		; far: the tiles' 4-bit pixels
gTileWidthCells	dw	0
gTileMode	dw	0		; the gTileDrawModes index: 0 or 2 draws, 4 does not
gTileFormatArg3	dw	0		; stored by SetTileFormat, read by nothing
gTileFormatArg4	dw	0
gTileReqWidth	dw	0
gTileReqHeight	dw	0
gTileColourRow	db	16 dup (0)		; the colour row of the tile being drawn

; MCGA.OVL 04F8 (export 0, the game's gCopyPage): CopyPage(srcSeg, dstSeg) copies a whole 64000-byte page,
; the segments as given (the game's CopyPage, 19EC:002A, passes them)
CopyPage	PROC	NEAR
	push	bp
	mov	bp, sp
	push	cx
	push	di
	push	ds
	push	es
	push	si
	mov	ds, word ptr [bp + 6]
	xor	si, si
	mov	es, word ptr [bp + 8]
	xor	di, di
	mov	cx, PAGE_WORDS
	rep movsw
	pop	si
	pop	es
	pop	ds
	pop	di
	pop	cx
	pop	bp
	retf
CopyPage	ENDP

; MCGA.OVL 0516 (export 1, gRestoreRectToPage): RestoreRect on the page gDrawPage names instead of page 0:
; it reads the page number and joins RestoreRect's body (RestoreRectBody, 0543)
RestoreRectToPage	PROC	NEAR
	push	bp
	mov	bp, sp
	sub	sp, 2
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
	pop	bp		; QUIRK 0534: dead code, the jmp above skips it
	retf
RestoreRectToPage	ENDP

; MCGA.OVL 0536 (export 2, gRestoreRect): RestoreRect(col, y, cols, rows, void far *buf) copies a buffer
; SaveRect filled back to page 0 at column col (0 to 39), line y; cols is clamped to 40 and rows to 200, and a
; rectangle of no width or height draws nothing. The buffer holds cols * 8 bytes per row, rows after rows.
RestoreRect	PROC	NEAR
	push	bp
	mov	bp, sp
	sub	sp, 2
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
	jge	L_0555
	xor	di, di
L_0555:
	cmp	di, SCREEN_COLS
	jl	L_055D
	mov	di, SCREEN_COLS - 1
L_055D:
	shl	di, 1
	shl	di, 1
	shl	di, 1
	mov	dx, word ptr [bp + 8]
	cmp	dx, 0
	jge	L_056D
	xor	dx, dx
L_056D:
	cmp	dx, SCREEN_HEIGHT
	jl	L_0576
	mov	dx, SCREEN_HEIGHT - 1
L_0576:
	mov	bx, word ptr [bp + 0Ah]
	mov	cx, word ptr [bp + 0Ch]
	cmp	bx, SCREEN_COLS
	jbe	L_0584
	mov	bx, SCREEN_COLS
L_0584:
	or	bx, bx
	je	L_05C8
	js	L_05C8
	cmp	cx, SCREEN_HEIGHT
	jbe	L_0593
	mov	cx, SCREEN_HEIGHT
L_0593:
	or	cx, cx
	je	L_05C8
	push	bx
	mov	bx, dx
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	di, ax
	pop	bx
	mov	dx, cx
	shl	bx, 1
	shl	bx, 1
	mov	ax, word ptr [bp - 2]
	call	PageSeg
	mov	es, ax
	mov	si, word ptr [bp + 0Eh]
	mov	ds, word ptr [bp + 10h]
	mov	ax, SCREEN_WIDTH / 2
	sub	ax, bx
	shl	ax, 1
L_05BF:
	mov	cx, bx
	rep movsw
	add	di, ax
	dec	dx
	jne	L_05BF
L_05C8:
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

; MCGA.OVL 05D4 (export 3, gCopyRegion): CopyRegion(srcCol, srcY, dstCol, dstY, cols, rows, srcPage,
; dstPage) copies a rectangle of cols 8-pixel columns (1 to 40) and rows lines from one page to another; the game's
; ClipRect (19F3:000E) is its only caller. Positions past the screen end it; rows are cut at line 199.
CopyRegion	PROC	NEAR
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
	mov	cl, 3
	mov	ax, word ptr [bp + 6]
	cmp	ax, SCREEN_COLS
	jge	L_063F
	or	ax, ax
	jns	L_05F0
	xor	ax, ax
L_05F0:
	shl	ax, cl
	mov	si, ax
	mov	ax, word ptr [bp + 8]
	cmp	ax, SCREEN_HEIGHT
	jge	L_063F
	or	ax, ax
	jns	L_0602
	xor	ax, ax
L_0602:
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	si, ax
	mov	bx, SCREEN_WIDTH
	mov	ax, word ptr [bp + 0Ah]
	cmp	ax, SCREEN_COLS
	jge	L_0689
	or	ax, ax
	jns	L_061E
	xor	ax, ax
L_061E:
	shl	ax, cl
	mov	di, ax
	mov	ax, word ptr [bp + 0Ch]
	mov	cx, ax
	cmp	ax, SCREEN_HEIGHT
	jge	L_0689
	or	ax, ax
	jns	L_0632
	xor	ax, ax
L_0632:
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	di, ax
	jmp	short L_0641
L_063F:
	jmp	short L_0689
L_0641:
	mov	ax, word ptr [bp + 12h]
	call	PageSeg
	mov	ds, ax
	mov	ax, word ptr [bp + 14h]
	call	PageSeg
	mov	es, ax
	mov	dx, word ptr [bp + 0Eh]
	cmp	dx, SCREEN_COLS
	ja	L_0689
	cmp	dx, 1
	jl	L_0689
	shl	dx, 1
	shl	dx, 1
	mov	ax, word ptr [bp + 10h]
	add	cx, ax
	cmp	cx, SCREEN_HEIGHT
	jbe	L_0673
	sub	cx, SCREEN_HEIGHT - 1
	sub	ax, cx
L_0673:
	or	ax, ax
	js	L_0689
	mov	bx, SCREEN_WIDTH / 2
	sub	bx, dx
	shl	bx, 1
L_067E:
	mov	cx, dx
	rep movsw
	add	si, bx
	add	di, bx
	dec	ax
	jne	L_067E
L_0689:
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
CopyRegion	ENDP

; MCGA.OVL 0693 (export 4, gClearPage): fills the draw page (gDrawPage) with colour 0
ClearPage	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	cx
	push	di
	push	es
	cld
	call	DrawPageSeg
	mov	es, ax
	xor	di, di
	mov	cx, PAGE_WORDS
	xor	ax, ax
	rep stosw
	pop	es
	pop	di
	pop	cx
	pop	ax
	pop	bp
	retf
ClearPage	ENDP

; MCGA.OVL 06AF (export 5, gDrawCell8x8): DrawCell8x8(void far *src, col, y) copies 8 rows of 8 bytes from
; src to the draw page at column col (0 to 39) and line y (0 to 192); the frame tiles of the text windows
DrawCell8x8	PROC	NEAR
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
	mov	ax, word ptr [bp + 8]
	mov	ds, ax
	mov	ax, word ptr [bp + 0Ch]
	cmp	ax, 0
	jge	L_06CD
	xor	ax, ax
L_06CD:
	cmp	ax, SCREEN_HEIGHT - 7
	jl	L_06D5
	mov	ax, SCREEN_HEIGHT - 8
L_06D5:
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	mov	dx, word ptr [bp + 0Ah]
	cmp	dx, 0
	jge	L_06E8
	xor	dx, dx
L_06E8:
	cmp	dx, SCREEN_COLS
	jl	L_06F0
	mov	dx, SCREEN_COLS - 1
L_06F0:
	mov	cl, 3
	shl	dx, cl
	add	ax, dx
	mov	di, ax
	call	DrawPageSeg
	mov	es, ax
	mov	dx, 8
	mov	bx, SCREEN_WIDTH - 8
L_0703:
	mov	cx, 4
	rep movsw
	add	di, bx
	dec	dx
	jne	L_0703
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
DrawCell8x8	ENDP

; MCGA.OVL 0717 (export 6, gDrawIndexedTile): DrawIndexedTile(tile, col, y) draws tile number tile of the set
; SetTileSet named: its kind (a byte of the tile map) picks a 16-colour row, through which the tile's 4-bit pixels
; are drawn to page 0 at column col, line y, in the size SetTileFormat set (gTileDrawModes; a size it does not
; know draws nothing). Saves bp twice: the second copy is the frame of DrawTilePixels.
DrawIndexedTile	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bp
	push	bx
	push	cx
	push	di
	push	ds
	push	dx
	push	es
	push	si
	mov	di, word ptr cs:[gTileMapPtr]
	mov	es, word ptr cs:[gTileMapPtr+2]
	mov	ax, word ptr [bp + 6]
	add	di, ax
	mov	bl, byte ptr es:[di]
	xor	bh, bh
	shl	bx, 1
	shl	bx, 1
	shl	bx, 1
	shl	bx, 1
	mov	di, bx
	mov	bx, 0Fh
	add	di, word ptr cs:[gTileColoursPtr]
	mov	es, word ptr cs:[gTileColoursPtr+2]
L_074E:
	mov	al, byte ptr es:[bx + di]
	mov	byte ptr cs:[bx + gTileColourRow], al
	dec	bx
	jge	L_074E
	mov	di, word ptr [bp + 0Ah]
	shl	di, 1
	mov	di, word ptr cs:[di + gRowOffsets]
	mov	ax, word ptr [bp + 8]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	add	di, ax
	mov	ax, 0
	call	PageSeg
	mov	es, ax
	mov	si, word ptr cs:[gTileMode]
	jmp	word ptr cs:[si + gTileDrawModes]
gTileDrawModes	LABEL	WORD
	dw	L_0786, L_0786, L_078E
L_0786:
	mov	ax, word ptr [bp + 6]
	call	DrawTilePixels
	jmp	short L_07D8
L_078E:
	jmp	short L_07D8
DrawIndexedTile	ENDP

; MCGA.OVL 0790 (near): draws the pixels of tile ax through gTileColourRow at es:di, gTileRows rows of
; gTileRowBytes bytes, two pixels a byte (high nibble first)
DrawTilePixels	PROC	NEAR
	mov	dx, word ptr cs:[gTileParas]
	mul	dx
	mov	si, word ptr cs:[gTilePixelsPtr]
	add	ax, word ptr cs:[gTilePixelsPtr+2]
	mov	ds, ax
	mov	dx, word ptr cs:[gTileRows]
	xor	bh, bh
L_07AA:
	mov	cx, word ptr cs:[gTileRowBytes]
L_07AF:
	lodsb
	mov	ah, al
	mov	bl, al
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	mov	al, byte ptr cs:[bx + gTileColourRow]
	stosb
	mov	bl, ah
	and	bl, 0Fh
	mov	al, byte ptr cs:[bx + gTileColourRow]
	stosb
	loop	L_07AF
	add	di, word ptr cs:[gTileDstSkip]
	dec	dx
	jne	L_07AA
	ret
L_07D8:
	pop	si
	pop	es
	pop	dx
	pop	ds
	pop	di
	pop	cx
	pop	bx
	pop	bp
	pop	ax
	pop	bp
	retf
DrawTilePixels	ENDP

; MCGA.OVL 07E3 (export 7, gDrawLine): DrawLine(x1, y1, x2, y2, colour) draws a line on the draw page,
; clipped to SetLineClip's rectangle
DrawLine	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	di
	push	dx
	push	es
	call	DrawPageSeg
	mov	es, ax
	mov	ax, word ptr [bp + 6]
	mov	bx, word ptr [bp + 8]
	mov	cx, word ptr [bp + 0Ah]
	mov	dx, word ptr [bp + 0Ch]
	mov	di, word ptr [bp + 0Eh]
	call	DrawClippedLine
	pop	es
	pop	dx
	pop	di
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
DrawLine	ENDP

; MCGA.OVL 080B (near): the line in ax = x1, bx = y1, cx = x2, dx = y2, colour di, on es. Cohen and
; Sutherland: while an end is outside, LineOutcode classifies it and gClipByOutcode moves it to the edge it
; crosses; a line wholly outside is not drawn. Then horizontal lines are runs of stosw, vertical lines a
; column, and others Bresenham steps (DrawLineInside, 093A).
DrawClippedLine	PROC	NEAR
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
	mov	word ptr cs:[gLineColour], di
	cmp	ax, word ptr cs:[gLineClipLeft]
	jl	L_0853
	cmp	ax, word ptr cs:[gLineClipRight]
	jg	L_0853
	cmp	bx, word ptr cs:[gLineClipTop]
	jl	L_0853
	cmp	bx, word ptr cs:[gLineClipBottom]
	jg	L_0853
	cmp	cx, word ptr cs:[gLineClipLeft]
	jl	L_0853
	cmp	cx, word ptr cs:[gLineClipRight]
	jg	L_0853
	cmp	dx, word ptr cs:[gLineClipTop]
	jl	L_0853
	cmp	dx, word ptr cs:[gLineClipBottom]
	jle	L_087D
L_0853:
	call	LineOutcode
	xchg	cx, ax
	xchg	bx, dx
	mov	di, si
	call	LineOutcode
	mov	bp, di
	or	bp, si
	je	L_087D
	test	di, si
	jne	L_0880
	shl	si, 1
	call	word ptr cs:[si + gClipByOutcode]
	jb	L_0853
	xchg	cx, ax
	xchg	bx, dx
	shl	di, 1
	call	word ptr cs:[di + gClipByOutcode]
	jmp	short L_0853
L_087D:
	jmp	DrawLineInside
L_0880:
	jmp	L_09EB
gClipByOutcode	LABEL	WORD
	dw	ClipKeep, ClipToTop, ClipToBottom, ClipKeep
	dw	ClipToLeft, ClipToLeft, ClipToBottom, ClipKeep
	dw	ClipToRight, ClipToTop, ClipToRight, ClipKeep
	dw	ClipKeep, ClipKeep, ClipKeep, ClipKeep
DrawClippedLine	ENDP

; MCGA.OVL 08A3 (near): outcode 0 and impossible codes: nothing to move, carry clear
ClipKeep	PROC	NEAR
	clc
	ret
ClipKeep	ENDP

; MCGA.OVL 08A5 (near): moves the end (ax, bx) to the clip top along the line, carry set
ClipToTop	PROC	NEAR
	mov	si, word ptr cs:[gLineClipTop]
	call	MoveToLineY
	stc
	ret
ClipToTop	ENDP

; MCGA.OVL 08AF (near): moves the end to the clip bottom (by mirroring y and using MoveToLineY)
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

; MCGA.OVL 08C3 (near): the x where the line from (ax, bx) to (cx, dx) meets y = si; returns ax = that x, bx = si
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

; MCGA.OVL 08DC (near): moves the end to the clip left edge, carry set
ClipToLeft	PROC	NEAR
	mov	si, word ptr cs:[gLineClipLeft]
	call	MoveToLineX
	stc
	ret
ClipToLeft	ENDP

; MCGA.OVL 08E6 (near): moves the end to the clip right edge (by mirroring x and using MoveToLineX)
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

; MCGA.OVL 08FA (near): the y where the line meets x = si; returns bx = that y, ax = si
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

; MCGA.OVL 090F (near): the outcode of the point (ax, bx) in si: 1 above the clip rectangle, 2 below, 4 left of
; it, 8 right of it
LineOutcode	PROC	NEAR
	xor	si, si
	cmp	bx, word ptr cs:[gLineClipTop]
	jge	L_091B
	or	si, 1
L_091B:
	cmp	bx, word ptr cs:[gLineClipBottom]
	jle	L_0925
	or	si, 2
L_0925:
	cmp	ax, word ptr cs:[gLineClipLeft]
	jge	L_092F
	or	si, 4
L_092F:
	cmp	ax, word ptr cs:[gLineClipRight]
	jle	L_0939
	or	si, 8
L_0939:
	ret
DrawLineInside:
	sub	dx, bx
	jne	L_0940
	jmp	short L_09A3
L_0940:
	jg	L_0947
	neg	dx
	xchg	cx, ax
	sub	bx, dx
L_0947:
	shl	bx, 1
	mov	bx, word ptr cs:[bx + gRowOffsets]
	mov	si, 1
	sub	cx, ax
	jne	L_0957
	jmp	short L_09D5
L_0957:
	jg	L_095D
	neg	cx
	neg	si
L_095D:
	cmp	cx, dx
	jge	L_0983
	xchg	cx, dx
	mov	di, cx
	mov	bp, cx
	shr	bp, 1
	add	bx, ax
	mov	ax, word ptr cs:[gLineColour]
L_096F:
	mov	byte ptr es:[bx], al
	dec	cx
	jl	L_09EB
	add	bx, SCREEN_WIDTH
	sub	bp, dx
	jge	L_096F
	add	bp, di
	add	bx, si
	jmp	short L_096F
L_0983:
	mov	di, cx
	mov	bp, cx
	shr	bp, 1
	add	bx, ax
	mov	ax, word ptr cs:[gLineColour]
L_098F:
	mov	byte ptr es:[bx], al
	dec	cx
	jl	L_09EB
	add	bx, si
	sub	bp, dx
	jge	L_098F
	add	bp, di
	add	bx, SCREEN_WIDTH
	jmp	short L_098F
L_09A3:
	cmp	ax, cx
	jl	L_09A8
	xchg	cx, ax
L_09A8:
	sub	cx, ax
	inc	cx
	shl	bx, 1
	mov	bx, word ptr cs:[bx + gRowOffsets]
	add	bx, ax
	mov	di, bx
	mov	ax, word ptr cs:[gLineColour]
	mov	ah, al
	test	di, 1
	je	L_09C4
	stosb
	dec	cx
L_09C4:
	mov	dx, cx
	shr	cx, 1
	je	L_09CC
	rep stosw
L_09CC:
	test	dx, 1
	je	L_09D3
	stosb
L_09D3:
	jmp	short L_09EB
L_09D5:
	mov	cx, dx
	inc	cx
	add	bx, ax
	mov	ax, word ptr cs:[gLineColour]
L_09DE:
	mov	byte ptr es:[bx], al
	add	bx, SCREEN_WIDTH
	loop	L_09DE
	jmp	short L_09EB
LineOutcode	ENDP

gLineColour	dw	0		; DrawLine's colour, kept inside the code (the jmp above skips it)

L_09EB:
	pop	si
	pop	es
	pop	dx
	pop	di
	pop	cx
	pop	bx
	pop	bp
	pop	ax
	pop	bp
	ret
; MCGA.OVL 09F5 (export 9, gFillRect): FillRect(x1, y1, x2, y2, colour) fills the rectangle on the draw page,
; corners included; a rectangle starting off the screen or with x1 > x2 or y1 > y2 fills nothing
FillRect	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	di
	push	dx
	push	es
	push	si
	cld
	mov	ax, word ptr [bp + 0Eh]
	mov	byte ptr cs:[gFillColour], al
	call	DrawPageSeg
	mov	es, ax
	mov	ax, word ptr [bp + 6]
	cmp	ax, SCREEN_WIDTH
	jge	L_0A8E
	or	ax, ax
	jns	L_0A1A
	xor	ax, ax
L_0A1A:
	mov	bx, word ptr [bp + 8]
	cmp	bx, SCREEN_HEIGHT
	jge	L_0A8E
	or	bx, bx
	jns	L_0A29
	xor	bx, bx
L_0A29:
	mov	cx, word ptr [bp + 0Ah]
	cmp	cx, 0
	jge	L_0A33
	xor	cx, cx
L_0A33:
	cmp	cx, SCREEN_WIDTH
	jge	L_0A8E
	mov	dx, word ptr [bp + 0Ch]
	cmp	dx, 0
	jge	L_0A43
	xor	dx, dx
L_0A43:
	cmp	dx, SCREEN_HEIGHT
	jge	L_0A8E
	cmp	ax, cx
	ja	L_0A8E
	cmp	bx, dx
	ja	L_0A8E
	cmp	cx, SCREEN_WIDTH
	jl	L_0A5A
	mov	cx, SCREEN_WIDTH - 1
L_0A5A:
	cmp	dx, SCREEN_HEIGHT
	jl	L_0A63
	mov	dx, SCREEN_HEIGHT - 1
L_0A63:
	mov	si, ax
	mov	di, ax
	mov	bp, dx
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	di, ax
	shr	bx, 1
	sub	bp, bx
	inc	bp
	inc	cx
	sub	cx, si
	mov	bx, SCREEN_WIDTH
	sub	bx, cx
	mov	dx, cx
	mov	al, byte ptr cs:[gFillColour]
L_0A85:
	rep stosb
	mov	cx, dx
	add	di, bx
	dec	bp
	jg	L_0A85
L_0A8E:
	pop	si
	pop	es
	pop	dx
	pop	di
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
FillRect	ENDP

; MCGA.OVL 0A97 (export 10, ds:806E): ReadCell8x8(page, void far *dst, col, y) copies the 8 by 8 pixels at
; column col, line y (0 to 192) of a page into dst, the inverse of DrawCell8x8. No call of ds:806E was found in
; EOB.EXE or INTRO.EXE.
ReadCell8x8	PROC	NEAR
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
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	ds, ax
	mov	di, word ptr [bp + 8]
	mov	es, word ptr [bp + 0Ah]
	mov	cx, 3
	mov	si, word ptr [bp + 0Ch]
	cmp	si, 0
	jge	L_0ABD
	xor	si, si
L_0ABD:
	cmp	si, SCREEN_COLS
	jl	L_0AC5
	mov	si, SCREEN_COLS - 1
L_0AC5:
	shl	si, cl
	mov	bx, word ptr [bp + 0Eh]
	cmp	bx, 0
	jge	L_0AD1
	xor	bx, bx
L_0AD1:
	cmp	bx, SCREEN_HEIGHT - 8
	jle	L_0ADA
	mov	bx, SCREEN_HEIGHT - 8
L_0ADA:
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	si, ax
	mov	cx, 8
L_0AE6:
	movsw
	movsw
	movsw
	movsw
	add	si, SCREEN_WIDTH - 8
	loop	L_0AE6
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
ReadCell8x8	ENDP

; MCGA.OVL 0AFA (near): ax = the segment of the page after the draw page (gDrawPage + 1), where the sprite
; cutters read from
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

; MCGA.OVL 0B1D (near): ax = the segment of the draw page, gPageSegs[gDrawPage & 1Fh]
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

; MCGA.OVL 0B3F (near): ax = the segment of page ax, gPageSegs[ax & 1Fh]
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

; MCGA.OVL 0B56 (export 11, gGetPixel): GetPixel(x, y) returns the colour of the draw page's pixel at
; (x, y), 0 off the screen
GetPixel	PROC	NEAR
	push	bp
	mov	bp, sp
	push	bx
	push	ds
	push	si
	mov	bx, word ptr [bp + 8]
	or	bx, bx
	js	L_0B88
	cmp	bx, SCREEN_HEIGHT
	jge	L_0B88
	shl	bx, 1
	mov	si, word ptr cs:[bx + gRowOffsets]
	mov	ax, word ptr [bp + 6]
	or	ax, ax
	js	L_0B88
	cmp	ax, SCREEN_WIDTH
	jge	L_0B88
	add	si, ax
	call	DrawPageSeg
	mov	ds, ax
	lodsb
	xor	ah, ah
	jmp	short L_0B8A
L_0B88:
	xor	ax, ax
L_0B8A:
	pop	si
	pop	ds
	pop	bx
	pop	bp
	retf
GetPixel	ENDP

; MCGA.OVL 0B8F (export 12, ds:8076): MirrorCell16(void far *src, void far *dst) writes the 16 by 16 cell of
; 4-bit pixels at src (16 rows of 8 bytes) mirrored left to right to dst. No call of ds:8076 was found in
; EOB.EXE or INTRO.EXE.
MirrorCell16	PROC	NEAR
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
	mov	si, word ptr [bp + 6]
	mov	ds, word ptr [bp + 8]
	mov	di, word ptr [bp + 0Ah]
	mov	es, word ptr [bp + 0Ch]
	mov	dl, 10h
	mov	cx, 4
L_0BAB:
	lodsw
	call	MirrorPixelWord
	mov	word ptr es:[di + 6], bx
	lodsw
	call	MirrorPixelWord
	mov	word ptr es:[di + 4], bx
	lodsw
	call	MirrorPixelWord
	mov	word ptr es:[di + 2], bx
	lodsw
	call	MirrorPixelWord
	mov	word ptr es:[di], bx
	add	di, 8
	dec	dl
	jne	L_0BAB
	jmp	short L_0BDC
MirrorCell16	ENDP

; MCGA.OVL 0BD3 (near): bx = the four pixels of ax (two bytes of two nibbles) in the other order
MirrorPixelWord	PROC	NEAR
	rol	al, cl
	rol	ah, cl
	xchg	al, ah
	mov	bx, ax
	ret
L_0BDC:
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
MirrorPixelWord	ENDP

; MCGA.OVL 0BE6: LinkDriver(void far *table) walks the game's slot table (ds:802A, the overlay request's
; pointer, 1C00:00C0) and the driver's (cs:0003) together, SLOT_COUNT slots: where the game's slot holds a
; pointer it is copied into the driver (an import); where it is zero, the driver's pointer is written into it
; (an export, its segment the driver's). Returns carry clear, or carry set when an export slot of the driver
; is zero.
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
L_0C04:
	xor	bx, bx
	lodsw
	mov	dx, ax
	lodsw
	or	bx, ax
	or	bx, dx
	je	L_0C18
	xchg	dx, ax
	stosw
	xchg	dx, ax
	stosw
	loop	L_0C04
	jmp	short L_0C31
L_0C18:
	sub	si, 4
	mov	dx, word ptr es:[di]
	add	di, 4
	or	dx, dx
	je	L_0C34
	mov	word ptr [si], dx
	add	si, 2
	mov	word ptr [si], es
	add	si, 2
	loop	L_0C04
L_0C31:
	clc
	jmp	short L_0C35
L_0C34:
	stc
L_0C35:
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

; MCGA.OVL 0C3F: DriverEntry, the far call the game makes at segment:0000 once the driver is loaded
; (1C00:0088): returns bx = the driver's size in paragraphs, for the game to shrink the block, and ax = the
; offset of LinkDriver, which the game calls next with its slot table
DriverEntry	PROC	NEAR
	push	bp
	mov	bp, sp
	push	cx
	push	di
	push	es
	mov	ax, 0A000h		; QUIRK 0C45: es:di = A000:0000 is set up and thrown away by the pops
	mov	es, ax
	xor	di, di
	mov	bx, offset DriverEnd
	mov	cl, 4
	shr	bx, cl
	inc	bx
	mov	ax, offset LinkDriver
	pop	es
	pop	di
	pop	cx
	pop	bp
	retf
DriverEntry	ENDP

; MCGA.OVL 0C5C (export 13, ds:807A): SetTileSet(void far *pixels, void far *colourRows, void far *tileMap)
; names the tile set DrawIndexedTile draws from
SetTileSet	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	mov	ax, word ptr [bp + 6]
	mov	word ptr cs:[gTilePixelsPtr], ax
	mov	ax, word ptr [bp + 8]
	mov	word ptr cs:[gTilePixelsPtr+2], ax
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gTileColoursPtr], ax
	mov	ax, word ptr [bp + 0Ch]
	mov	word ptr cs:[gTileColoursPtr+2], ax
	mov	ax, word ptr [bp + 0Eh]
	mov	word ptr cs:[gTileMapPtr], ax
	mov	ax, word ptr [bp + 10h]
	mov	word ptr cs:[gTileMapPtr+2], ax
	pop	ax
	pop	bp
	retf
SetTileSet	ENDP

; MCGA.OVL 0C8D (export 15, gGetSpriteCellRle): GetSpriteCellRle(col, y, cols, rows, void far *sheet) cuts
; the rectangle at column col, line y out of the page after the draw page (NextPageSeg; the sheet argument is not
; used by this driver) into a block from MemAlloc: a 4-byte header (0, rows, cols, rows), then the pixels, each
; run of zero bytes coded as 0 and its length (runs longer than 255 split with FFh). Two passes: one counts the
; bytes, one writes them. Returns dx:ax the block.
GetSpriteCellRle	PROC	NEAR
	push	bp
	mov	bp, sp
	push	bx
	push	cx
	push	di
	push	es
	push	si
	push	ds
	mov	ax, ds
	mov	es, ax
	cld
	call	CellSource
	call	CellSize
	push	bp
	xor	ax, ax
	mov	word ptr cs:[gCellBytes], ax
L_0CA8:
	mov	word ptr cs:[gCellRowStart], si
	mov	al, 8
	mul	dl
	add	ax, si
	mov	bp, ax
L_0CB5:
	lodsb
	or	al, al
	jne	L_0CEE
	inc	word ptr cs:[gCellBytes]
	mov	cx, si
L_0CC1:
	cmp	bp, si
	je	L_0CCD
	cmp	byte ptr [si], 0
	jne	L_0CCD
	inc	si
	jmp	short L_0CC1
L_0CCD:
	mov	ax, si
	sub	ax, cx
	inc	ax
	or	ah, ah
	je	L_0CEE
	push	si
	mov	si, ax
	sub	si, 0FFh
	mov	al, 0FFh
	inc	word ptr cs:[gCellBytes]
	xor	al, al
	inc	word ptr cs:[gCellBytes]
	mov	ax, si
	pop	si
L_0CEE:
	inc	word ptr cs:[gCellBytes]
	cmp	bp, si
	jne	L_0CB5
	mov	si, word ptr cs:[gCellRowStart]
	add	si, SCREEN_WIDTH
	dec	dh
	jne	L_0CA8
	pop	bp
	mov	ax, es
	mov	ds, ax
	mov	ax, 0
	push	ax
	xor	ax, ax
	push	ax
	mov	ax, word ptr cs:[gCellBytes]
	add	ax, 4
	push	ax
	call	dword ptr cs:[gMemAllocPtr]
	mov	di, ax
	mov	word ptr cs:[gCellOff], ax
	mov	es, dx
	mov	word ptr cs:[gCellSeg], dx
	add	sp, 6
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
	call	CellSource
L_0D42:
	mov	word ptr cs:[gCellRowStart], si
	mov	al, 8
	mul	dl
	add	ax, si
	mov	bp, ax
L_0D4F:
	lodsb
	or	al, al
	jne	L_0D7C
	stosb
	mov	cx, si
L_0D57:
	cmp	bp, si
	je	L_0D63
	cmp	byte ptr [si], 0
	jne	L_0D63
	inc	si
	jmp	short L_0D57
L_0D63:
	mov	ax, si
	sub	ax, cx
	inc	ax
	or	ah, ah
	je	L_0D7C
	push	si
	mov	si, ax
	sub	si, 0FFh
	mov	al, 0FFh
	stosb
	xor	al, al
	stosb
	mov	ax, si
	pop	si
L_0D7C:
	stosb
	cmp	bp, si
	jne	L_0D4F
	mov	si, word ptr cs:[gCellRowStart]
	add	si, SCREEN_WIDTH
	dec	dh
	jne	L_0D42
	pop	ds
	pop	si
	pop	es
	pop	di
	pop	cx
	pop	bx
	mov	ax, word ptr cs:[gCellOff]
	mov	dx, word ptr cs:[gCellSeg]
	pop	bp
	retf
GetSpriteCellRle	ENDP

; MCGA.OVL 0D9F (near): ds:si = the source rectangle's corner in the page after the draw page, from the
; caller's col ([bp+6]) and y ([bp+8])
CellSource	PROC	NEAR
	call	NextPageSeg
	mov	ds, ax
	mov	cl, 3
	mov	si, word ptr [bp + 6]
	shl	si, cl
	mov	ax, word ptr [bp + 8]
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	si, ax
	ret
CellSource	ENDP

; MCGA.OVL 0DBA (near): dh = rows and dl = cols from the caller's frame; gCellRows = rows
CellSize	PROC	NEAR
	mov	ax, word ptr [bp + 0Ch]
	mov	dh, al
	mov	byte ptr cs:[gCellRows], al
	mov	ax, word ptr [bp + 0Ah]
	mov	dl, al
	ret
CellSize	ENDP

; MCGA.OVL 0DC9 (near): marks every screen colour as not yet given a 4-bit code (gColourMap = FFh)
ClearColourMap	PROC	NEAR
	mov	di, offset gColourMap
	mov	cx, 80h
	mov	ax, cs
	mov	es, ax
	mov	ax, 0FFFFh
	rep stosw
	ret
ClearColourMap	ENDP

; MCGA.OVL 0DD9 (export 16, gGetSpriteCell): GetSpriteCell(col, y, cols, rows, void far *sheet) cuts a
; rectangle of the page after the draw page into a 4-bit cell from MemAlloc: a 4-byte header (1, rows, cols,
; rows), 16 bytes of colours (the screen colour of each code, in the order met), then two pixels a byte. Colours
; past the sixteenth get code 0. Two passes, like GetSpriteCellRle; the sheet argument is not used. Returns dx:ax
; the block.
GetSpriteCell	PROC	NEAR
	push	bp
	mov	bp, sp
	push	bx
	push	cx
	push	di
	push	es
	push	si
	push	ds
	mov	ax, ds
	mov	word ptr cs:[gCellSeg], ax
	cld
	call	ClearColourMap
	call	CellSource
	mov	word ptr cs:[gCellSrcRow], si
	call	CellSize
	xor	dx, dx
	mov	dl, al
	push	bp
	mov	ax, 10h
	mov	word ptr cs:[gCellBytes], ax
	mov	bx, di
	add	di, 10h
	shl	dx, 1
	shl	dx, 1
	shl	dx, 1
	xor	ch, ch
	mov	byte ptr cs:[gNibbleHigh], ch
	mov	byte ptr cs:[gColoursUsed], ch
L_0E1A:
	mov	cx, dx
L_0E1C:
	xor	ax, ax
	lodsb
	mov	bp, ax
	mov	ah, byte ptr cs:[bp + gColourMap]
	cmp	ah, 0FFh
	je	L_0E45
L_0E2B:
	inc	byte ptr cs:[gNibbleHigh]
	test	byte ptr cs:[gNibbleHigh], 1
	je	L_0E3C
	loop	L_0E1C
	jmp	short L_0E63
L_0E3C:
	inc	word ptr cs:[gCellBytes]
	loop	L_0E1C
	jmp	short L_0E63
L_0E45:
	cmp	byte ptr cs:[gColoursUsed], 10h
	jae	L_0E5F
	inc	bx
	mov	ah, byte ptr cs:[gColoursUsed]
	inc	byte ptr cs:[gColoursUsed]
	mov	byte ptr cs:[bp + gColourMap], ah
	jne	L_0E2B
L_0E5F:
	xor	ah, ah
	je	L_0E2B
L_0E63:
	add	word ptr cs:[gCellSrcRow], SCREEN_WIDTH
	mov	si, word ptr cs:[gCellSrcRow]
	dec	byte ptr cs:[gCellRows]
	jne	L_0E1A
	call	ClearColourMap
	pop	bp
	mov	ax, word ptr cs:[gCellSeg]
	mov	ds, ax
	mov	ax, 0
	push	ax
	xor	ax, ax
	push	ax
	mov	ax, word ptr cs:[gCellBytes]
	add	ax, 4
	push	ax
	call	dword ptr cs:[gMemAllocPtr]
	mov	di, ax
	mov	word ptr cs:[gCellOff], ax
	mov	es, dx
	mov	word ptr cs:[gCellSeg], dx
	add	sp, 6
	mov	al, 1
	stosb
	mov	ax, word ptr [bp + 0Ch]
	mov	dh, al
	stosb
	mov	ax, word ptr [bp + 0Ah]
	mov	dl, al
	stosb
	mov	al, dh
	stosb
	call	CellSource
	mov	byte ptr cs:[gCellRows], dh
	xor	dh, dh
	mov	word ptr cs:[gCellSrcRow], si
	mov	bx, di
	add	di, 10h
	shl	dx, 1
	shl	dx, 1
	shl	dx, 1
	xor	ch, ch
	mov	byte ptr cs:[gNibbleHigh], ch
	mov	byte ptr cs:[gColoursUsed], ch
L_0EDC:
	mov	cx, dx
L_0EDE:
	xor	ax, ax
	lodsb
	mov	bp, ax
	mov	ah, byte ptr cs:[bp + gColourMap]
	cmp	ah, 0FFh
	je	L_0F11
L_0EED:
	inc	byte ptr cs:[gNibbleHigh]
	test	byte ptr cs:[gNibbleHigh], 1
	je	L_0F09
	shl	ah, 1
	shl	ah, 1
	shl	ah, 1
	shl	ah, 1
	mov	byte ptr es:[di], ah
	loop	L_0EDE
	jmp	short L_0F32
L_0F09:
	or	byte ptr es:[di], ah
	inc	di
	loop	L_0EDE
	jmp	short L_0F32
L_0F11:
	cmp	byte ptr cs:[gColoursUsed], 10h
	jae	L_0F2E
	mov	byte ptr es:[bx], al
	inc	bx
	mov	ah, byte ptr cs:[gColoursUsed]
	inc	byte ptr cs:[gColoursUsed]
	mov	byte ptr cs:[bp + gColourMap], ah
	jne	L_0EED
L_0F2E:
	xor	ah, ah
	je	L_0EED
L_0F32:
	add	word ptr cs:[gCellSrcRow], SCREEN_WIDTH
	mov	si, word ptr cs:[gCellSrcRow]
	dec	byte ptr cs:[gCellRows]
	jne	L_0EDC
	pop	ds
	pop	si
	pop	es
	pop	di
	pop	cx
	pop	bx
	mov	ax, word ptr cs:[gCellOff]
	mov	dx, word ptr cs:[gCellSeg]
	pop	bp
	retf
GetSpriteCell	ENDP

; MCGA.OVL 0F56 (export 17, ds:808A): DrawMaskedCell16(maskCell, cell, void far *sheet, col, y) draws the
; 16 by 16 cell number cell of a sheet of 128-byte 4-bit cells to the draw page at column col and line y,
; through cell maskCell of the same sheet: where a mask nibble is set the pixel keeps the cell's nibble, elsewhere
; the sheet's. INTRO.EXE calls it once (ds:1F16); EOB.EXE never.
DrawMaskedCell16	PROC	NEAR
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
	mov	si, word ptr [bp + 0Ah]
	mov	ds, word ptr [bp + 0Ch]
	mov	di, word ptr [bp + 0Eh]
	shl	di, 1
	shl	di, 1
	shl	di, 1
	mov	ax, word ptr [bp + 10h]
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	di, ax
	mov	bx, word ptr [bp + 6]
	mov	ax, word ptr [bp + 8]
	mov	cx, 7
	shl	ax, cl
	shl	bx, cl
	add	bx, si
	add	si, ax
	call	DrawPageSeg
	mov	es, ax
	mov	dx, 10h
L_0F98:
	mov	cx, 8
L_0F9B:
	mov	al, byte ptr [bx]
	mov	ah, 0FFh
	test	al, 0F0h
	je	L_0FA6
	and	ah, 0Fh
L_0FA6:
	test	al, 0Fh
	je	L_0FAD
	and	ah, 0F0h
L_0FAD:
	and	ah, byte ptr [si]
	or	al, ah
	mov	ah, al
	and	ax, 0FF0h
	stosw
	inc	si
	inc	bx
	loop	L_0F9B
	add	di, SCREEN_WIDTH - 16
	dec	dl
	jne	L_0F98
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
DrawMaskedCell16	ENDP

; MCGA.OVL 0FCD (export 18, gCopyToPlanar): nothing to do in a chunky mode (EGA.OVL writes the four planes)
CopyToPlanar	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
CopyToPlanar	ENDP

; MCGA.OVL 0FD2 (export 19, ds:8092): SwapPageWithScreen(page) exchanges the 64000 bytes of a page and
; page 0. No call of ds:8092 was found in EOB.EXE or INTRO.EXE.
SwapPageWithScreen	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	cx
	push	di
	push	ds
	push	es
	push	si
	xor	cx, cx
	mov	si, cx
	mov	di, cx
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	ds, ax
	mov	ax, 0
	call	PageSeg
	mov	es, ax
	mov	cx, PAGE_WORDS
	cld
L_0FF5:
	mov	ax, word ptr es:[di]
	movsw
	mov	word ptr [si - 2], ax
	loop	L_0FF5
	pop	si
	pop	es
	pop	ds
	pop	di
	pop	cx
	pop	ax
	pop	bp
	retf
SwapPageWithScreen	ENDP

; MCGA.OVL 1006 (export 20, gDrawShapeMaskedFlipped): DrawShapeMasked drawn mirrored left to right
DrawShapeMaskedFlipped	PROC	NEAR
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
	mov	ax, word ptr [bp + 10h]
	mov	word ptr cs:[gMaskLevel], ax
	mov	ax, 4
	call	PageSeg
	mov	word ptr cs:[gMaskPlaneSeg], ax
	mov	ax, 2
	call	PageSeg
	mov	word ptr cs:[gBackPageSeg], ax
	xor	di, di
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	es, ax
	mov	si, word ptr [bp + 8]
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gShapeSeg], ax
	mov	bx, word ptr [bp + 0Ch]
	mov	cx, word ptr [bp + 0Eh]
	mov	word ptr cs:[gShapeY], cx
	mov	ax, word ptr cs:[gRegionLeft]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	add	di, ax
	sub	bx, ax
	mov	word ptr cs:[gShapeX], bx
	mov	ax, word ptr cs:[gRegionRight]
	sub	ax, word ptr cs:[gRegionLeft]
	mov	word ptr cs:[gRegionCols], ax
	push	ds
	mov	ax, word ptr cs:[gShapeSeg]
	mov	ds, ax
	inc	si
	lodsw
	inc	si
	pop	ds
	push	ax
	and	ax, 0FFh
	mov	word ptr cs:[gShapeRows], ax
	pop	ax
	xchg	al, ah
	and	ax, 0FFh
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	word ptr cs:[gShapeWidth], ax
	mov	word ptr cs:[gShapeFullWidth], ax
	mov	ax, word ptr cs:[gShapeY]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_10E3
	add	word ptr cs:[gShapeRows], ax
	jg	L_10A8
	jmp	L_115B
L_10A8:
	neg	ax
	mov	dx, ax
	mov	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipLeft], ax
	push	di
	push	es
	mov	di, si
	mov	ax, word ptr cs:[gShapeSeg]
	mov	es, ax
L_10BE:
	mov	cx, word ptr cs:[gSkipLeft]
L_10C3:
	xor	ax, ax
	repne scasb
	je	L_10CB
	jcxz	L_10D4
L_10CB:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jne	L_10C3
L_10D4:
	dec	dx
	jne	L_10BE
	mov	si, di
	pop	es
	pop	di
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gShapeY], ax
L_10E3:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gShapeY]
	js	L_115B
	je	L_115B
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_10FB
	mov	word ptr cs:[gShapeRows], ax
L_10FB:
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr cs:[gShapeX]
	or	ax, ax
	jns	L_1122
	add	word ptr cs:[gShapeWidth], ax
	neg	ax
	mov	word ptr cs:[gSkipLeft], ax
	cmp	ax, word ptr cs:[gShapeFullWidth]
	jae	L_115B
	xor	ax, ax
	mov	word ptr cs:[gShapeX], ax
L_1122:
	mov	word ptr cs:[gSkipRight], 0
	mov	ax, word ptr cs:[gRegionCols]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr cs:[gShapeX]
	js	L_115B
	je	L_115B
	cmp	ax, word ptr cs:[gShapeWidth]
	jae	L_115E
	mov	word ptr cs:[gShapeWidth], ax
	mov	ax, word ptr cs:[gShapeFullWidth]
	sub	ax, word ptr cs:[gSkipLeft]
	sub	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipRight], ax
	jmp	short L_115E
L_115B:
	jmp	L_12E6
L_115E:
	mov	ax, word ptr cs:[gShapeY]
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	ax, word ptr cs:[gShapeX]
	add	di, ax
	mov	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gVisibleWidth], ax
	add	di, ax
	dec	di
	mov	bp, di
	mov	dx, word ptr cs:[gShapeRows]
	mov	ax, word ptr cs:[gShapeSeg]
	mov	ds, ax
	xor	ax, ax
	mov	bx, es
L_118E:
	mov	cx, word ptr cs:[gSkipRight]
	jcxz	L_11B8
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_119D:
	xor	ax, ax
	repne scasb
	je	L_11A5
	jcxz	L_11AE
L_11A5:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jg	L_119D
L_11AE:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_11B8:
	add	di, cx
	add	cx, word ptr cs:[gVisibleWidth]
	jle	L_1235
L_11C1:
	lodsb
	or	al, al
	je	L_1237
	push	dx
	push	cx
	push	bx
	push	es
	push	di
	push	ax
	mov	ax, di
	push	ax
	and	ax, 7
	mov	cx, ax
	mov	al, 1
	clc
L_11D7:
	cmp	cx, 0
	jle	L_11E1
	shl	al, 1
	dec	cx
	jmp	short L_11D7
L_11E1:
	mov	byte ptr cs:[gMaskBit], al
	pop	ax
	shr	ax, 1
	shr	ax, 1
	shr	ax, 1
	mov	di, ax
	pop	ax
	mov	es, word ptr cs:[gMaskPlaneSeg]
	xor	bx, bx
	push	ax
	cmp	word ptr cs:[gMaskLevel], 1
	jge	L_120B
	mov	al, byte ptr es:[di + MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_120B:
	cmp	word ptr cs:[gMaskLevel], 2
	jge	L_121F
	mov	al, byte ptr es:[di + 2 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_121F:
	cmp	word ptr cs:[gMaskLevel], 3
	jge	L_123E
	mov	al, byte ptr es:[di + 3 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
	jmp	short L_123E
L_1235:
	jmp	short L_12B3
L_1237:
	jmp	short L_12AC
L_1239:
	jmp	short L_11C1
L_123B:
	jmp	L_118E
L_123E:
	cmp	word ptr cs:[gMaskLevel], 4
	jge	L_1252
	mov	al, byte ptr es:[di + 4 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1252:
	cmp	word ptr cs:[gMaskLevel], 5
	jge	L_1266
	mov	al, byte ptr es:[di + 5 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1266:
	cmp	word ptr cs:[gMaskLevel], 6
	jge	L_127A
	mov	al, byte ptr es:[di + 6 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_127A:
	mov	al, byte ptr es:[di + 7 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
	pop	ax
	pop	di
	cmp	bl, 0
	je	L_129C
	mov	es, word ptr cs:[gBackPageSeg]
	mov	bl, byte ptr es:[di]
	pop	es
	mov	byte ptr es:[di], bl
	dec	di
	jmp	short L_12A5
L_129C:
	pop	es
	mov	byte ptr es:[di], al
	dec	di
	jmp	short L_12A5
L_12A3:
	jmp	short L_123B
L_12A5:
	pop	bx
	pop	cx
	pop	dx
	loop	L_1239
	jmp	short L_12B3
L_12AC:
	lodsb
	sub	di, ax
	sub	cx, ax
	jg	L_1239
L_12B3:
	add	cx, word ptr cs:[gSkipLeft]
	jcxz	L_12DD
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_12C2:
	xor	ax, ax
	repne scasb
	je	L_12CA
	jcxz	L_12D3
L_12CA:
	inc	cx
	mov	al, byte ptr es:[di]
	inc	di
	sub	cx, ax
	jne	L_12C2
L_12D3:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_12DD:
	add	bp, SCREEN_WIDTH
	mov	di, bp
	dec	dx
	jne	L_12A3
L_12E6:
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
DrawShapeMaskedFlipped	ENDP

; MCGA.OVL 12F0 (export 21, gDrawShapeMasked): DrawShapeMasked(page, void far *shape, x, y, depth) draws an
; 8-bit shape (format 0, runs of transparent pixels) clipped to the region, behind the scenery: page 4 holds up
; to seven bit planes of 8000 bytes, plane k set where scenery at depth k covers the pixel; where a plane below
; depth is set, the pixel of page 2 (the background) is drawn instead of the shape's
DrawShapeMasked	PROC	NEAR
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
	mov	ax, word ptr [bp + 10h]
	mov	word ptr cs:[gMaskLevel], ax
	mov	ax, 4
	call	PageSeg
	mov	word ptr cs:[gMaskPlaneSeg], ax
	mov	ax, 2
	call	PageSeg
	mov	word ptr cs:[gBackPageSeg], ax
	xor	di, di
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	es, ax
	mov	si, word ptr [bp + 8]
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gShapeSeg], ax
	mov	bx, word ptr [bp + 0Ch]
	mov	cx, word ptr [bp + 0Eh]
	mov	word ptr cs:[gShapeY], cx
	mov	ax, word ptr cs:[gRegionLeft]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	add	di, ax
	sub	bx, ax
	mov	word ptr cs:[gShapeX], bx
	mov	ax, word ptr cs:[gRegionRight]
	sub	ax, word ptr cs:[gRegionLeft]
	mov	word ptr cs:[gRegionCols], ax
	push	ds
	mov	ax, word ptr cs:[gShapeSeg]
	mov	ds, ax
	inc	si
	lodsw
	inc	si
	pop	ds
	push	ax
	and	ax, 0FFh
	mov	word ptr cs:[gShapeRows], ax
	pop	ax
	xchg	al, ah
	and	ax, 0FFh
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	word ptr cs:[gShapeWidth], ax
	mov	word ptr cs:[gShapeFullWidth], ax
	mov	ax, word ptr cs:[gShapeY]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_13CD
	add	word ptr cs:[gShapeRows], ax
	jg	L_1392
	jmp	L_1445
L_1392:
	neg	ax
	mov	dx, ax
	mov	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipLeft], ax
	push	di
	push	es
	mov	di, si
	mov	ax, word ptr cs:[gShapeSeg]
	mov	es, ax
L_13A8:
	mov	cx, word ptr cs:[gSkipLeft]
L_13AD:
	xor	ax, ax
	repne scasb
	je	L_13B5
	jcxz	L_13BE
L_13B5:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jne	L_13AD
L_13BE:
	dec	dx
	jne	L_13A8
	mov	si, di
	pop	es
	pop	di
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gShapeY], ax
L_13CD:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gShapeY]
	js	L_1445
	je	L_1445
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_13E5
	mov	word ptr cs:[gShapeRows], ax
L_13E5:
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr cs:[gShapeX]
	or	ax, ax
	jns	L_140C
	add	word ptr cs:[gShapeWidth], ax
	neg	ax
	mov	word ptr cs:[gSkipLeft], ax
	cmp	ax, word ptr cs:[gShapeFullWidth]
	jae	L_1445
	xor	ax, ax
	mov	word ptr cs:[gShapeX], ax
L_140C:
	mov	word ptr cs:[gSkipRight], 0
	mov	ax, word ptr cs:[gRegionCols]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr cs:[gShapeX]
	js	L_1445
	je	L_1445
	cmp	ax, word ptr cs:[gShapeWidth]
	jae	L_1448
	mov	word ptr cs:[gShapeWidth], ax
	mov	ax, word ptr cs:[gShapeFullWidth]
	sub	ax, word ptr cs:[gSkipLeft]
	sub	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipRight], ax
	jmp	short L_1448
L_1445:
	jmp	L_15D0
L_1448:
	mov	ax, word ptr cs:[gShapeY]
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	ax, word ptr cs:[gShapeX]
	add	di, ax
	mov	bp, di
	mov	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gVisibleWidth], ax
	mov	dx, word ptr cs:[gShapeRows]
	mov	ax, word ptr cs:[gShapeSeg]
	mov	ds, ax
	xor	ax, ax
	mov	bx, es
L_1475:
	mov	cx, word ptr cs:[gSkipLeft]
	jcxz	L_149F
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_1484:
	xor	ax, ax
	repne scasb
	je	L_148C
	jcxz	L_1495
L_148C:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jg	L_1484
L_1495:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_149F:
	sub	di, cx
	add	cx, word ptr cs:[gVisibleWidth]
	jle	L_151E
L_14A8:
	lodsb
	or	al, al
	je	L_1520
	push	dx
	push	cx
	push	bx
	push	es
	push	di
	push	ax
	mov	ax, di
	push	ax
	and	ax, 7
	mov	cx, ax
	mov	al, 1
	clc
L_14BE:
	cmp	cx, 0
	jle	L_14C8
	shl	al, 1
	dec	cx
	jmp	short L_14BE
L_14C8:
	mov	byte ptr cs:[gMaskBit], al
	pop	ax
	shr	ax, 1
	shr	ax, 1
	shr	ax, 1
	mov	di, ax
	pop	ax
	mov	es, word ptr cs:[gMaskPlaneSeg]
	xor	bx, bx
	push	ax
	cmp	word ptr cs:[gMaskLevel], 1
	jge	L_14F2
	mov	al, byte ptr es:[di + MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_14F2:
	cmp	word ptr cs:[gMaskLevel], 2
	jge	L_1506
	mov	al, byte ptr es:[di + 2 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1506:
	cmp	word ptr cs:[gMaskLevel], 3
	jge	L_1522
	mov	al, byte ptr es:[di + 3 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
	jmp	short L_1522
	jmp	short L_156F		; QUIRK 151C: dead code, the jmp above skips it
L_151E:
	jmp	short L_159D
L_1520:
	jmp	short L_1596
L_1522:
	cmp	word ptr cs:[gMaskLevel], 4
	jge	L_1536
	mov	al, byte ptr es:[di + 4 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1536:
	cmp	word ptr cs:[gMaskLevel], 5
	jge	L_154F
	mov	al, byte ptr es:[di + 5 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
	jmp	short L_154F
L_154C:
	jmp	L_14A8
L_154F:
	cmp	word ptr cs:[gMaskLevel], 6
	jge	L_1563
	mov	al, byte ptr es:[di + 6 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1563:
	mov	al, byte ptr es:[di + 7 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_156F:
	pop	ax
	pop	di
	cmp	bl, 0
	je	L_1585
	mov	es, word ptr cs:[gBackPageSeg]
	mov	bl, byte ptr es:[di]
	pop	es
	mov	byte ptr es:[di], bl
	inc	di
	jmp	short L_158A
L_1585:
	pop	es
	mov	byte ptr es:[di], al
	inc	di
L_158A:
	pop	bx
	pop	cx
	pop	dx
	jmp	short L_1592
L_158F:
	jmp	L_1475
L_1592:
	loop	L_154C
	jmp	short L_159D
L_1596:
	lodsb
	add	di, ax
	sub	cx, ax
	jg	L_154C
L_159D:
	add	cx, word ptr cs:[gSkipRight]
	jcxz	L_15C7
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_15AC:
	xor	ax, ax
	repne scasb
	je	L_15B4
	jcxz	L_15BD
L_15B4:
	inc	cx
	mov	al, byte ptr es:[di]
	inc	di
	sub	cx, ax
	jne	L_15AC
L_15BD:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_15C7:
	add	bp, SCREEN_WIDTH
	mov	di, bp
	dec	dx
	jne	L_158F
L_15D0:
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
DrawShapeMasked	ENDP

; MCGA.OVL 15DA (export 22, gPutPixel): PutPixel(x, y, colour) sets one pixel of the draw page; off the
; screen it does nothing
PutPixel	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	es
	push	di
	mov	bx, word ptr [bp + 8]
	or	bx, bx
	js	L_160C
	cmp	bx, SCREEN_HEIGHT
	jge	L_160C
	shl	bx, 1
	mov	di, word ptr cs:[bx + gRowOffsets]
	mov	ax, word ptr [bp + 6]
	or	ax, ax
	js	L_160C
	cmp	ax, SCREEN_WIDTH
	jge	L_160C
	add	di, ax
	call	DrawPageSeg
	mov	es, ax
	mov	ax, word ptr [bp + 0Ah]
	stosb
L_160C:
	pop	di
	pop	es
	pop	bx
	pop	ax
	pop	bp
	retf
PutPixel	ENDP

; MCGA.OVL 1612 (export 23, gRestoreDrawState): puts back the region SaveDrawState kept
RestoreDrawState	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	mov	ax, word ptr cs:[gSavedLeft]
	mov	word ptr cs:[gRegionLeft], ax
	mov	ax, word ptr cs:[gSavedRight]
	mov	word ptr cs:[gRegionRight], ax
	mov	ax, word ptr cs:[gSavedTop]
	mov	word ptr cs:[gRegionTop], ax
	mov	ax, word ptr cs:[gSavedBottom]
	mov	word ptr cs:[gRegionBottom], ax
	pop	ax
	pop	bp
	retf
RestoreDrawState	ENDP

; MCGA.OVL 1639 (export 24, gDrawShapeFlipped): DrawShape mirrored left to right (both formats)
DrawShapeFlipped	PROC	NEAR
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
	mov	word ptr cs:[gSkipLeft], 0
	xor	di, di
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	es, ax
	mov	word ptr cs:[gDstSeg], ax
	mov	si, word ptr [bp + 8]
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gShapeSeg], ax
	mov	bx, word ptr [bp + 0Ch]
	mov	cx, word ptr [bp + 0Eh]
	mov	word ptr cs:[gShapeY], cx
	mov	ax, word ptr cs:[gRegionLeft]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	add	di, ax
	mov	word ptr cs:[gShapeUnused], 0
	sub	bx, ax
	mov	word ptr cs:[gShapeX], bx
	mov	ax, word ptr cs:[gRegionRight]
	sub	ax, word ptr cs:[gRegionLeft]
	mov	word ptr cs:[gRegionCols], ax
	push	ds
	mov	ax, word ptr cs:[gShapeSeg]
	mov	ds, ax
	lodsb
	or	al, al
	je	L_16A5
	jmp	L_1843
L_16A5:
	lodsw
	inc	si
	pop	ds
	push	ax
	and	ax, 0FFh
	mov	word ptr cs:[gShapeRows], ax
	pop	ax
	xchg	al, ah
	and	ax, 0FFh
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	word ptr cs:[gShapeWidth], ax
	mov	word ptr cs:[gShapeFullWidth], ax
	mov	ax, word ptr cs:[gShapeY]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_1714
	add	word ptr cs:[gShapeRows], ax
	jg	L_16D9
	jmp	L_178C
L_16D9:
	neg	ax
	mov	dx, ax
	mov	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipLeft], ax
	push	di
	push	es
	mov	di, si
	mov	ax, word ptr cs:[gShapeSeg]
	mov	es, ax
L_16EF:
	mov	cx, word ptr cs:[gSkipLeft]
L_16F4:
	xor	ax, ax
	repne scasb
	je	L_16FC
	jcxz	L_1705
L_16FC:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jne	L_16F4
L_1705:
	dec	dx
	jne	L_16EF
	mov	si, di
	pop	es
	pop	di
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gShapeY], ax
L_1714:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gShapeY]
	js	L_178C
	je	L_178C
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_172C
	mov	word ptr cs:[gShapeRows], ax
L_172C:
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr cs:[gShapeX]
	or	ax, ax
	jns	L_1753
	add	word ptr cs:[gShapeWidth], ax
	neg	ax
	mov	word ptr cs:[gSkipLeft], ax
	cmp	ax, word ptr cs:[gShapeFullWidth]
	jae	L_178C
	xor	ax, ax
	mov	word ptr cs:[gShapeX], ax
L_1753:
	mov	word ptr cs:[gSkipRight], 0
	mov	ax, word ptr cs:[gRegionCols]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr cs:[gShapeX]
	js	L_178C
	je	L_178C
	cmp	ax, word ptr cs:[gShapeWidth]
	jae	L_178F
	mov	word ptr cs:[gShapeWidth], ax
	mov	ax, word ptr cs:[gShapeFullWidth]
	sub	ax, word ptr cs:[gSkipLeft]
	sub	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipRight], ax
	jmp	short L_178F
L_178C:
	jmp	L_1839
L_178F:
	mov	ax, word ptr cs:[gShapeY]
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	ax, word ptr cs:[gShapeX]
	add	di, ax
	mov	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gVisibleWidth], ax
	add	di, ax
	dec	di
	mov	bp, di
	mov	dx, word ptr cs:[gShapeRows]
	mov	ax, word ptr cs:[gShapeSeg]
	mov	ds, ax
	xor	ax, ax
	mov	bx, es
L_17BF:
	mov	cx, word ptr cs:[gSkipRight]
	jcxz	L_17E9
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_17CE:
	xor	ax, ax
	repne scasb
	je	L_17D6
	jcxz	L_17DF
L_17D6:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jg	L_17CE
L_17DF:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_17E9:
	add	di, cx
	add	cx, word ptr cs:[gVisibleWidth]
	jle	L_1806
L_17F2:
	lodsb
	or	al, al
	je	L_17FF
	mov	byte ptr es:[di], al
	dec	di
	loop	L_17F2
	jmp	short L_1806
L_17FF:
	lodsb
	sub	di, ax
	sub	cx, ax
	jg	L_17F2
L_1806:
	add	cx, word ptr cs:[gSkipLeft]
	jcxz	L_1830
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_1815:
	xor	ax, ax
	repne scasb
	je	L_181D
	jcxz	L_1826
L_181D:
	inc	cx
	mov	al, byte ptr es:[di]
	inc	di
	sub	cx, ax
	jne	L_1815
L_1826:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_1830:
	add	bp, SCREEN_WIDTH
	mov	di, bp
	dec	dx
	jne	L_17BF
L_1839:
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
L_1843:
	lodsw
	inc	si
	mov	bx, si
	add	si, 10h
	pop	ds
	push	ax
	and	ax, 0FFh
	mov	word ptr cs:[gShapeRows], ax
	pop	ax
	xchg	al, ah
	and	ax, 0FFh
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	word ptr cs:[gShapeWidth], ax
	mov	word ptr cs:[gShapeFullWidth], ax
	mov	ax, word ptr cs:[gShapeY]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_18A2
	neg	ax
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_18E4
	mov	dx, word ptr cs:[gShapeWidth]
	shr	dx, 1
	mul	dx
	add	si, ax
	mov	ax, word ptr cs:[gShapeRows]
	add	ax, word ptr cs:[gShapeY]
	sub	ax, word ptr cs:[gRegionTop]
	js	L_18BC
	mov	word ptr cs:[gShapeRows], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gShapeY], ax
L_18A2:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gShapeY]
	js	L_18BC
	je	L_18BC
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_18BF
	mov	word ptr cs:[gShapeRows], ax
	jmp	short L_18BF
L_18BC:
	jmp	L_1839
L_18BF:
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr cs:[gShapeX]
	mov	word ptr cs:[gOddX], ax
	and	word ptr cs:[gOddX], 1
	or	ax, ax
	jns	L_18F1
	add	word ptr cs:[gShapeWidth], ax
	neg	ax
	cmp	ax, word ptr cs:[gShapeFullWidth]
L_18E4:
	jae	L_18BC
	xor	ax, ax
	mov	word ptr cs:[gShapeX], ax
	inc	word ptr cs:[gSkipLeft]
L_18F1:
	mov	word ptr cs:[gSkipRight], 0
	mov	ax, word ptr cs:[gRegionCols]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr cs:[gShapeX]
	js	L_18BC
	je	L_18BC
	cmp	ax, word ptr cs:[gShapeWidth]
	jae	L_192A
	mov	dx, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gShapeWidth], ax
	shr	dx, 1
	shr	ax, 1
	add	si, dx
	sub	si, ax
	inc	word ptr cs:[gSkipRight]
	jmp	short L_192A
L_192A:
	push	bx
	mov	bx, word ptr cs:[gShapeY]
	shl	bx, 1
	add	di, word ptr cs:[bx + gRowOffsets]
	add	di, word ptr cs:[gShapeX]
	pop	bx
	mov	ax, SCREEN_WIDTH
	sub	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gShapeY], ax
	shr	word ptr cs:[gShapeWidth], 1
	shr	word ptr cs:[gShapeFullWidth], 1
	dec	si
	mov	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gRowBytes], ax
	mov	word ptr cs:[gSrcRowStart], ax
	cmp	word ptr cs:[gSkipLeft], 0
	je	L_198F
	test	word ptr cs:[gOddX], 1
	je	L_198F
	dec	word ptr cs:[gShapeFullWidth]
	add	word ptr cs:[gShapeY], 2
	inc	word ptr cs:[gSrcRowStart]
	cmp	word ptr cs:[gSkipRight], 1
	jne	L_19A6
	sub	word ptr cs:[gShapeY], 2
L_198F:
	cmp	word ptr cs:[gSkipRight], 0
	je	L_19A6
	test	word ptr cs:[gOddX], 1
	je	L_19A6
	inc	word ptr cs:[gSrcRowStart]
	dec	si
L_19A6:
	mov	ax, word ptr cs:[gShapeFullWidth]
	mov	word ptr cs:[gSrcRowSkip], ax
	mov	ax, word ptr cs:[gShapeY]
	mov	word ptr cs:[gDstRowSkip], ax
	mov	ax, word ptr cs:[gShapeRows]
	mov	word ptr cs:[gRowsLeft], ax
	mov	dx, word ptr cs:[gShapeSeg]
	mov	ax, word ptr cs:[gDstSeg]
	mov	es, ax
	mov	ds, dx
	xor	dx, dx
L_19CD:
	mov	cx, word ptr cs:[gRowBytes]
	add	si, word ptr cs:[gSrcRowStart]
	cmp	word ptr cs:[gSkipLeft], 0
	je	L_19FA
	cmp	word ptr cs:[gOddX], 0
	je	L_19FA
	jcxz	L_19F7
	mov	dl, byte ptr [si]
	dec	si
	xchg	bx, dx
	mov	al, byte ptr cs:[bx + gNibbleSwap]
	xchg	bx, dx
	jmp	short L_1A1C
L_19F7:
	jmp	L_1839
L_19FA:
	jcxz	L_1A3A
L_19FC:
	mov	al, byte ptr [si]
	dec	si
	mov	dl, al
	xchg	bx, dx
	mov	al, byte ptr cs:[bx + gNibbleSwap]
	xchg	bx, dx
	mov	ah, al
	shr	al, 1
	shr	al, 1
	shr	al, 1
	shr	al, 1
	xlat
	or	al, al
	je	L_1A28
	stosb
	mov	al, ah
L_1A1C:
	and	al, 0Fh
	xlat
	or	al, al
	je	L_1A37
	stosb
	loop	L_19FC
	jmp	short L_1A3A
L_1A28:
	inc	di
	mov	al, ah
	and	al, 0Fh
	xlat
	or	al, al
	je	L_1A37
	stosb
	loop	L_19FC
	jmp	short L_1A3A
L_1A37:
	inc	di
	loop	L_19FC
L_1A3A:
	cmp	word ptr cs:[gSkipRight], 0
	je	L_1A63
	cmp	word ptr cs:[gOddX], 0
	je	L_1A63
	mov	dl, byte ptr [si]
	dec	si
	xchg	bx, dx
	mov	al, byte ptr cs:[bx + gNibbleSwap]
	xchg	bx, dx
	mov	cl, 4
	shr	al, cl
	xlat
	or	al, al
	je	L_1A62
	mov	byte ptr es:[di], al
L_1A62:
	inc	di
L_1A63:
	add	si, word ptr cs:[gSrcRowSkip]
	add	di, word ptr cs:[gDstRowSkip]
	dec	word ptr cs:[gRowsLeft]
	je	L_1A77
	jmp	L_19CD
L_1A77:
	jmp	L_1839
DrawShapeFlipped	ENDP

; MCGA.OVL 1A7A (export 25, gSaveDrawState): keeps the region and sets it to the whole screen (0, 40, 0,
; 200); the game calls it around each pointer redraw
SaveDrawState	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
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
	pop	ax
	pop	bp
	retf
SaveDrawState	ENDP

; MCGA.OVL 1ABD (export 14, gSaveRectFromPage): SaveRect from the page gDrawPage names instead of page 0
SaveRectFromPage	PROC	NEAR
	push	bp
	mov	bp, sp
	sub	sp, 2
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
	pop	bp		; QUIRK 1ADA: dead code, the jmp above skips it
	retf
SaveRectFromPage	ENDP

; MCGA.OVL 1ADC (export 26, gSaveRect): SaveRect(col, y, cols, rows, void far *buf) copies a rectangle of
; page 0 (cols 8-pixel columns, rows lines) into buf for RestoreRect; returns the bytes written (0 for an empty
; rectangle)
SaveRect	PROC	NEAR
	push	bp
	mov	bp, sp
	sub	sp, 2
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
	jge	L_1AFA
	xor	si, si
L_1AFA:
	cmp	si, SCREEN_COLS
	jl	L_1B02
	mov	si, SCREEN_COLS - 1
L_1B02:
	shl	si, 1
	shl	si, 1
	shl	si, 1
	mov	dx, word ptr [bp + 8]
	cmp	dx, 0
	jge	L_1B12
	xor	dx, dx
L_1B12:
	cmp	dx, SCREEN_HEIGHT
	jl	L_1B1B
	mov	dx, SCREEN_HEIGHT - 1
L_1B1B:
	mov	bx, word ptr [bp + 0Ah]
	cmp	bx, 0
	jg	L_1B27
L_1B23:
	xor	ax, ax
	jmp	short L_1B92
L_1B27:
	cmp	bx, SCREEN_COLS
	jle	L_1B2F
	mov	bx, SCREEN_COLS
L_1B2F:
	mov	cx, word ptr [bp + 0Ch]
	cmp	cx, 0
	jle	L_1B23
	cmp	cx, SCREEN_HEIGHT
	jle	L_1B40
	mov	cx, SCREEN_HEIGHT
L_1B40:
	mov	di, word ptr [bp + 0Eh]
	push	di
	mov	es, word ptr [bp + 10h]
	cmp	bx, SCREEN_COLS
	jbe	L_1B4F
	mov	bx, SCREEN_COLS
L_1B4F:
	or	bx, bx
	jne	L_1B55
L_1B53:
	jmp	short L_1B8D
L_1B55:
	cmp	cx, SCREEN_HEIGHT
	jbe	L_1B5E
	mov	cx, SCREEN_HEIGHT
L_1B5E:
	or	cx, cx
	je	L_1B53
	push	bx
	mov	bx, dx
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	si, ax
	pop	bx
	mov	dx, cx
	shl	bx, 1
	shl	bx, 1
	mov	ax, word ptr [bp - 2]
	call	PageSeg
	mov	ds, ax
	mov	ax, SCREEN_WIDTH / 2
	sub	ax, bx
	shl	ax, 1
L_1B84:
	mov	cx, bx
	rep movsw
	add	si, ax
	dec	dx
	jne	L_1B84
L_1B8D:
	mov	ax, di
	pop	di
	sub	ax, di
L_1B92:
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

; MCGA.OVL 1B9D (export 27, ds:80B2): SetLineClip(left, top, right, bottom) sets the rectangle DrawLine
; clips to, in pixels (the image holds the whole screen). No call of ds:80B2 was found in EOB.EXE or INTRO.EXE.
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

; MCGA.OVL 1BC0 (export 28, gSetRegion): SetRegion(left, right, top, bottom) sets the region the shape
; draws clip to: left and right in 8-pixel columns, top and bottom in lines
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

; MCGA.OVL 1BE3 (export 29, gSetPalette): SetPalette(void far *rgb) loads all 256 DAC registers from 768
; bytes of red, green, blue (INT 10h AX=1012h)
SetPalette	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	es
	mov	dx, word ptr [bp + 6]
	mov	es, word ptr [bp + 8]
	mov	ax, 1012h
	mov	bx, 0
	mov	cx, 100h
	int	10h
	pop	es
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
SetPalette	ENDP

; MCGA.OVL 1C03 (export 30, ds:80BE): SetTileFormat(width, height, arg3, arg4) sets the tile size for
; DrawIndexedTile, in 8-pixel cells: 1 by 1 and 2 by 2 are drawn, any other size makes DrawIndexedTile draw
; nothing (gTileMode 4). arg3 and arg4 are stored and never read. No call of ds:80BE was found in EOB.EXE or
; INTRO.EXE.
SetTileFormat	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gTileFormatArg3], ax
	mov	ax, word ptr [bp + 0Ch]
	mov	word ptr cs:[gTileFormatArg4], ax
	mov	ax, word ptr [bp + 6]
	mov	bx, word ptr [bp + 8]
	cmp	ax, bx
	jne	L_1C6C
	cmp	ax, 3
	jge	L_1C6C
	mov	word ptr cs:[gTileReqWidth], 1
	mov	word ptr cs:[gTileReqHeight], 1
	mov	cx, ax
	and	cx, 2
	mov	word ptr cs:[gTileMode], cx
	mov	cx, ax
	rcr	cx, 1
	shl	cx, 1
	rcl	cx, 1
	shl	cx, 1
	mov	word ptr cs:[gTileParas], cx
	mov	cx, ax
	shl	cx, 1
	shl	cx, 1
	mov	word ptr cs:[gTileRowBytes], cx
	shl	cx, 1
	mov	word ptr cs:[gTileRows], cx
	neg	cx
	add	cx, SCREEN_WIDTH
	mov	word ptr cs:[gTileDstSkip], cx
	jmp	short L_1C9D
L_1C6C:
	mov	word ptr cs:[gTileMode], 4
	mov	word ptr cs:[gTileParas], 2
	mov	word ptr cs:[gTileRowBytes], 4
	mov	word ptr cs:[gTileRows], 8
	mov	word ptr cs:[gTileDstSkip], SCREEN_WIDTH - 8
	mov	word ptr cs:[gTileReqWidth], ax
	mov	word ptr cs:[gTileReqHeight], bx
	mov	ax, 1
	mov	bx, ax
L_1C9D:
	mov	word ptr cs:[gTileWidthCells], ax
	mov	word ptr cs:[gTileHeightCells], bx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
SetTileFormat	ENDP

; MCGA.OVL 1CAB (export 31, gDrawShape): DrawShape(page, void far *shape, x, y) draws a shape at pixel
; (x, y), clipped to the region. Format 0 (first byte 0): rows (byte 1), width / 8 (byte 2), then rows of
; 8-bit pixels where 0 and a count is a run of transparent pixels. Any other first byte: a 4-bit shape, the same
; two size bytes, a 16-byte colour table of its own (bx), then two pixels a byte, colour 0 transparent.
DrawShape	PROC	NEAR
	push	bp
	mov	bp, sp
	sub	sp, 18h
	push	ax
	push	bx
	push	cx
	push	dx
	push	ds
	push	es
	push	di
	push	si
	cld
	xor	di, di
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	es, ax
	mov	si, word ptr [bp + 8]
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr [bp - 14h], ax
	mov	bx, word ptr [bp + 0Ch]
	mov	cx, word ptr [bp + 0Eh]
	mov	word ptr [bp - 12h], cx
	mov	ax, word ptr cs:[gRegionLeft]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	add	di, ax
	sub	bx, ax
	mov	word ptr [bp - 10h], bx
	mov	ax, word ptr cs:[gRegionRight]
	sub	ax, word ptr cs:[gRegionLeft]
	mov	word ptr [bp - 0Eh], ax
	push	ds
	mov	ax, word ptr [bp - 14h]
	mov	ds, ax
	lodsb
	or	al, al
	je	L_1D01
	jmp	L_1E68
L_1D01:
	lodsw
	inc	si
	pop	ds
	push	ax
	and	ax, 0FFh
	mov	word ptr [bp - 0Ch], ax
	pop	ax
	xchg	al, ah
	and	ax, 0FFh
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	word ptr [bp - 0Ah], ax
	mov	word ptr [bp - 8], ax
	mov	ax, word ptr [bp - 12h]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_1D64
	add	word ptr [bp - 0Ch], ax
	jg	L_1D2F
	jmp	L_1DC0
L_1D2F:
	neg	ax
	mov	dx, ax
	mov	ax, word ptr [bp - 0Ah]
	mov	word ptr [bp - 6], ax
	push	di
	push	es
	mov	di, si
	mov	ax, word ptr [bp - 14h]
	mov	es, ax
L_1D42:
	mov	cx, word ptr [bp - 6]
L_1D45:
	xor	ax, ax
	repne scasb
	je	L_1D4D
	jcxz	L_1D56
L_1D4D:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jne	L_1D45
L_1D56:
	dec	dx
	jne	L_1D42
	mov	si, di
	pop	es
	pop	di
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr [bp - 12h], ax
L_1D64:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr [bp - 12h]
	js	L_1DC0
	je	L_1DC0
	cmp	ax, word ptr [bp - 0Ch]
	jae	L_1D77
	mov	word ptr [bp - 0Ch], ax
L_1D77:
	mov	word ptr [bp - 6], 0
	mov	ax, word ptr [bp - 10h]
	or	ax, ax
	jns	L_1D95
	add	word ptr [bp - 0Ah], ax
	neg	ax
	mov	word ptr [bp - 6], ax
	cmp	ax, word ptr [bp - 8]
	jae	L_1DC0
	xor	ax, ax
	mov	word ptr [bp - 10h], ax
L_1D95:
	mov	word ptr [bp - 4], 0
	mov	ax, word ptr [bp - 0Eh]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr [bp - 10h]
	js	L_1DC0
	je	L_1DC0
	cmp	ax, word ptr [bp - 0Ah]
	jae	L_1DC3
	mov	word ptr [bp - 0Ah], ax
	mov	ax, word ptr [bp - 8]
	sub	ax, word ptr [bp - 6]
	sub	ax, word ptr [bp - 0Ah]
	mov	word ptr [bp - 4], ax
	jmp	short L_1DC3
L_1DC0:
	jmp	L_1E5C
L_1DC3:
	mov	ax, word ptr [bp - 12h]
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	ax, word ptr [bp - 10h]
	add	di, ax
	mov	word ptr [bp - 16h], di
	mov	ax, word ptr [bp - 0Ah]
	mov	word ptr [bp - 2], ax
	mov	dx, word ptr [bp - 0Ch]
	mov	ax, word ptr [bp - 14h]
	mov	ds, ax
	xor	ax, ax
	mov	bx, es
L_1DE9:
	mov	cx, word ptr [bp - 6]
	jcxz	L_1E11
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_1DF6:
	xor	ax, ax
	repne scasb
	je	L_1DFE
	jcxz	L_1E07
L_1DFE:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jg	L_1DF6
L_1E07:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_1E11:
	sub	di, cx
	add	cx, word ptr [bp - 2]
	jle	L_1E29
L_1E18:
	lodsb
	or	al, al
	je	L_1E22
	stosb
	loop	L_1E18
	jmp	short L_1E29
L_1E22:
	lodsb
	add	di, ax
	sub	cx, ax
	jg	L_1E18
L_1E29:
	add	cx, word ptr [bp - 4]
	jcxz	L_1E51
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_1E36:
	xor	ax, ax
	repne scasb
	je	L_1E3E
	jcxz	L_1E47
L_1E3E:
	inc	cx
	mov	al, byte ptr es:[di]
	inc	di
	sub	cx, ax
	jne	L_1E36
L_1E47:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_1E51:
	add	word ptr [bp - 16h], SCREEN_WIDTH
	mov	di, word ptr [bp - 16h]
	dec	dx
	jne	L_1DE9
L_1E5C:
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
L_1E68:
	lodsw
	inc	si
	mov	bx, si
	add	si, 10h
	pop	ds
	push	ax
	and	ax, 0FFh
	mov	word ptr [bp - 0Ch], ax
	pop	ax
	xchg	al, ah
	and	ax, 0FFh
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	word ptr [bp - 0Ah], ax
	mov	word ptr [bp - 8], ax
	mov	ax, word ptr [bp - 12h]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_1EBA
	neg	ax
	cmp	ax, word ptr [bp - 0Ch]
	jae	L_1EF2
	mov	dx, word ptr [bp - 0Ah]
	shr	dx, 1
	mul	dx
	add	si, ax
	mov	ax, word ptr [bp - 0Ch]
	add	ax, word ptr [bp - 12h]
	sub	ax, word ptr cs:[gRegionTop]
	js	L_1EC1
	mov	word ptr [bp - 0Ch], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr [bp - 12h], ax
L_1EBA:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr [bp - 12h]
L_1EC1:
	js	L_1ECF
	je	L_1ECF
	cmp	ax, word ptr [bp - 0Ch]
	jae	L_1ED1
	mov	word ptr [bp - 0Ch], ax
	jmp	short L_1ED1
L_1ECF:
	jmp	short L_1E5C
L_1ED1:
	mov	word ptr [bp - 6], 0
	mov	ax, word ptr [bp - 10h]
	mov	word ptr [bp - 2], ax
	and	word ptr [bp - 2], 1
	or	ax, ax
	jns	L_1EFC
	add	word ptr [bp - 0Ah], ax
	neg	ax
	mov	dx, ax
	shr	dx, 1
	add	si, dx
	cmp	ax, word ptr [bp - 8]
L_1EF2:
	jae	L_1ECF
	xor	ax, ax
	mov	word ptr [bp - 10h], ax
	inc	word ptr [bp - 6]
L_1EFC:
	mov	word ptr [bp - 4], 0
	mov	ax, word ptr [bp - 0Eh]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	dx, ax
	sub	ax, word ptr [bp - 10h]
	js	L_1ECF
	je	L_1ECF
	cmp	ax, word ptr [bp - 0Ah]
	jae	L_1F23
	sub	dx, word ptr [bp - 10h]
	mov	word ptr [bp - 0Ah], dx
	inc	word ptr [bp - 4]
	jmp	short L_1F23
L_1F23:
	mov	ax, word ptr [bp - 12h]
	mov	dx, SCREEN_WIDTH
	mul	dx
	add	ax, word ptr [bp - 10h]
	add	di, ax
	mov	ax, SCREEN_WIDTH
	sub	ax, word ptr [bp - 0Ah]
	mov	word ptr [bp - 12h], ax
	mov	dx, word ptr [bp - 14h]
	mov	word ptr [bp - 16h], ds
	shr	word ptr [bp - 0Ah], 1
	shr	word ptr [bp - 8], 1
	mov	ax, word ptr [bp - 0Ah]
	mov	word ptr [bp - 18h], ax
	cmp	word ptr [bp - 6], 0
	je	L_1F68
	test	word ptr [bp - 2], 1
	je	L_1F68
	inc	word ptr [bp - 18h]
	inc	word ptr [bp - 8]
	cmp	word ptr [bp - 4], 0
	jne	L_1F6E
	add	word ptr [bp - 12h], 2
L_1F68:
	cmp	word ptr [bp - 4], 0
	je	L_1F78
L_1F6E:
	test	word ptr [bp - 2], 1
	je	L_1F78
	inc	word ptr [bp - 18h]
L_1F78:
	mov	cx, word ptr [bp - 0Ah]
	mov	ds, dx
	cmp	word ptr [bp - 6], 0
	je	L_1F8E
	cmp	word ptr [bp - 2], 0
	je	L_1F8E
	jcxz	L_1FD8
	lodsb
	jmp	short L_1FA3
L_1F8E:
	jcxz	L_1FAD
L_1F90:
	lodsb
	mov	ah, al
	shr	al, 1
	shr	al, 1
	shr	al, 1
	shr	al, 1
	xlat
	or	al, al
	je	L_1FDB
	stosb
	mov	al, ah
L_1FA3:
	and	al, 0Fh
	xlat
	or	al, al
	je	L_1FEA
	stosb
	loop	L_1F90
L_1FAD:
	cmp	word ptr [bp - 4], 0
	je	L_1FC7
	cmp	word ptr [bp - 2], 0
	je	L_1FC7
	lodsb
	mov	cl, 4
	shr	al, cl
	xlat
	or	al, al
	je	L_1FC6
	mov	byte ptr es:[di], al
L_1FC6:
	inc	di
L_1FC7:
	mov	ds, word ptr [bp - 16h]
	add	si, word ptr [bp - 8]
	sub	si, word ptr [bp - 18h]
	add	di, word ptr [bp - 12h]
	dec	word ptr [bp - 0Ch]
	jne	L_1F78
L_1FD8:
	jmp	L_1E5C
L_1FDB:
	inc	di
	mov	al, ah
	and	al, 0Fh
	xlat
	or	al, al
	je	L_1FEA
	stosb
	loop	L_1F90
	jmp	short L_1FAD
L_1FEA:
	inc	di
	loop	L_1F90
	jmp	short L_1FAD
	pop	bp		; QUIRK 1FEF: dead code, the jmp above skips it
	retf
DrawShape	ENDP

; MCGA.OVL 1FF1 (export 32, gSaveRectSize): SaveRectSize(cols, rows) returns the bytes SaveRect needs,
; cols * rows * 8, cols clamped to 1 to 40 and rows to 1 to 200
SaveRectSize	PROC	NEAR
	push	bp
	mov	bp, sp
	push	cx
	push	dx
	mov	ax, word ptr [bp + 6]
	cmp	ax, 1
	jge	L_2001
	mov	ax, 1
L_2001:
	cmp	ax, SCREEN_COLS
	jle	L_2009
	mov	ax, SCREEN_COLS
L_2009:
	mov	dx, word ptr [bp + 8]
	cmp	dx, 1
	jge	L_2014
	mov	dx, 1
L_2014:
	cmp	dx, SCREEN_HEIGHT
	jle	L_201D
	mov	dx, SCREEN_HEIGHT
L_201D:
	mul	dx
	mov	cl, 3
	shl	ax, cl
	pop	dx
	pop	cx
	pop	bp
	retf
SaveRectSize	ENDP

; cs:2027: DrawGlyph's and XorRect's state
gBlitRowSkip	dw	0		; screen bytes from the end of one row to the next
	db	6 dup (0)		; 2029: not used
gXorBytes	dw	0		; XorRect: pixels in the first byte group
gBlitWidth	db	0		; DrawGlyph: the glyph's width; XorRect: whole 8-pixel groups
gBlitRows	db	0		; rows left
gBlitX	db	0
gBlitY	db	0
gBlitColour	db	0		; the text colour; XorRect's colour
gBlitBack	db	0		; the text background (0: transparent)
	db	4 dup (0)		; 2037: not used
gTextLeft	dw	0		; DrawText: the line start x
gLineHeight	dw	0
gCharWidth	dw	0
gLeftMask	db	0		; XorRect: the bit mask of the first group
gRightMask	db	0
gGlyphBytes	db	0
gFontSeg	dw	0		; the font's segment
; cs:2046: for each width of 1 to 24 pixels, a set of eight bit masks, one per starting bit, MSB first:
; gBitMaskTable[w - 1] -> gMaskSet<w>, whose word p points at the mask bytes for a span of w bits
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

; MCGA.OVL 241E (near): DrawGlyph(ch, x, y), arguments on the stack: draws character ch of the current font
; (gCurFont: +2 the glyph offsets, +102h the height, +103h the width) at (x, y) on the draw page, set bits in
; gBlitColour and clear bits in gBlitBack unless it is 0; a glyph off the screen or wider than 24 is not drawn
DrawGlyph	PROC	NEAR
	push	bp
	mov	bp, sp
	push	bx
	push	di
	push	es
	push	si
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
	jb	L_244D
L_244A:
	jmp	L_253D
L_244D:
	mov	ax, dx
	mov	byte ptr cs:[gBlitX], al
	xor	cx, cx
	mov	cl, byte ptr es:[si + FONT_WIDTH]
	mov	byte ptr cs:[gBlitWidth], cl
	add	ax, cx
	cmp	ax, SCREEN_WIDTH
	ja	L_244A
	mov	di, dx
	mov	dx, word ptr [bp + 8]
	cmp	dx, SCREEN_HEIGHT
	jae	L_244A
	mov	ax, dx
	mov	byte ptr cs:[gBlitY], al
	mov	cl, byte ptr es:[si + FONT_HEIGHT]
	mov	byte ptr cs:[gBlitRows], cl
	add	ax, cx
	cmp	ax, SCREEN_HEIGHT
	ja	L_244A
	add	si, bx
	xor	ah, ah
	mov	al, byte ptr cs:[gBlitWidth]
	dec	al
	cmp	al, 18h
	jae	L_244A
	mov	bp, ax
	add	al, 7
	shr	al, 1
	shr	al, 1
	shr	al, 1
	mov	byte ptr cs:[gGlyphBytes], al
	shl	bp, 1
	mov	bp, word ptr cs:[bp + gBitMaskTable]
	mov	al, byte ptr cs:[gBlitWidth]
	dec	al
	shr	al, 1
	shr	al, 1
	shr	al, 1
	mov	byte ptr cs:[gGlyphBytes], al
	mov	bp, word ptr cs:[bp]
	mov	ax, dx
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	di, ax
	mov	ax, SCREEN_WIDTH
	xor	dh, dh
	mov	dl, byte ptr cs:[gBlitWidth]
	sub	ax, dx
	mov	word ptr cs:[gBlitRowSkip], ax
	call	DrawPageSeg
	mov	dx, es
	mov	es, ax
	mov	ds, dx
	mov	bl, byte ptr cs:[gBlitColour]
	mov	bh, byte ptr cs:[gBlitBack]
L_24EF:
	mov	ch, byte ptr cs:[gGlyphBytes]
	mov	dx, bp
L_24F6:
	lodsb
	mov	ah, byte ptr cs:[bp]
	mov	cl, 80h
L_24FD:
	test	ah, cl
	je	L_252F
	test	al, cl
	je	L_251A
	or	bl, bl
	je	L_2521
	mov	byte ptr es:[di], bl
	inc	di
	shr	cl, 1
	jne	L_24FD
	or	ch, ch
	je	L_252F
	inc	bp
	dec	ch
	jmp	short L_24F6
L_251A:
	or	bh, bh
	je	L_2521
	mov	byte ptr es:[di], bh
L_2521:
	inc	di
	shr	cl, 1
	jne	L_24FD
	or	ch, ch
	je	L_252F
	inc	bp
	dec	ch
	jmp	short L_24F6
L_252F:
	mov	bp, dx
	add	di, word ptr cs:[gBlitRowSkip]
	dec	byte ptr cs:[gBlitRows]
	jne	L_24EF
L_253D:
	pop	si
	pop	es
	pop	di
	pop	bx
	pop	bp
	ret
DrawGlyph	ENDP

; MCGA.OVL 2543 (export 33, gDrawText): DrawText(char far *text, x, y, colour, background) draws a
; string in the current font: each character & 7Fh through DrawGlyph, CR starts a new line at x, a character
; that would pass the right edge wraps; the string ends at its 0 or at the bottom of the screen
DrawText	PROC	NEAR
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
	jge	L_2583
	xor	bx, bx
L_2583:
	cmp	bx, SCREEN_WIDTH
	jb	L_258C
	jmp	L_260D
L_258C:
	mov	word ptr cs:[gTextLeft], bx
	mov	cx, word ptr [bp + 0Ch]
	cmp	cx, 0
	jge	L_259B
	xor	cx, cx
L_259B:
	cmp	cx, SCREEN_HEIGHT
	jge	L_260D
	mov	al, byte ptr [bp + 0Eh]
	mov	byte ptr cs:[gBlitColour], al
	mov	al, byte ptr [bp + 10h]
	mov	byte ptr cs:[gBlitBack], al
L_25AF:
	cmp	byte ptr es:[si], 0
	je	L_260D
	cmp	byte ptr es:[si], 0Dh
	jne	L_25C8
	mov	bx, word ptr cs:[gTextLeft]
	add	cx, word ptr cs:[gLineHeight]
	inc	si
	jmp	short L_25AF
L_25C8:
	push	es
	mov	es, word ptr cs:[gFontSeg]
	xor	ah, ah
	mov	al, byte ptr es:[di + FONT_WIDTH]
	mov	word ptr cs:[gCharWidth], ax
	mov	dx, bx
	add	dx, ax
	pop	es
	cmp	dx, SCREEN_WIDTH
	jbe	L_25EE
	mov	bx, word ptr cs:[gTextLeft]
	add	cx, word ptr cs:[gLineHeight]
L_25EE:
	cmp	cx, SCREEN_HEIGHT
	jae	L_260D
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
	jmp	short L_25AF
L_260D:
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
DrawText	ENDP

; MCGA.OVL 2617 (export 8, gXorRect): XorRect(x1, y1, x2, y2, colour) XORs colour into every pixel of the
; rectangle (corners included, clamped to the screen, the corners in either order) on the draw page; spans of
; 8 pixels go through gBitMaskTable
XorRect	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	di
	push	dx
	push	es
	mov	di, word ptr [bp + 6]
	cmp	di, 0
	jge	L_262A
	xor	di, di
L_262A:
	cmp	di, SCREEN_WIDTH
	jl	L_2633
	mov	di, SCREEN_WIDTH - 1
L_2633:
	mov	dx, word ptr [bp + 8]
	cmp	dx, 0
	jge	L_263D
	xor	dx, dx
L_263D:
	cmp	dx, SCREEN_HEIGHT
	jl	L_2646
	mov	dx, SCREEN_HEIGHT - 1
L_2646:
	mov	ax, word ptr [bp + 0Ah]
	cmp	ax, 0
	jge	L_2650
	xor	ax, ax
L_2650:
	cmp	ax, SCREEN_WIDTH
	jl	L_2658
	mov	ax, SCREEN_WIDTH - 1
L_2658:
	cmp	ax, di
	jge	L_265D
	xchg	di, ax
L_265D:
	mov	bx, di
	and	bl, 7
	mov	byte ptr cs:[gLeftMask], bl
	mov	bh, al
	and	bh, 7
	sub	ax, di
	inc	ax
	mov	cx, ax
	cmp	ax, 8
	jle	L_2680
	xor	ch, ch
	mov	cl, 8
	sub	cl, bl
	add	cl, bh
	inc	cl
L_2680:
	mov	word ptr cs:[gXorBytes], cx
	sub	ax, cx
	shr	ax, 1
	shr	ax, 1
	shr	ax, 1
	mov	byte ptr cs:[gBlitWidth], al
	mov	ax, word ptr [bp + 0Ch]
	cmp	ax, 0
	jge	L_269B
	xor	ax, ax
L_269B:
	cmp	ax, SCREEN_HEIGHT
	jl	L_26A3
	mov	ax, SCREEN_HEIGHT - 1
L_26A3:
	cmp	ax, dx
	jge	L_26A8
	xchg	dx, ax
L_26A8:
	mov	byte ptr cs:[gBlitY], dl
	sub	ax, dx
	inc	ax
	mov	byte ptr cs:[gBlitRows], al
	mov	ax, word ptr [bp + 0Eh]
	mov	byte ptr cs:[gBlitColour], al
	mov	cx, word ptr cs:[gXorBytes]
	dec	cx
	mov	bp, cx
	add	cl, 7
	shr	cl, 1
	shr	cl, 1
	shr	cl, 1
	shl	bp, 1
	mov	bp, word ptr cs:[bp + gBitMaskTable]
	mov	bp, word ptr cs:[bp]
	mov	al, byte ptr cs:[bp]
	mov	byte ptr cs:[gLeftMask], al
	mov	al, 0
	or	cl, cl
	je	L_26EA
	inc	bp
	mov	al, byte ptr cs:[bp]
L_26EA:
	mov	byte ptr cs:[gRightMask], al
	mov	ax, dx
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	di, ax
	mov	ax, SCREEN_WIDTH
	sub	ax, word ptr cs:[gXorBytes]
	xor	dh, dh
	mov	dl, byte ptr cs:[gBlitWidth]
	shl	dx, 1
	shl	dx, 1
	shl	dx, 1
	sub	ax, dx
	mov	word ptr cs:[gBlitRowSkip], ax
	call	DrawPageSeg
	mov	es, ax
	mov	ch, byte ptr cs:[gBlitWidth]
	mov	bh, byte ptr cs:[gBlitRows]
	mov	bl, byte ptr cs:[gBlitColour]
	mov	dl, 80h
L_272C:
	mov	al, byte ptr cs:[gLeftMask]
	mov	cl, dl
L_2732:
	test	al, cl
	je	L_273E
	xor	byte ptr es:[di], bl
	inc	di
	shr	cl, 1
	jne	L_2732
L_273E:
	or	ch, ch
	je	L_2754
	mov	al, 0FFh
	mov	dh, ch
L_2746:
	mov	cl, dl
L_2748:
	xor	byte ptr es:[di], bl
	inc	di
	shr	cl, 1
	jne	L_2748
	dec	dh
	jne	L_2746
L_2754:
	mov	al, byte ptr cs:[gRightMask]
	or	al, al
	je	L_276A
	mov	cl, dl
L_275E:
	test	al, cl
	je	L_276A
	xor	byte ptr es:[di], bl
	inc	di
	shr	cl, 1
	jne	L_275E
L_276A:
	add	di, word ptr cs:[gBlitRowSkip]
	dec	bh
	jne	L_272C
	pop	es
	pop	dx
	pop	di
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
XorRect	ENDP

; MCGA.OVL 277B (export 35, gSetRemapTable): SetRemapTable(void far *colours) copies 16 colours into
; gRemapTable for the remapped draws
SetRemapTable	PROC	NEAR
	push	bp
	mov	bp, sp
	push	cx
	push	si
	push	di
	push	ds
	push	es
	mov	si, word ptr [bp + 6]
	mov	ds, word ptr [bp + 8]
	mov	di, offset gRemapTable
	mov	cx, cs
	mov	es, cx
	mov	cx, 8
	rep movsw
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	cx
	pop	bp
	retf
SetRemapTable	ENDP

; MCGA.OVL 279C (export 36, gDrawShapeRemapped): DrawShapeRemapped(page, void far *shape, x, y) draws a
; 4-bit shape like DrawShape but through gRemapTable instead of its own colours; a format 0 shape draws nothing
DrawShapeRemapped	PROC	NEAR
	push	bp
	mov	bp, sp
	sub	sp, 18h
	push	ax
	push	bx
	push	cx
	push	dx
	push	ds
	push	es
	push	di
	push	si
	cld
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	es, ax
	mov	si, word ptr [bp + 8]
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr [bp - 14h], ax
	mov	bx, word ptr [bp + 0Ch]
	mov	cx, word ptr [bp + 0Eh]
	mov	word ptr [bp - 12h], cx
	mov	ax, word ptr cs:[gRegionLeft]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	di, ax
	sub	bx, ax
	mov	word ptr [bp - 10h], bx
	mov	ax, word ptr cs:[gRegionRight]
	sub	ax, word ptr cs:[gRegionLeft]
	mov	word ptr [bp - 0Eh], ax
	push	ds
	mov	ax, word ptr [bp - 14h]
	mov	ds, ax
	lodsb
	or	al, al
	je	L_27EF
	jmp	short L_27FB
L_27EF:
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
L_27FB:
	lodsw
	add	si, 11h
	pop	ds
	push	ax
	and	ax, 0FFh
	mov	word ptr [bp - 0Ch], ax
	pop	ax
	xchg	al, ah
	and	ax, 0FFh
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	word ptr [bp - 0Ah], ax
	mov	word ptr [bp - 8], ax
	mov	ax, word ptr [bp - 12h]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_284A
	neg	ax
	cmp	ax, word ptr [bp - 0Ch]
	jae	L_2882
	mov	dx, word ptr [bp - 0Ah]
	shr	dx, 1
	mul	dx
	add	si, ax
	mov	ax, word ptr [bp - 0Ch]
	add	ax, word ptr [bp - 12h]
	sub	ax, word ptr cs:[gRegionTop]
	js	L_2851
	mov	word ptr [bp - 0Ch], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr [bp - 12h], ax
L_284A:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr [bp - 12h]
L_2851:
	js	L_285F
	je	L_285F
	cmp	ax, word ptr [bp - 0Ch]
	jae	L_2861
	mov	word ptr [bp - 0Ch], ax
	jmp	short L_2861
L_285F:
	jmp	short L_27EF
L_2861:
	mov	word ptr [bp - 6], 0
	mov	ax, word ptr [bp - 10h]
	mov	word ptr [bp - 2], ax
	and	word ptr [bp - 2], 1
	or	ax, ax
	jns	L_288C
	add	word ptr [bp - 0Ah], ax
	neg	ax
	mov	dx, ax
	shr	dx, 1
	add	si, dx
	cmp	ax, word ptr [bp - 8]
L_2882:
	jae	L_285F
	xor	ax, ax
	mov	word ptr [bp - 10h], ax
	inc	word ptr [bp - 6]
L_288C:
	mov	word ptr [bp - 4], 0
	mov	ax, word ptr [bp - 0Eh]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	dx, ax
	sub	ax, word ptr [bp - 10h]
	js	L_285F
	je	L_285F
	cmp	ax, word ptr [bp - 0Ah]
	jae	L_28B3
	sub	dx, word ptr [bp - 10h]
	mov	word ptr [bp - 0Ah], dx
	inc	word ptr [bp - 4]
	jmp	short L_28B3
L_28B3:
	mov	bx, word ptr [bp - 12h]
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	ax, word ptr [bp - 10h]
	add	di, ax
	mov	bx, offset gRemapTable
	mov	ax, SCREEN_WIDTH
	sub	ax, word ptr [bp - 0Ah]
	mov	word ptr [bp - 12h], ax
	mov	dx, word ptr [bp - 14h]
	mov	word ptr [bp - 16h], ds
	shr	word ptr [bp - 0Ah], 1
	shr	word ptr [bp - 8], 1
	mov	ax, word ptr [bp - 0Ah]
	mov	word ptr [bp - 18h], ax
	cmp	word ptr [bp - 6], 0
	je	L_28FD
	test	word ptr [bp - 2], 1
	je	L_28FD
	inc	word ptr [bp - 18h]
	inc	word ptr [bp - 8]
	cmp	word ptr [bp - 4], 0
	jne	L_2903
	add	word ptr [bp - 12h], 2
L_28FD:
	cmp	word ptr [bp - 4], 0
	je	L_290D
L_2903:
	test	word ptr [bp - 2], 1
	je	L_290D
	inc	word ptr [bp - 18h]
L_290D:
	mov	cx, word ptr [bp - 0Ah]
	mov	ds, dx
	cmp	word ptr [bp - 6], 0
	je	L_2923
	cmp	word ptr [bp - 2], 0
	je	L_2923
	jcxz	L_2970
	lodsb
	jmp	short L_2939
L_2923:
	jcxz	L_2944
L_2925:
	lodsb
	mov	ah, al
	shr	al, 1
	shr	al, 1
	shr	al, 1
	shr	al, 1
	xlat	byte ptr cs:[bx]
	or	al, al
	je	L_2973
	stosb
	mov	al, ah
L_2939:
	and	al, 0Fh
	xlat	byte ptr cs:[bx]
	or	al, al
	je	L_2983
	stosb
	loop	L_2925
L_2944:
	cmp	word ptr [bp - 4], 0
	je	L_295F
	cmp	word ptr [bp - 2], 0
	je	L_295F
	lodsb
	mov	cl, 4
	shr	al, cl
	xlat	byte ptr cs:[bx]
	or	al, al
	je	L_295E
	mov	byte ptr es:[di], al
L_295E:
	inc	di
L_295F:
	mov	ds, word ptr [bp - 16h]
	add	si, word ptr [bp - 8]
	sub	si, word ptr [bp - 18h]
	add	di, word ptr [bp - 12h]
	dec	word ptr [bp - 0Ch]
	jne	L_290D
L_2970:
	jmp	L_27EF
L_2973:
	inc	di
	mov	al, ah
	and	al, 0Fh
	xlat	byte ptr cs:[bx]
	or	al, al
	je	L_2983
	stosb
	loop	L_2925
	jmp	short L_2944
L_2983:
	inc	di
	loop	L_2925
	jmp	short L_2944
	pop	bp		; QUIRK 2988: dead code, the jmp above skips it
	retf
DrawShapeRemapped	ENDP

; MCGA.OVL 298A (export 37, gDrawShapeFlippedRemapped): DrawShapeRemapped mirrored left to right (each
; byte's pixels swapped through gNibbleSwap)
DrawShapeFlippedRemapped	PROC	NEAR
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
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	es, ax
	mov	word ptr cs:[gDstSeg], ax
	mov	si, word ptr [bp + 8]
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gShapeSeg], ax
	mov	bx, word ptr [bp + 0Ch]
	mov	cx, word ptr [bp + 0Eh]
	mov	word ptr cs:[gShapeY], cx
	mov	ax, word ptr cs:[gRegionLeft]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	di, ax
	mov	word ptr cs:[gShapeUnused], 0
	sub	bx, ax
	mov	word ptr cs:[gShapeX], bx
	mov	ax, word ptr cs:[gRegionRight]
	sub	ax, word ptr cs:[gRegionLeft]
	mov	word ptr cs:[gRegionCols], ax
	push	ds
	mov	ax, word ptr cs:[gShapeSeg]
	mov	ds, ax
	lodsb
	or	al, al
	je	L_29F3
	jmp	short L_29FD
L_29F3:
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
L_29FD:
	lodsw
	add	si, 11h
	pop	ds
	push	ax
	and	ax, 0FFh
	mov	word ptr cs:[gShapeRows], ax
	pop	ax
	xchg	al, ah
	and	ax, 0FFh
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	mov	word ptr cs:[gShapeWidth], ax
	mov	word ptr cs:[gShapeFullWidth], ax
	mov	ax, word ptr cs:[gShapeY]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_2A59
	neg	ax
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_2A9B
	mov	dx, word ptr cs:[gShapeWidth]
	shr	dx, 1
	mul	dx
	add	si, ax
	mov	ax, word ptr cs:[gShapeRows]
	add	ax, word ptr cs:[gShapeY]
	sub	ax, word ptr cs:[gRegionTop]
	js	L_2A73
	mov	word ptr cs:[gShapeRows], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gShapeY], ax
L_2A59:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gShapeY]
	js	L_2A73
	je	L_2A73
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_2A76
	mov	word ptr cs:[gShapeRows], ax
	jmp	short L_2A76
L_2A73:
	jmp	L_29F3
L_2A76:
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr cs:[gShapeX]
	mov	word ptr cs:[gOddX], ax
	and	word ptr cs:[gOddX], 1
	or	ax, ax
	jns	L_2AA8
	add	word ptr cs:[gShapeWidth], ax
	neg	ax
	cmp	ax, word ptr cs:[gShapeFullWidth]
L_2A9B:
	jae	L_2A73
	xor	ax, ax
	mov	word ptr cs:[gShapeX], ax
	inc	word ptr cs:[gSkipLeft]
L_2AA8:
	mov	word ptr cs:[gSkipRight], 0
	mov	ax, word ptr cs:[gRegionCols]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr cs:[gShapeX]
	js	L_2A73
	je	L_2A73
	cmp	ax, word ptr cs:[gShapeWidth]
	jae	L_2AE1
	mov	dx, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gShapeWidth], ax
	shr	dx, 1
	shr	ax, 1
	add	si, dx
	sub	si, ax
	inc	word ptr cs:[gSkipRight]
	jmp	short L_2AE1
L_2AE1:
	mov	bx, word ptr cs:[gShapeY]
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	ax, word ptr cs:[gShapeX]
	add	di, ax
	mov	bx, offset gRemapTable
	mov	ax, SCREEN_WIDTH
	sub	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gShapeY], ax
	shr	word ptr cs:[gShapeWidth], 1
	shr	word ptr cs:[gShapeFullWidth], 1
	dec	si
	mov	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gRowBytes], ax
	mov	word ptr cs:[gSrcRowStart], ax
	cmp	word ptr cs:[gSkipLeft], 0
	je	L_2B49
	test	word ptr cs:[gOddX], 1
	je	L_2B49
	dec	word ptr cs:[gShapeFullWidth]
	add	word ptr cs:[gShapeY], 2
	inc	word ptr cs:[gSrcRowStart]
	cmp	word ptr cs:[gSkipRight], 1
	jne	L_2B60
	sub	word ptr cs:[gShapeY], 2
L_2B49:
	cmp	word ptr cs:[gSkipRight], 0
	je	L_2B60
	test	word ptr cs:[gOddX], 1
	je	L_2B60
	inc	word ptr cs:[gSrcRowStart]
	dec	si
L_2B60:
	mov	ax, word ptr cs:[gShapeFullWidth]
	mov	word ptr cs:[gSrcRowSkip], ax
	mov	ax, word ptr cs:[gShapeY]
	mov	word ptr cs:[gDstRowSkip], ax
	mov	ax, word ptr cs:[gShapeRows]
	mov	word ptr cs:[gRowsLeft], ax
	mov	dx, word ptr cs:[gShapeSeg]
	mov	ax, word ptr cs:[gDstSeg]
	mov	es, ax
	mov	ds, dx
	xor	dx, dx
L_2B87:
	mov	cx, word ptr cs:[gRowBytes]
	add	si, word ptr cs:[gSrcRowStart]
	cmp	word ptr cs:[gSkipLeft], 0
	je	L_2BB4
	cmp	word ptr cs:[gOddX], 0
	je	L_2BB4
	jcxz	L_2BB1
	mov	dl, byte ptr [si]
	dec	si
	xchg	bx, dx
	mov	al, byte ptr cs:[bx + gNibbleSwap]
	xchg	bx, dx
	jmp	short L_2BD7
L_2BB1:
	jmp	L_29F3
L_2BB4:
	jcxz	L_2BF7
L_2BB6:
	mov	al, byte ptr [si]
	dec	si
	mov	dl, al
	xchg	bx, dx
	mov	al, byte ptr cs:[bx + gNibbleSwap]
	xchg	bx, dx
	mov	ah, al
	shr	al, 1
	shr	al, 1
	shr	al, 1
	shr	al, 1
	xlat	byte ptr cs:[bx]
	or	al, al
	je	L_2BE4
	stosb
	mov	al, ah
L_2BD7:
	and	al, 0Fh
	xlat	byte ptr cs:[bx]
	or	al, al
	je	L_2BF4
	stosb
	loop	L_2BB6
	jmp	short L_2BF7
L_2BE4:
	inc	di
	mov	al, ah
	and	al, 0Fh
	xlat	byte ptr cs:[bx]
	or	al, al
	je	L_2BF4
	stosb
	loop	L_2BB6
	jmp	short L_2BF7
L_2BF4:
	inc	di
	loop	L_2BB6
L_2BF7:
	cmp	word ptr cs:[gSkipRight], 0
	je	L_2C21
	cmp	word ptr cs:[gOddX], 0
	je	L_2C21
	mov	dl, byte ptr [si]
	dec	si
	xchg	bx, dx
	mov	al, byte ptr cs:[bx + gNibbleSwap]
	xchg	bx, dx
	mov	cl, 4
	shr	al, cl
	xlat	byte ptr cs:[bx]
	or	al, al
	je	L_2C20
	mov	byte ptr es:[di], al
L_2C20:
	inc	di
L_2C21:
	add	si, word ptr cs:[gSrcRowSkip]
	add	di, word ptr cs:[gDstRowSkip]
	dec	word ptr cs:[gRowsLeft]
	je	L_2C35
	jmp	L_2B87
L_2C35:
	jmp	L_29F3
DrawShapeFlippedRemapped	ENDP

DriverEnd	LABEL	BYTE		; 2C38: the end of the image, DriverEntry's size
DRIVER	ENDS
	END
