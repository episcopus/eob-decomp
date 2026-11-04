; TGA.OVL: the graphics driver for the Tandy choice, 320 by 200 pixels in 16 colours, four bits a pixel, two
; pixels a byte (the left one in the high nibble), 160 bytes a line, the lines in four banks 2000h apart: line y
; is row y SHR 2 of bank y AND 3 (gRowOffsets). It is the second of the game's driver names (gOverlayFiles,
; ds:3074: cga.ovl, tga.ovl, ega.ovl, mcga.ovl, vga.ovl), as Tandy is graphics choice 1 [H]. The game loads and
; links it as it does MCGA.OVL (whitepaper 2.5): the same entry jump, the same slot table of seven imports and
; forty exports (cs:0003), the same calls. The pages the game gives the driver are in this screen format (a page
; spans 3FA0h words: ClearPage, SwapPageWithScreen); the art pages CopyPage and GetSpriteCell read hold one
; 16-colour pixel a byte, as on the other drivers. The step from a line to the next is BANK, and from the last
; bank back to the first, BANK - BANK_WRAP: the code adds BANK and, when the offset passes 8000h (the sign
; flag), subtracts BANK_WRAP.
;
; The code is CGA.OVL's for this format, at other offsets, and keeps its stubs: DrawIndexedTile, SetTileSet,
; DrawMaskedCell16, DrawShapeMasked and its mirror, and SetTileFormat return at once (push bp, mov bp,sp, pop bp,
; retf), each followed by the body MCGA.OVL has, as dead code (QUIRK); CopyToPlanar does nothing. SetPalette is
; real here (the video array's 16 palette registers). CGA.OVL's two-bit pixel masks stay in the image, unread
; (cs:05BE). A cell (GetSpriteCell, exports 15 and 16) is the art's pixels packed two a byte after a 4-byte
; header; colour 0 is transparent: the draws keep the screen's nibble wherever the cell's is 0 (gKeepMasks).
; Routines are far (retf) unless their comment says near. TASM 2.0 and TLINK 3.0 build the whole file from this
; source (make rebuild PROGRAM=TGA.OVL).

SLOT_COUNT	equ	47		; slots in the table at cs:0003: 7 imports and 40 exports
SCREEN_WIDTH	equ	320		; pixels in a line, and bytes in a line of an art page
SCREEN_HEIGHT	equ	200
SCREEN_COLS	equ	40		; 8-pixel columns in a line
LINE_BYTES	equ	160		; bytes in a line
BANK		equ	2000h		; offset between the four banks of lines
BANK_WRAP	equ	4 * BANK - LINE_BYTES	; subtracted after BANK to go from bank 3 to the next line in bank 0
PAGE_WORDS	equ	(3 * BANK + SCREEN_HEIGHT / 4 * LINE_BYTES) / 2	; words from a page's start to the end of its last bank
FONT_HEIGHT	equ	102h		; the font's byte offsets: its height in lines
FONT_WIDTH	equ	103h		; and its width in pixels
MASK_PLANE	equ	8000		; bytes in one bit plane of page 4 (the dead masked draws)
VIDEO_ARRAY	equ	3DAh		; the video array's address port (a read resets it)
VIDEO_DATA	equ	3DEh		; and its data port
PALETTE_REGS	equ	10h		; the video array's first palette register

DRIVER	SEGMENT BYTE PUBLIC 'CODE'
	ASSUME	cs:DRIVER, ds:NOTHING, es:NOTHING, ss:NOTHING

; TGA.OVL 0000: the image starts with a jump to DriverEntry; the game calls segment:0000 far after loading
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
	dd	gNibbleSwap		; export 38, slot 45, ds:80DE
	dd	gRemapTable		; export 39, slot 46, ds:80E2 gRemapTable
gSlotCount	dw	SLOT_COUNT		; the slots LinkDriver walks

; cs:00C1: the region and the shape draws' state (as CGA.OVL's, one byte further on)
gSavedLeft	dw	0		; SaveDrawState keeps the region here (in columns and lines)
gSavedRight	dw	0
gSavedTop	dw	0
gSavedBottom	dw	0
gFillPattern	db	0		; FillRect's colour, in both pixels of a byte
	db	0F6h		; 00CA: not used
gRegionLeft	dw	0		; the drawing region SetRegion sets: left and right in 8-pixel columns
gRegionRight	dw	0
gRegionTop	dw	0		; top and bottom in lines
gRegionBottom	dw	0
gShapeSeg	dw	0		; the shape draws' state: the shape's segment
gShapeX	dw	0		; x in bytes from the region's left edge
gShapeY	dw	0		; y; then the screen bytes between two rows
gShapeRows	dw	0
gShapeWidth	dw	0
gShapeFullWidth	dw	0
gRegionCols	dw	0
gSkipLeft	dw	0		; 1 when the region's left edge cuts the shape
gSkipRight	dw	0		; 1 when the right edge cuts it
gVisibleWidth	dw	0
gVisibleBytes	dw	0		; the visible bytes of a row
gShapeRowBytes	dw	0		; the shape's bytes in a row
	db	6 dup (0)		; 00EB: not used
gDstSeg	dw	0		; DrawShapeFlippedRemapped: the page's segment
gOddX	db	0		; the low bit of x, read only by the dead code at 18FE
gShapeHeader3	dw	0		; DrawShapeFlipped: the header's fourth byte, read by nothing
gShapeLine	dw	0		; DrawShapeFlipped: the line (the dead loop flips its parity)
	db	2 dup (0)		; 00F8: not used
gRemapSeg	dw	0		; the remapped draws' state, as the shape draws': the shape's segment
gRemapX	dw	0
gRemapY	dw	0
gRemapRows	dw	0
gRemapVisibleBytes	dw	0
gRemapRowBytes	dw	0
gRemapUnused	dw	0		; read by DrawShapeFlippedRemapped, written by nothing
gRemapRegionCols	dw	0

; cs:010A (export 39, the game's gRemapTable): each byte of two pixels with both remapped, built by
; SetRemapTable; the remapped draws translate the shape's bytes through it
gRemapTable	db	256 dup (0)

; cs:020A (export 38): each byte with its two nibbles swapped, its two pixels in the other order, for the
; mirrored draws
gNibbleSwap LABEL BYTE
NibbleByte = 0
	REPT	256
	db	((NibbleByte AND 0Fh) SHL 4) OR (NibbleByte SHR 4)
NibbleByte = NibbleByte + 1
	ENDM

; cs:030A: for each byte of two pixels, the bits of the screen byte to keep under it: the nibble of each pixel
; that is 0 (transparent)
gKeepMasks LABEL BYTE
PixelPair = 0
	REPT	256
	db	((((PixelPair AND 0F0h) EQ 0) AND 0F0h) OR (((PixelPair AND 0Fh) EQ 0) AND 0Fh))
PixelPair = PixelPair + 1
	ENDM

; cs:040A: the dead masked draws' pages, the sprite cutter, SetLineClip's rectangle
gMaskPlaneSeg	dw	0		; DrawShapeMasked (dead): page 4's segment
gBackPageSeg	dw	0		; and page 2's
gMaskBit	db	0
gMaskLevel	dw	0
	db	2 dup (0)		; 0411: not used
gCellBytes	dw	0		; the cell's pixel bytes
	db	7 dup (0)		; 0415: not used
gLineClipLeft	dw	0		; the line clip rectangle, in pixels (SetLineClip)
gLineClipTop	dw	0
gLineClipRight	dw	13Fh
gLineClipBottom	dw	0C7h
gCellSeg	dw	0		; the allocated cell: segment
gCellOff	dw	0		; and offset
	db	4 dup (0)		; 0428: not used
gCellRowStart	dw	0		; the start of the source row

; cs:042E (export 34, the game's gRowOffsetTableOff): the offset of each screen line, the four lines of a
; row in the four banks
gRowOffsets LABEL WORD
RowOffset = 0
	REPT	SCREEN_HEIGHT / 4
	dw	RowOffset, BANK + RowOffset, 2 * BANK + RowOffset, 3 * BANK + RowOffset
RowOffset = RowOffset + LINE_BYTES
	ENDM

; cs:05BE: CGA.OVL's masks for its two-bit pixels (cs:06B1 there), which nothing in this driver reads
gPixelBits	db	0C0h, 30h, 0Ch, 3
gPixelKeep	db	3Fh, 0CFh, 0F3h, 0FCh
gFillLeftKeep	db	0, 0C0h, 0F0h, 0FCh
	db	3Fh, 0Fh, 3, 0, 0Fh, 0CFh, 0C3h, 0C0h
	db	3, 0C3h, 0F3h, 0F0h, 0, 0C0h, 0F0h, 0FCh
gFillRightKeep	db	3Fh, 0Fh, 3, 0
gPixelShifts	db	6, 4, 2, 0

; cs:05E2: the tile draws' state (only the dead DrawIndexedTile, SetTileSet and SetTileFormat use it)
gTileColoursPtr	dd	0
gTileMapPtr	dd	0
gTileDstSkip	dw	0
gTileRows	dw	0
gTileRowBytes	dw	0
gTileHeightCells	dw	0
gTileParas	dw	0
gTilePixelsPtr	dd	0
gTileWidthCells	dw	0
gTileMode	dw	0
gTileFormatArg3	dw	0
gTileFormatArg4	dw	0
gTileReqWidth	dw	0
gTileReqHeight	dw	0
gTileColourRow	db	16 dup (0)

; TGA.OVL 0614 (export 0, gCopyPage): CopyPage(srcSeg, dstSeg) turns a 64000-byte art page (one 16-colour pixel
; a byte) into a screen page, two pixels a byte, line by line through the four banks. The CGA colour map the game
; passes after the two segments is not read.
CopyPage	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	ds
	push	es
	mov	ds, word ptr [bp + 6]
	mov	es, word ptr [bp + 8]
	xor	si, si
	mov	di, si
	mov	bx, 0F0Fh
	mov	dx, SCREEN_HEIGHT
	mov	cl, 4
L_0631:
	mov	bp, LINE_BYTES
L_0634:
	lodsw
	and	ax, bx
	shl	al, cl
	or	al, ah
	stosb
	dec	bp
	jne	L_0634
	dec	dx
	je	L_064E
	add	di, BANK - LINE_BYTES
	jns	L_0631
	sub	di, BANK_WRAP
	jmp	short L_0631
L_064E:
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
CopyPage	ENDP

gFillStartOdd	db	0		; FillRect: 1 when the span starts on a byte's right pixel, kept inside the code
gFillEndOdd	db	0		; and 1 when it ends on one (the retf above ends CopyPage)

; TGA.OVL 065A (export 1, gRestoreRectToPage): RestoreRect on the page gDrawPage names instead of page 0
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
	pop	bp		; QUIRK 0678: dead code, the jmp above skips it
	retf
RestoreRectToPage	ENDP

; TGA.OVL 067A (export 2, gRestoreRect): RestoreRect(col, y, cols, rows, void far *buf) copies a buffer of
; screen bytes (cols * 4 a row) back to page 0, row by row through the banks
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
	jge	L_0699
	xor	di, di
L_0699:
	cmp	di, SCREEN_COLS
	jl	L_06A1
	mov	di, SCREEN_COLS - 1
L_06A1:
	shl	di, 1
	shl	di, 1
	mov	dx, word ptr [bp + 8]
	cmp	dx, 0
	jge	L_06AF
	xor	dx, dx
L_06AF:
	cmp	dx, SCREEN_HEIGHT
	jl	L_06B8
	mov	dx, SCREEN_HEIGHT - 1
L_06B8:
	mov	bx, word ptr [bp + 0Ah]
	mov	cx, word ptr [bp + 0Ch]
	cmp	bx, SCREEN_COLS
	jbe	L_06C6
	mov	bx, SCREEN_COLS
L_06C6:
	or	bx, bx
	je	L_0711
	js	L_0711
	cmp	cx, SCREEN_HEIGHT
	jbe	L_06D5
	mov	cx, SCREEN_HEIGHT
L_06D5:
	or	cx, cx
	je	L_0711
	push	bx
	mov	bx, dx
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	di, ax
	pop	bx
	mov	dx, cx
	mov	ax, word ptr [bp - 2]
	call	PageSeg
	mov	es, ax
	mov	si, word ptr [bp + 0Eh]
	mov	ds, word ptr [bp + 10h]
	shl	bx, 1
	push	bp
	mov	bp, di
L_06FB:
	mov	cx, bx
	mov	di, bp
	rep movsw
	dec	dx
	je	L_0710
	add	bp, BANK
	jns	L_06FB
	sub	bp, BANK_WRAP
	jmp	short L_06FB
L_0710:
	pop	bp
L_0711:
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

; TGA.OVL 071D (export 3, gCopyRegion): CopyRegion(srcCol, srcY, dstCol, dstY, cols, rows, srcPage,
; dstPage), as MCGA.OVL's, four bytes to a column
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
	mov	ax, word ptr [bp + 6]
	cmp	ax, SCREEN_COLS
	jge	L_0787
	or	ax, ax
	jns	L_0737
	xor	ax, ax
L_0737:
	shl	ax, 1
	shl	ax, 1
	mov	si, ax
	mov	ax, word ptr [bp + 8]
	cmp	ax, SCREEN_HEIGHT
	jge	L_0787
	or	ax, ax
	jns	L_074B
	xor	ax, ax
L_074B:
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	si, ax
	mov	ax, word ptr [bp + 0Ah]
	cmp	ax, SCREEN_COLS
	jge	L_0787
	or	ax, ax
	jns	L_0764
	xor	ax, ax
L_0764:
	shl	ax, 1
	shl	ax, 1
	mov	di, ax
	mov	ax, word ptr [bp + 0Ch]
	mov	cx, ax
	cmp	ax, SCREEN_HEIGHT
	jge	L_07E8
	or	ax, ax
	jns	L_077A
	xor	ax, ax
L_077A:
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	di, ax
	jmp	short L_0789
L_0787:
	jmp	short L_07E8
L_0789:
	mov	ax, word ptr [bp + 12h]
	call	PageSeg
	mov	ds, ax
	mov	ax, word ptr [bp + 14h]
	call	PageSeg
	mov	es, ax
	mov	dx, word ptr [bp + 0Eh]
	cmp	dx, SCREEN_COLS
	ja	L_07E8
	cmp	dx, 1
	jl	L_07E8
	shl	dx, 1
	mov	ax, word ptr [bp + 10h]
	add	cx, ax
	cmp	cx, SCREEN_HEIGHT
	jbe	L_07B9
	sub	cx, SCREEN_HEIGHT - 1
	sub	ax, cx
L_07B9:
	or	ax, ax
	js	L_07E8
	mov	bx, di
	push	bp
	mov	bp, si
L_07C2:
	mov	si, bp
	mov	di, bx
	mov	cx, dx
	rep movsw
	add	bp, BANK
	js	L_07DC
L_07D0:
	add	bx, BANK
	js	L_07E2
L_07D6:
	dec	ax
	jne	L_07C2
	pop	bp
	jmp	short L_07E8
L_07DC:
	sub	bp, BANK_WRAP
	jmp	short L_07D0
L_07E2:
	sub	bx, BANK_WRAP
	jmp	short L_07D6
L_07E8:
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

; TGA.OVL 07F2 (export 4, gClearPage): fills the draw page's PAGE_WORDS words with colour 0
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

; TGA.OVL 080E (export 5, gDrawCell8x8): DrawCell8x8(void far *src, col, y) copies 8 rows of 4 bytes from src
; to the draw page at column col, line y (0 to 192)
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
	jge	L_082C
	xor	ax, ax
L_082C:
	cmp	ax, SCREEN_HEIGHT - 7
	jl	L_0834
	mov	ax, SCREEN_HEIGHT - 8
L_0834:
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	mov	dx, word ptr [bp + 0Ah]
	cmp	dx, 0
	jge	L_0847
	xor	dx, dx
L_0847:
	cmp	dx, SCREEN_COLS
	jl	L_084F
	mov	dx, SCREEN_COLS - 1
L_084F:
	shl	dx, 1
	shl	dx, 1
	add	ax, dx
	mov	di, ax
	call	DrawPageSeg
	mov	es, ax
	mov	dx, 8
L_085F:
	movsw
	movsw
	dec	dx
	je	L_0870
	add	di, BANK - 4
	jns	L_085F
	sub	di, BANK_WRAP
	jmp	short L_085F
L_0870:
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

; TGA.OVL 087A (export 6, gDrawIndexedTile): returns at once; the dead body after it is MCGA.OVL's DrawIndexedTile
DrawIndexedTile	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
	push	ax		; QUIRK 087F: dead code, DrawIndexedTile returns above; its body as MCGA.OVL has it
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
L_08B3:
	mov	al, byte ptr es:[bx + di]
	mov	byte ptr cs:[bx + gTileColourRow], al
	dec	bx
	jge	L_08B3
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
	dw	L_08EB, L_08EB, L_08F3
L_08EB:
	mov	ax, word ptr [bp + 6]
	call	DrawTilePixels
	jmp	short L_093D
L_08F3:
	jmp	short L_093D
DrawIndexedTile	ENDP

; TGA.OVL 08F5 (near, dead): DrawIndexedTile's pixel loop
DrawTilePixels	PROC	NEAR
	mov	dx, word ptr cs:[gTileParas]
	mul	dx
	mov	si, word ptr cs:[gTilePixelsPtr]
	add	ax, word ptr cs:[gTilePixelsPtr+2]
	mov	ds, ax
	mov	dx, word ptr cs:[gTileRows]
	xor	bh, bh
L_090F:
	mov	cx, word ptr cs:[gTileRowBytes]
L_0914:
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
	loop	L_0914
	add	di, word ptr cs:[gTileDstSkip]
	dec	dx
	jne	L_090F
	ret
L_093D:
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

; TGA.OVL 0948 (export 7, gDrawLine): DrawLine(x1, y1, x2, y2, colour), clipped to SetLineClip's rectangle as
; in MCGA.OVL
DrawLine	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
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
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
DrawLine	ENDP

; TGA.OVL 0972 (near): the clipped line (ax = x1, bx = y1, cx = x2, dx = y2, colour di), as CGA.OVL 0ADD;
; the line itself runs in one of four loops (gLineSteps) by its direction, a byte written for each pixel pair
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
	jl	L_09BA
	cmp	ax, word ptr cs:[gLineClipRight]
	jg	L_09BA
	cmp	bx, word ptr cs:[gLineClipTop]
	jl	L_09BA
	cmp	bx, word ptr cs:[gLineClipBottom]
	jg	L_09BA
	cmp	cx, word ptr cs:[gLineClipLeft]
	jl	L_09BA
	cmp	cx, word ptr cs:[gLineClipRight]
	jg	L_09BA
	cmp	dx, word ptr cs:[gLineClipTop]
	jl	L_09BA
	cmp	dx, word ptr cs:[gLineClipBottom]
	jle	L_09E4
L_09BA:
	call	LineOutcode
	xchg	cx, ax
	xchg	bx, dx
	mov	di, si
	call	LineOutcode
	mov	bp, di
	or	bp, si
	je	L_09E4
	test	di, si
	jne	L_09E7
	shl	si, 1
	call	word ptr cs:[si + gClipByOutcode]
	jb	L_09BA
	xchg	cx, ax
	xchg	bx, dx
	shl	di, 1
	call	word ptr cs:[di + gClipByOutcode]
	jmp	short L_09BA
L_09E4:
	jmp	DrawLineInside
L_09E7:
	jmp	L_0C5F
gClipByOutcode	LABEL	WORD
	dw	ClipKeep, ClipToTop, ClipToBottom, ClipKeep
	dw	ClipToLeft, ClipToLeft, ClipToBottom, ClipKeep
	dw	ClipToRight, ClipToTop, ClipToRight, ClipKeep
	dw	ClipKeep, ClipKeep, ClipKeep, ClipKeep
DrawClippedLine	ENDP

; TGA.OVL 0A0A (near): outcode 0 and impossible codes: nothing to move, carry clear
ClipKeep	PROC	NEAR
	clc
	ret
ClipKeep	ENDP

; TGA.OVL 0A0C (near): moves the end (ax, bx) to the clip top along the line, carry set
ClipToTop	PROC	NEAR
	mov	si, word ptr cs:[gLineClipTop]
	call	MoveToLineY
	stc
	ret
ClipToTop	ENDP

; TGA.OVL 0A16 (near): moves the end to the clip bottom (by mirroring y and using MoveToLineY)
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

; TGA.OVL 0A2A (near): the x where the line from (ax, bx) to (cx, dx) meets y = si; returns ax = that x, bx = si
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

; TGA.OVL 0A43 (near): moves the end to the clip left edge, carry set
ClipToLeft	PROC	NEAR
	mov	si, word ptr cs:[gLineClipLeft]
	call	MoveToLineX
	stc
	ret
ClipToLeft	ENDP

; TGA.OVL 0A4D (near): moves the end to the clip right edge (by mirroring x and using MoveToLineX)
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

; TGA.OVL 0A61 (near): the y where the line meets x = si; returns bx = that y, ax = si
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

; TGA.OVL 0A76 (near): the outcode of the point (ax, bx) in si: 1 above the clip rectangle, 2 below, 4 left of
; it, 8 right of it; DrawLineInside (0AA1) follows, the line within the rectangle: the colour in both nibbles, the
; pixel's nibble by gLinePixelMasks
LineOutcode	PROC	NEAR
	xor	si, si
	cmp	bx, word ptr cs:[gLineClipTop]
	jge	L_0A82
	or	si, 1
L_0A82:
	cmp	bx, word ptr cs:[gLineClipBottom]
	jle	L_0A8C
	or	si, 2
L_0A8C:
	cmp	ax, word ptr cs:[gLineClipLeft]
	jge	L_0A96
	or	si, 4
L_0A96:
	cmp	ax, word ptr cs:[gLineClipRight]
	jle	L_0AA0
	or	si, 8
L_0AA0:
	ret
DrawLineInside:
	cmp	dx, bx
	jge	L_0AA8
	xchg	dx, bx
	xchg	cx, ax
L_0AA8:
	xor	si, si
	sub	dx, bx
	sub	cx, ax
	jns	L_0AB5
	neg	cx
	or	si, 1
L_0AB5:
	cmp	cx, dx
	jge	L_0ABE
	xchg	cx, dx
	or	si, 2
L_0ABE:
	mov	word ptr cs:[gLineMajor], cx
	shr	cx, 1
	mov	bp, cx
	mov	word ptr cs:[gLineMinor], dx
	xchg	bx, ax
	mov	word ptr cs:[gLineY], ax
	and	ax, 0FCh
	mov	dx, SCREEN_COLS
	mul	dl
	mov	di, bx
	shr	di, 1
	add	di, ax
	mov	ax, word ptr cs:[gLineY]
	and	ax, 3
	je	L_0AEF
L_0AE8:
	add	di, BANK
	dec	ax
	jne	L_0AE8
L_0AEF:
	and	bx, 1
	mov	bl, byte ptr cs:[bx + gLinePixelMasks]
	mov	cx, word ptr cs:[gLineMajor]
	inc	cx
	mov	ax, word ptr cs:[gLineColour]
	xor	ah, ah
	and	al, 0Fh
	mov	bh, al
	shl	al, 1
	shl	al, 1
	shl	al, 1
	shl	al, 1
	or	bh, al
	xor	al, al
	shl	si, 1
	jmp	word ptr cs:[si + gLineSteps]
L_0B1A:
	mov	si, word ptr cs:[gLineMinor]
	mov	ah, 0FFh
L_0B21:
	or	al, bl
	sub	bp, si
	jns	L_0B46
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	xor	al, al
	add	di, BANK - 1
	jns	L_0B41
	sub	di, BANK_WRAP
L_0B41:
	add	bp, word ptr cs:[gLineMajor]
L_0B46:
	xor	bl, ah
	js	L_0B5D
	loop	L_0B21
L_0B4C:
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	jmp	L_0C5F
L_0B5D:
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	xor	al, al
	loop	L_0B21
	jmp	short L_0B4C
L_0B71:
	mov	si, word ptr cs:[gLineMinor]
	mov	ah, 0FFh
L_0B78:
	or	al, bl
	sub	bp, si
	jns	L_0B9D
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	xor	al, al
	add	di, BANK - 1
	jns	L_0B98
	sub	di, BANK_WRAP
L_0B98:
	add	bp, word ptr cs:[gLineMajor]
L_0B9D:
	xor	bl, ah
	jns	L_0BB4
	loop	L_0B78
L_0BA3:
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	jmp	L_0C5F
L_0BB4:
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	xor	al, al
	sub	di, 2
	loop	L_0B78
	jmp	short L_0BA3
L_0BCB:
	mov	si, word ptr cs:[gLineMinor]
	mov	al, bl
	mov	ah, 0FFh
L_0BD4:
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	add	di, BANK - 1
	jns	L_0BEC
	sub	di, BANK_WRAP
L_0BEC:
	sub	bp, si
	jns	L_0BF9
	xor	bl, ah
	js	L_0BFF
	add	bp, word ptr cs:[gLineMajor]
L_0BF9:
	mov	al, bl
	loop	L_0BD4
	jmp	short L_0C5F
L_0BFF:
	inc	di
	add	bp, word ptr cs:[gLineMajor]
	mov	al, bl
	loop	L_0BD4
	jmp	short L_0C5F
L_0C0B:
	mov	si, word ptr cs:[gLineMinor]
	mov	ah, 0FFh
	mov	al, bl
L_0C14:
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	add	di, BANK - 1
	jns	L_0C2C
	sub	di, BANK_WRAP
L_0C2C:
	sub	bp, si
	jns	L_0C39
	xor	bl, ah
	jns	L_0C3F
	add	bp, word ptr cs:[gLineMajor]
L_0C39:
	mov	al, bl
	loop	L_0C14
	jmp	short L_0C5F
L_0C3F:
	dec	di
	add	bp, word ptr cs:[gLineMajor]
	mov	al, bl
	loop	L_0C14
	jmp	short L_0C5F
LineOutcode	ENDP

; cs:0C4B: DrawLine's state, kept inside the code (the jmp above skips it)
	db	2 dup (0)		; 0C4B: not used
gLineMajor	dw	0		; the larger of the two distances: pixels in the line, less one
gLineMinor	dw	0		; the smaller
gLineY	dw	0		; the first point's y
gLinePixelMasks	db	0F0h, 0Fh		; the nibble of a pixel, by x AND 1

gLineSteps	LABEL	WORD
	dw	L_0B1A, L_0B71, L_0BCB, L_0C0B
gLineColour	dw	0		; DrawLine's colour

L_0C5F:
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
; TGA.OVL 0C69 (export 9, gFillRect): FillRect(x1, y1, x2, y2, colour) fills the rectangle on the draw page
; with colour AND 0Fh (gFillPattern, the colour in both nibbles), keeping the other pixel of its first and last
; bytes when the span starts or ends inside one
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
	and	al, 0Fh
	mov	ah, al
	shl	al, 1
	shl	al, 1
	shl	al, 1
	shl	al, 1
	or	al, ah
	mov	byte ptr cs:[gFillPattern], al
	call	DrawPageSeg
	mov	es, ax
	mov	ax, word ptr [bp + 6]
	cmp	ax, SCREEN_WIDTH
	jge	L_0CD5
	or	ax, ax
	jns	L_0C9C
	xor	ax, ax
L_0C9C:
	mov	bx, word ptr [bp + 8]
	cmp	bx, SCREEN_HEIGHT
	jge	L_0CD5
	or	bx, bx
	jns	L_0CAB
	xor	bx, bx
L_0CAB:
	mov	cx, word ptr [bp + 0Ah]
	cmp	cx, 0
	jge	L_0CB5
	xor	cx, cx
L_0CB5:
	cmp	cx, SCREEN_WIDTH
	jge	L_0CD5
	mov	dx, word ptr [bp + 0Ch]
	cmp	dx, 0
	jge	L_0CC5
	xor	dx, dx
L_0CC5:
	cmp	dx, SCREEN_HEIGHT
	jge	L_0CD5
	cmp	ax, cx
	ja	L_0CD5
	cmp	bx, dx
	ja	L_0CD5
	jmp	short L_0CD8
L_0CD5:
	jmp	L_0D8E
L_0CD8:
	cmp	cx, SCREEN_WIDTH
	jl	L_0CE1
	mov	cx, SCREEN_WIDTH - 1
L_0CE1:
	cmp	dx, SCREEN_HEIGHT
	jl	L_0CEA
	mov	dx, SCREEN_HEIGHT - 1
L_0CEA:
	sub	dx, bx
	shl	bx, 1
	mov	di, word ptr cs:[bx + gRowOffsets]
	mov	byte ptr cs:[gFillStartOdd], al
	and	byte ptr cs:[gFillStartOdd], 1
	shr	ax, 1
	add	di, ax
	mov	byte ptr cs:[gFillEndOdd], cl
	and	byte ptr cs:[gFillEndOdd], 1
	mov	bx, cx
	shr	bx, 1
	cmp	bx, ax
	je	L_0D76
	inc	cx
	shr	cx, 1
	sub	cx, ax
	je	L_0D24
	cmp	byte ptr cs:[gFillStartOdd], 0
	je	L_0D24
	dec	cx
L_0D24:
	mov	si, di
L_0D26:
	mov	bl, byte ptr cs:[gFillStartOdd]
	or	bx, bx
	je	L_0D3E
	mov	ah, 0F0h
	and	ah, byte ptr es:[di]
	mov	al, 0Fh
	and	al, byte ptr cs:[gFillPattern]
	or	al, ah
	stosb
L_0D3E:
	or	cx, cx
	je	L_0D4C
	mov	ah, cl
	mov	al, byte ptr cs:[gFillPattern]
	rep stosb
	mov	cl, ah
L_0D4C:
	mov	bl, byte ptr cs:[gFillEndOdd]
	cmp	bl, 1
	je	L_0D65
	mov	ah, 0Fh
	and	ah, byte ptr es:[di]
	mov	al, 0F0h
	and	al, byte ptr cs:[gFillPattern]
	or	al, ah
	stosb
L_0D65:
	add	si, BANK
	jns	L_0D6F
	sub	si, BANK_WRAP
L_0D6F:
	mov	di, si
	dec	dx
	jns	L_0D26
	jmp	short L_0D8E
L_0D76:
	xor	cx, cx
	mov	bl, byte ptr cs:[gFillEndOdd]
	inc	bl
	shl	bl, 1
	add	byte ptr cs:[gFillStartOdd], bl
	mov	byte ptr cs:[gFillEndOdd], 1
	jmp	short L_0D24
L_0D8E:
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

; TGA.OVL 0D97 (export 10, ds:806E): ReadCell8x8(page, void far *dst, col, y) copies 8 rows of 4 bytes of a
; page into dst; no call of ds:806E was found in EOB.EXE or INTRO.EXE
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
	mov	si, word ptr [bp + 0Ch]
	cmp	si, 0
	jge	L_0DBA
	xor	si, si
L_0DBA:
	cmp	si, SCREEN_COLS
	jl	L_0DC2
	mov	si, SCREEN_COLS - 1
L_0DC2:
	shl	si, 1
	shl	si, 1
	mov	bx, word ptr [bp + 0Eh]
	cmp	bx, 0
	jge	L_0DD0
	xor	bx, bx
L_0DD0:
	cmp	bx, SCREEN_HEIGHT - 8
	jle	L_0DD9
	mov	bx, SCREEN_HEIGHT - 8
L_0DD9:
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	si, ax
	mov	cx, 8
L_0DE5:
	movsw
	movsw
	add	si, BANK - 4
	jns	L_0DF1
	sub	si, BANK_WRAP
L_0DF1:
	loop	L_0DE5
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

; TGA.OVL 0DFD (near): ax = the segment of the page after the draw page (gDrawPage + 1)
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

; TGA.OVL 0E20 (near): ax = the segment of the draw page, gPageSegs[gDrawPage & 1Fh]
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

; TGA.OVL 0E42 (near): ax = the segment of page ax, gPageSegs[ax & 1Fh]
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

; TGA.OVL 0E59 (export 11, gGetPixel): GetPixel(x, y) returns the colour (0 to 15) of the draw page's pixel
GetPixel	PROC	NEAR
	push	bp
	mov	bp, sp
	push	bx
	push	cx
	push	ds
	push	si
	mov	bx, word ptr [bp + 8]
	or	bx, bx
	js	L_0E9D
	cmp	bx, SCREEN_HEIGHT
	jge	L_0E9D
	shl	bx, 1
	mov	si, word ptr cs:[bx + gRowOffsets]
	mov	ax, word ptr [bp + 6]
	or	ax, ax
	js	L_0E9D
	cmp	ax, SCREEN_WIDTH
	jge	L_0E9D
	mov	bx, ax
	shr	bx, 1
	add	si, bx
	and	al, 1
	mov	cl, al
	call	DrawPageSeg
	mov	ds, ax
	lodsb
	or	cl, cl
	jne	L_0E98
	mov	cl, 4
	shr	al, cl
L_0E98:
	and	ax, 0Fh
	jmp	short L_0E9F
L_0E9D:
	xor	ax, ax
L_0E9F:
	pop	si
	pop	ds
	pop	cx
	pop	bx
	pop	bp
	retf
GetPixel	ENDP

; TGA.OVL 0EA5 (export 12, ds:8076): MirrorCell16(void far *src, void far *dst) mirrors a 16 by 16 cell (16
; rows of 8 bytes) left to right through gNibbleSwap; no call of ds:8076 was found
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
L_0EBE:
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
	jne	L_0EBE
	jmp	short L_0EF9
MirrorCell16	ENDP

; TGA.OVL 0EE6 (near): bx = the four pixels of ax in the other order
MirrorPixelWord	PROC	NEAR
	xor	bx, bx
	mov	bl, al
	mov	bl, byte ptr cs:[bx + gNibbleSwap]
	xchg	ah, bl
	mov	bl, byte ptr cs:[bx + gNibbleSwap]
	mov	bh, ah
	ret
L_0EF9:
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

; TGA.OVL 0F03: LinkDriver(void far *table), as MCGA.OVL 0BE6: exchanges the slot tables (imports in, exports
; out); carry set when an export slot of the driver is zero
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
L_0F21:
	xor	bx, bx
	lodsw
	mov	dx, ax
	lodsw
	or	bx, ax
	or	bx, dx
	je	L_0F35
	xchg	dx, ax
	stosw
	xchg	dx, ax
	stosw
	loop	L_0F21
	jmp	short L_0F4E
L_0F35:
	sub	si, 4
	mov	dx, word ptr es:[di]
	add	di, 4
	or	dx, dx
	je	L_0F51
	mov	word ptr [si], dx
	add	si, 2
	mov	word ptr [si], es
	add	si, 2
	loop	L_0F21
L_0F4E:
	clc
	jmp	short L_0F52
L_0F51:
	stc
L_0F52:
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

; TGA.OVL 0F5C: DriverEntry, the far call the game makes at segment:0000 once the driver is loaded: returns
; bx = the driver's size in paragraphs and ax = the offset of LinkDriver
DriverEntry	PROC	NEAR
	push	bp
	mov	bp, sp
	push	cx
	push	di
	push	es
	mov	ax, 0B800h		; QUIRK 0F62: es:di = B800:0000 is set up and thrown away by the pops, as in CGA.OVL
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

; TGA.OVL 0F79 (export 13, ds:807A): SetTileSet returns at once; the dead body after it is MCGA.OVL's
SetTileSet	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
	push	ax		; QUIRK 0F7E: dead code, SetTileSet returns above; its body as MCGA.OVL has it
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

; TGA.OVL 0FAC (exports 15 and 16, gGetSpriteCellRle and gGetSpriteCell): GetSpriteCell(col, y, cols, rows,
; map) cuts the rectangle at column col, line y out of the art page after the draw page into a cell from
; MemAlloc: a 4-byte header (0, rows, cols, rows), then rows of cols * 4 bytes, the art's pixels two a byte.
; Returns dx:ax the block. The colour map argument is not read.
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
	mov	ax, word ptr [bp + 0Ah]
	shl	ax, 1
	shl	ax, 1
	mov	dx, word ptr [bp + 0Ch]
	mul	dx
	mov	bx, 0
	push	bx
	xor	bx, bx
	push	bx
	mov	word ptr cs:[gCellBytes], ax
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
	call	NextPageSeg
	mov	ds, ax
	mov	cl, 3
	mov	si, word ptr [bp + 6]
	shl	si, cl
	mov	ax, word ptr [bp + 8]
	shl	ax, cl
	mov	bx, ax
	shl	ax, 1
	shl	ax, 1
	add	ax, bx
	shl	ax, cl
	add	si, ax
L_1015:
	mov	word ptr cs:[gCellRowStart], si
	mov	al, 8
	mul	dl
	add	ax, si
	mov	bx, ax
	mov	cl, 4
L_1024:
	lodsw
	and	ax, 0F0Fh
	shl	al, cl
	or	al, ah
	stosb
	cmp	si, bx
	jne	L_1024
	mov	si, word ptr cs:[gCellRowStart]
	add	si, SCREEN_WIDTH
	dec	dh
	jne	L_1015
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

; TGA.OVL 104F (export 17, ds:808A): DrawMaskedCell16 returns at once; the dead body after it is MCGA.OVL's
DrawMaskedCell16	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
	push	ax		; QUIRK 1054: dead code, DrawMaskedCell16 returns above; its body as MCGA.OVL has it
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
L_1093:
	mov	cx, 8
L_1096:
	mov	al, byte ptr [bx]
	mov	ah, 0FFh
	test	al, 0F0h
	je	L_10A1
	and	ah, 0Fh
L_10A1:
	test	al, 0Fh
	je	L_10A8
	and	ah, 0F0h
L_10A8:
	and	ah, byte ptr [si]
	or	al, ah
	mov	ah, al
	and	ax, 0FF0h
	stosw
	inc	si
	inc	bx
	loop	L_1096
	add	di, 130h
	dec	dl
	jne	L_1093
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

; TGA.OVL 10C8 (export 18, gCopyToPlanar): nothing to do in a packed-pixel mode
CopyToPlanar	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
CopyToPlanar	ENDP

; TGA.OVL 10CD (export 19, ds:8092): SwapPageWithScreen(page) exchanges a page's PAGE_WORDS words with page 0;
; no call of ds:8092 was found
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
L_10F0:
	mov	ax, word ptr es:[di]
	xchg	word ptr [si], ax
	stosw
	add	si, 2
	loop	L_10F0
	pop	si
	pop	es
	pop	ds
	pop	di
	pop	cx
	pop	ax
	pop	bp
	retf
SwapPageWithScreen	ENDP

; TGA.OVL 1103 (export 20, gDrawShapeMaskedFlipped): returns at once; the dead body after it is MCGA.OVL's
DrawShapeMaskedFlipped	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
	push	ax		; QUIRK 1108: dead code, DrawShapeMaskedFlipped returns above; its body as MCGA.OVL has it
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
	mov	ax, 6
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
	jns	L_11E2
	add	word ptr cs:[gShapeRows], ax
	jg	L_11A7
	jmp	L_125A
L_11A7:
	neg	ax
	mov	dx, ax
	mov	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipLeft], ax
	push	di
	push	es
	mov	di, si
	mov	ax, word ptr cs:[gShapeSeg]
	mov	es, ax
L_11BD:
	mov	cx, word ptr cs:[gSkipLeft]
L_11C2:
	xor	ax, ax
	repne scasb
	je	L_11CA
	jcxz	L_11D3
L_11CA:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jne	L_11C2
L_11D3:
	dec	dx
	jne	L_11BD
	mov	si, di
	pop	es
	pop	di
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gShapeY], ax
L_11E2:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gShapeY]
	js	L_125A
	je	L_125A
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_11FA
	mov	word ptr cs:[gShapeRows], ax
L_11FA:
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr cs:[gShapeX]
	or	ax, ax
	jns	L_1221
	add	word ptr cs:[gShapeWidth], ax
	neg	ax
	mov	word ptr cs:[gSkipLeft], ax
	cmp	ax, word ptr cs:[gShapeFullWidth]
	jae	L_125A
	xor	ax, ax
	mov	word ptr cs:[gShapeX], ax
L_1221:
	mov	word ptr cs:[gSkipRight], 0
	mov	ax, word ptr cs:[gRegionCols]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr cs:[gShapeX]
	js	L_125A
	je	L_125A
	cmp	ax, word ptr cs:[gShapeWidth]
	jae	L_125D
	mov	word ptr cs:[gShapeWidth], ax
	mov	ax, word ptr cs:[gShapeFullWidth]
	sub	ax, word ptr cs:[gSkipLeft]
	sub	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipRight], ax
	jmp	short L_125D
L_125A:
	jmp	L_13E5
L_125D:
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
L_128D:
	mov	cx, word ptr cs:[gSkipRight]
	jcxz	L_12B7
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_129C:
	xor	ax, ax
	repne scasb
	je	L_12A4
	jcxz	L_12AD
L_12A4:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jg	L_129C
L_12AD:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_12B7:
	add	di, cx
	add	cx, word ptr cs:[gVisibleWidth]
	jle	L_1334
L_12C0:
	lodsb
	or	al, al
	je	L_1336
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
L_12D6:
	cmp	cx, 0
	jle	L_12E0
	shl	al, 1
	dec	cx
	jmp	short L_12D6
L_12E0:
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
	jge	L_130A
	mov	al, byte ptr es:[di + MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_130A:
	cmp	word ptr cs:[gMaskLevel], 2
	jge	L_131E
	mov	al, byte ptr es:[di + 2 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_131E:
	cmp	word ptr cs:[gMaskLevel], 3
	jge	L_133D
	mov	al, byte ptr es:[di + 3 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
	jmp	short L_133D
L_1334:
	jmp	short L_13B2
L_1336:
	jmp	short L_13AB
L_1338:
	jmp	short L_12C0
L_133A:
	jmp	L_128D
L_133D:
	cmp	word ptr cs:[gMaskLevel], 4
	jge	L_1351
	mov	al, byte ptr es:[di + 4 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1351:
	cmp	word ptr cs:[gMaskLevel], 5
	jge	L_1365
	mov	al, byte ptr es:[di + 5 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1365:
	cmp	word ptr cs:[gMaskLevel], 6
	jge	L_1379
	mov	al, byte ptr es:[di + 6 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1379:
	mov	al, byte ptr es:[di + 7 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
	pop	ax
	pop	di
	cmp	bl, 0
	je	L_139B
	mov	es, word ptr cs:[gBackPageSeg]
	mov	bl, byte ptr es:[di]
	pop	es
	mov	byte ptr es:[di], bl
	dec	di
	jmp	short L_13A4
L_139B:
	pop	es
	mov	byte ptr es:[di], al
	dec	di
	jmp	short L_13A4
L_13A2:
	jmp	short L_133A
L_13A4:
	pop	bx
	pop	cx
	pop	dx
	loop	L_1338
	jmp	short L_13B2
L_13AB:
	lodsb
	sub	di, ax
	sub	cx, ax
	jg	L_1338
L_13B2:
	add	cx, word ptr cs:[gSkipLeft]
	jcxz	L_13DC
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_13C1:
	xor	ax, ax
	repne scasb
	je	L_13C9
	jcxz	L_13D2
L_13C9:
	inc	cx
	mov	al, byte ptr es:[di]
	inc	di
	sub	cx, ax
	jne	L_13C1
L_13D2:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_13DC:
	add	bp, LINE_BYTES
	mov	di, bp
	dec	dx
	jne	L_13A2
L_13E5:
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

; TGA.OVL 13EF (export 21, gDrawShapeMasked): returns at once; the dead body after it is MCGA.OVL's
DrawShapeMasked	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
	push	ax		; QUIRK 13F4: dead code, DrawShapeMasked returns above; its body as MCGA.OVL has it
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
	mov	ax, 6
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
	jns	L_14CE
	add	word ptr cs:[gShapeRows], ax
	jg	L_1493
	jmp	L_1546
L_1493:
	neg	ax
	mov	dx, ax
	mov	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipLeft], ax
	push	di
	push	es
	mov	di, si
	mov	ax, word ptr cs:[gShapeSeg]
	mov	es, ax
L_14A9:
	mov	cx, word ptr cs:[gSkipLeft]
L_14AE:
	xor	ax, ax
	repne scasb
	je	L_14B6
	jcxz	L_14BF
L_14B6:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jne	L_14AE
L_14BF:
	dec	dx
	jne	L_14A9
	mov	si, di
	pop	es
	pop	di
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gShapeY], ax
L_14CE:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gShapeY]
	js	L_1546
	je	L_1546
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_14E6
	mov	word ptr cs:[gShapeRows], ax
L_14E6:
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr cs:[gShapeX]
	or	ax, ax
	jns	L_150D
	add	word ptr cs:[gShapeWidth], ax
	neg	ax
	mov	word ptr cs:[gSkipLeft], ax
	cmp	ax, word ptr cs:[gShapeFullWidth]
	jae	L_1546
	xor	ax, ax
	mov	word ptr cs:[gShapeX], ax
L_150D:
	mov	word ptr cs:[gSkipRight], 0
	mov	ax, word ptr cs:[gRegionCols]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr cs:[gShapeX]
	js	L_1546
	je	L_1546
	cmp	ax, word ptr cs:[gShapeWidth]
	jae	L_1549
	mov	word ptr cs:[gShapeWidth], ax
	mov	ax, word ptr cs:[gShapeFullWidth]
	sub	ax, word ptr cs:[gSkipLeft]
	sub	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipRight], ax
	jmp	short L_1549
L_1546:
	jmp	L_16D1
L_1549:
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
L_1576:
	mov	cx, word ptr cs:[gSkipLeft]
	jcxz	L_15A0
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_1585:
	xor	ax, ax
	repne scasb
	je	L_158D
	jcxz	L_1596
L_158D:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jg	L_1585
L_1596:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_15A0:
	sub	di, cx
	add	cx, word ptr cs:[gVisibleWidth]
	jle	L_161F
L_15A9:
	lodsb
	or	al, al
	je	L_1621
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
L_15BF:
	cmp	cx, 0
	jle	L_15C9
	shl	al, 1
	dec	cx
	jmp	short L_15BF
L_15C9:
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
	jge	L_15F3
	mov	al, byte ptr es:[di + MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_15F3:
	cmp	word ptr cs:[gMaskLevel], 2
	jge	L_1607
	mov	al, byte ptr es:[di + 2 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1607:
	cmp	word ptr cs:[gMaskLevel], 3
	jge	L_1623
	mov	al, byte ptr es:[di + 3 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
	jmp	short L_1623
	jmp	short L_1670		; QUIRK 161D: dead code, the jmp above skips it
L_161F:
	jmp	short L_169E
L_1621:
	jmp	short L_1697
L_1623:
	cmp	word ptr cs:[gMaskLevel], 4
	jge	L_1637
	mov	al, byte ptr es:[di + 4 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1637:
	cmp	word ptr cs:[gMaskLevel], 5
	jge	L_1650
	mov	al, byte ptr es:[di + 5 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
	jmp	short L_1650
L_164D:
	jmp	L_15A9
L_1650:
	cmp	word ptr cs:[gMaskLevel], 6
	jge	L_1664
	mov	al, byte ptr es:[di + 6 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1664:
	mov	al, byte ptr es:[di + 7 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1670:
	pop	ax
	pop	di
	cmp	bl, 0
	je	L_1686
	mov	es, word ptr cs:[gBackPageSeg]
	mov	bl, byte ptr es:[di]
	pop	es
	mov	byte ptr es:[di], bl
	inc	di
	jmp	short L_168B
L_1686:
	pop	es
	mov	byte ptr es:[di], al
	inc	di
L_168B:
	pop	bx
	pop	cx
	pop	dx
	jmp	short L_1693
L_1690:
	jmp	L_1576
L_1693:
	loop	L_164D
	jmp	short L_169E
L_1697:
	lodsb
	add	di, ax
	sub	cx, ax
	jg	L_164D
L_169E:
	add	cx, word ptr cs:[gSkipRight]
	jcxz	L_16C8
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_16AD:
	xor	ax, ax
	repne scasb
	je	L_16B5
	jcxz	L_16BE
L_16B5:
	inc	cx
	mov	al, byte ptr es:[di]
	inc	di
	sub	cx, ax
	jne	L_16AD
L_16BE:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_16C8:
	add	bp, LINE_BYTES
	mov	di, bp
	dec	dx
	jne	L_1690
L_16D1:
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

; TGA.OVL 16DB (export 22, gPutPixel): PutPixel(x, y, colour) sets the draw page's pixel to colour AND 0Fh; off
; the screen it does nothing
PutPixel	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	ds
	push	di
	mov	bx, word ptr [bp + 8]
	or	bx, bx
	js	L_172B
	cmp	bx, SCREEN_HEIGHT
	jge	L_172B
	shl	bx, 1
	mov	di, word ptr cs:[bx + gRowOffsets]
	mov	ax, word ptr [bp + 6]
	or	ax, ax
	js	L_172B
	cmp	ax, SCREEN_WIDTH
	jge	L_172B
	mov	bx, ax
	and	bx, 1
	shr	ax, 1
	add	di, ax
	call	DrawPageSeg
	mov	ds, ax
	mov	ax, word ptr [bp + 0Ah]
	and	ax, 0Fh
	mov	bh, 0F0h
	or	bl, bl
	jne	L_1723
	mov	cl, 4
	shl	al, cl
	not	bh
L_1723:
	mov	ah, byte ptr [di]
	and	ah, bh
	or	ah, al
	mov	byte ptr [di], ah
L_172B:
	pop	di
	pop	ds
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
PutPixel	ENDP

; TGA.OVL 1732 (export 23, gRestoreDrawState): puts back the region SaveDrawState kept
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

; TGA.OVL 1759 (export 24, gDrawShapeFlipped): DrawShape mirrored left to right: each byte's pixels swapped
; through gNibbleSwap, drawn from the row's end
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
	cld
	mov	word ptr cs:[gSkipLeft], 0
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
	mov	di, ax
	sar	bx, 1
	sub	bx, ax
	mov	word ptr cs:[gShapeX], bx
	mov	ax, word ptr cs:[gRegionRight]
	sub	ax, word ptr cs:[gRegionLeft]
	mov	word ptr cs:[gRegionCols], ax
	mov	ax, word ptr cs:[gShapeSeg]
	mov	ds, ax
	inc	si
	lodsw
	push	ax
	xor	ah, ah
	mov	word ptr cs:[gShapeRows], ax
	pop	ax
	xchg	al, ah
	xor	ah, ah
	shl	ax, 1
	shl	ax, 1
	mov	word ptr cs:[gVisibleBytes], ax
	mov	word ptr cs:[gShapeRowBytes], ax
	lodsb
	mov	word ptr cs:[gShapeHeader3], ax
	mov	ax, word ptr cs:[gShapeY]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_180A
	neg	ax
	cmp	ax, word ptr cs:[gShapeRows]
	jb	L_17E5
L_17E2:
	jmp	L_187B
L_17E5:
	mov	dx, word ptr cs:[gVisibleBytes]
	mul	dl
	add	si, ax
	mov	ax, word ptr cs:[gShapeRows]
	add	ax, word ptr cs:[gShapeY]
	sub	ax, word ptr cs:[gRegionTop]
	js	L_17E2
	mov	word ptr cs:[gShapeRows], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gShapeY], ax
L_180A:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gShapeY]
	js	L_187B
	je	L_187B
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_1822
	mov	word ptr cs:[gShapeRows], ax
L_1822:
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr cs:[gShapeX]
	or	ax, ax
	jns	L_184A
	add	word ptr cs:[gVisibleBytes], ax
	neg	ax
	cmp	ax, word ptr cs:[gShapeRowBytes]
	jae	L_187B
	xor	ax, ax
	mov	word ptr cs:[gShapeX], ax
	inc	word ptr cs:[gSkipLeft]
L_184A:
	mov	ax, word ptr cs:[gRegionCols]
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr cs:[gShapeX]
	js	L_187B
	je	L_187B
	mov	word ptr cs:[gSkipRight], 0
	cmp	ax, word ptr cs:[gVisibleBytes]
	ja	L_187D
	add	si, word ptr cs:[gVisibleBytes]
	sub	si, ax
	mov	word ptr cs:[gVisibleBytes], ax
	inc	word ptr cs:[gSkipRight]
	jmp	short L_187D
L_187B:
	jmp	short L_18F4
L_187D:
	mov	bx, word ptr cs:[gShapeY]
	mov	word ptr cs:[gShapeLine], bx
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	ax, word ptr cs:[gShapeX]
	add	di, ax
	mov	ax, BANK
	sub	ax, word ptr cs:[gVisibleBytes]
	mov	word ptr cs:[gShapeY], ax
	dec	si
L_18A2:
	xor	bx, bx
	mov	cx, word ptr cs:[gVisibleBytes]
	add	si, cx
L_18AB:
	mov	bl, byte ptr [si]
	or	bl, bl
	je	L_18EE
L_18B1:
	mov	bl, byte ptr cs:[bx + gNibbleSwap]
	mov	al, bl
	mov	bl, byte ptr cs:[bx + gKeepMasks]
	and	bl, byte ptr es:[di]
	or	al, bl
	stosb
	dec	si
	loop	L_18AB
L_18C6:
	add	si, word ptr cs:[gShapeRowBytes]
	add	di, word ptr cs:[gShapeY]
	js	L_18DB
	dec	word ptr cs:[gShapeRows]
	jne	L_18A2
	je	L_18F4
L_18DB:
	sub	di, BANK_WRAP
	dec	word ptr cs:[gShapeRows]
	jne	L_18A2
	jmp	short L_18F4
L_18E8:
	mov	bl, byte ptr [si]
	or	bl, bl
	jne	L_18B1
L_18EE:
	inc	di
	dec	si
	loop	L_18E8
	jmp	short L_18C6
L_18F4:
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
L_18FE:
	mov	cx, word ptr cs:[gVisibleBytes]		; QUIRK 18FE: dead code after DrawShapeFlipped's retf, CGA.OVL's loop for masked cells
	add	si, cx
	add	bp, cx
	mov	ch, cl
	mov	cl, byte ptr cs:[gOddX]
	mov	bx, offset gNibbleSwap
	cmp	word ptr cs:[gSkipLeft], 0
	je	L_191C
	call	DrawMirroredEdge
L_191C:
	or	ch, ch
	je	L_1952
L_1920:
	mov	al, byte ptr [si]
	mov	ah, byte ptr ds:[bp]
	mov	bx, offset gNibbleSwap
	xlat	byte ptr cs:[bx]
	xchg	ah, al
	xlat	byte ptr cs:[bx]
	mov	bh, al
	mov	bl, 0FFh
	stc
	rcr	bx, cl
	xchg	bh, bl
	xor	al, al
	shr	ax, cl
	xchg	ah, al
	and	bx, word ptr es:[di]
	cmp	ch, 1
	je	L_197C
L_1946:
	or	ax, bx
	mov	word ptr es:[di], ax
L_194B:
	dec	si
	dec	bp
	inc	di
	dec	ch
	jne	L_1920
L_1952:
	add	si, word ptr cs:[gShapeRowBytes]
	add	bp, word ptr cs:[gShapeRowBytes]
	mov	ax, word ptr cs:[gShapeLine]
	and	ax, 1
	xor	ax, 1
	mov	word ptr cs:[gShapeLine], ax
	je	L_198B
	add	di, word ptr cs:[gShapeY]
	dec	word ptr cs:[gShapeRows]
	jne	L_18FE
L_1978:
	pop	bp
	jmp	L_18F4
L_197C:
	cmp	word ptr cs:[gSkipRight], 0
	je	L_1946
	or	al, bl
	mov	byte ptr es:[di], al
	jmp	short L_194B
L_198B:
	sub	di, word ptr cs:[gShapeX]
	dec	word ptr cs:[gShapeRows]
	je	L_1978
	jmp	L_18FE
DrawShapeFlipped	ENDP

; TGA.OVL 199A (near, dead): CGA.OVL's DrawMirroredEdge with gNibbleSwap, called only from the dead code at
; 18FE
DrawMirroredEdge	PROC	NEAR
	mov	al, byte ptr [si + 1]
	mov	ah, byte ptr ds:[bp + 1]
	mov	bx, offset gNibbleSwap
	xlat	byte ptr cs:[bx]
	xchg	ah, al
	xlat	byte ptr cs:[bx]
	mov	bh, al
	mov	bl, 0FFh
	stc
	rcr	bx, cl
	xor	al, al
	shr	ax, cl
	and	bl, byte ptr es:[di]
	or	al, bl
	mov	byte ptr es:[di], al
	ret
DrawMirroredEdge	ENDP

; TGA.OVL 19BE (export 25, gSaveDrawState): keeps the region and sets it to the whole screen (0, 40, 0, 200)
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

; TGA.OVL 1A01 (export 14, gSaveRectFromPage): SaveRect from the page gDrawPage names instead of page 0
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
	pop	bp		; QUIRK 1A1E: dead code, the jmp above skips it
	retf
SaveRectFromPage	ENDP

; TGA.OVL 1A20 (export 26, gSaveRect): SaveRect(col, y, cols, rows, void far *buf) copies a rectangle of page 0
; (cols * 4 bytes a row) into buf; returns the bytes written
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
	jge	L_1A3E
	xor	si, si
L_1A3E:
	cmp	si, SCREEN_COLS
	jl	L_1A46
	mov	si, SCREEN_COLS - 1
L_1A46:
	shl	si, 1
	shl	si, 1
	mov	dx, word ptr [bp + 8]
	cmp	dx, 0
	jge	L_1A54
	xor	dx, dx
L_1A54:
	cmp	dx, SCREEN_HEIGHT
	jl	L_1A5D
	mov	dx, SCREEN_HEIGHT - 1
L_1A5D:
	mov	bx, word ptr [bp + 0Ah]
	cmp	bx, 0
	jg	L_1A69
L_1A65:
	xor	ax, ax
	jmp	short L_1AD7
L_1A69:
	cmp	bx, SCREEN_COLS
	jle	L_1A71
	mov	bx, SCREEN_COLS
L_1A71:
	mov	cx, word ptr [bp + 0Ch]
	cmp	cx, 0
	jle	L_1A65
	cmp	cx, SCREEN_HEIGHT
	jle	L_1A82
	mov	cx, SCREEN_HEIGHT
L_1A82:
	mov	di, word ptr [bp + 0Eh]
	push	di
	mov	es, word ptr [bp + 10h]
	cmp	bx, SCREEN_COLS
	jbe	L_1A91
	mov	bx, SCREEN_COLS
L_1A91:
	or	bx, bx
	jne	L_1A97
L_1A95:
	jmp	short L_1AD2
L_1A97:
	cmp	cx, SCREEN_HEIGHT
	jbe	L_1AA0
	mov	cx, SCREEN_HEIGHT
L_1AA0:
	or	cx, cx
	je	L_1A95
	push	bx
	mov	bx, dx
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	si, ax
	pop	bx
	mov	dx, cx
	mov	ax, word ptr [bp - 2]
	call	PageSeg
	mov	ds, ax
	shl	bx, 1
L_1ABD:
	push	si
	mov	cx, bx
	rep movsw
	pop	si
	dec	dx
	je	L_1AD2
	add	si, BANK
	jns	L_1ABD
	sub	si, BANK_WRAP
	jmp	short L_1ABD
L_1AD2:
	mov	ax, di
	pop	di
	sub	ax, di
L_1AD7:
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

; TGA.OVL 1AE2 (export 27, ds:80B2): SetLineClip(left, top, right, bottom) sets the rectangle DrawLine clips
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

; TGA.OVL 1B05 (export 28, gSetRegion): SetRegion(left, right, top, bottom): the region the shape draws clip
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

; TGA.OVL 1B28 (export 29, gSetPalette): SetPalette(void far *colours) loads the video array's 16 palette
; registers (10h to 1Fh) from 16 bytes: for each, a read of the address port resets it, then the register's
; number to the address port and the byte to the data port
SetPalette	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	ds
	mov	si, word ptr [bp + 6]		; QUIRK 1B30: si is changed and not saved
	mov	ds, word ptr [bp + 8]
	mov	cx, 10h
	mov	bl, PALETTE_REGS
	mov	dh, HIGH VIDEO_ARRAY
L_1B3D:
	mov	dl, LOW VIDEO_ARRAY
	in	al, dx
	mov	al, bl
	out	dx, al
	lodsb
	mov	dl, LOW VIDEO_DATA
	out	dx, al
	inc	bl
	loop	L_1B3D
	mov	dl, LOW VIDEO_ARRAY
	in	al, dx
	mov	al, 0Fh		; out of the palette registers, which turns the picture back on [H]
	out	dx, al
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
SetPalette	ENDP

; TGA.OVL 1B58 (export 30, ds:80BE): SetTileFormat returns at once; the dead body after it is MCGA.OVL's
SetTileFormat	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
	push	ax		; QUIRK 1B5D: dead code, SetTileFormat returns above; its body as MCGA.OVL has it
	push	bx
	push	cx
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gTileFormatArg3], ax
	mov	ax, word ptr [bp + 0Ch]
	mov	word ptr cs:[gTileFormatArg4], ax
	mov	ax, word ptr [bp + 6]
	mov	bx, word ptr [bp + 8]
	cmp	ax, bx
	jne	L_1BC3
	cmp	ax, 3
	jge	L_1BC3
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
	add	cx, LINE_BYTES
	mov	word ptr cs:[gTileDstSkip], cx
	jmp	short L_1BF4
L_1BC3:
	mov	word ptr cs:[gTileMode], 4
	mov	word ptr cs:[gTileParas], 2
	mov	word ptr cs:[gTileRowBytes], 4
	mov	word ptr cs:[gTileRows], 8
	mov	word ptr cs:[gTileDstSkip], 138h
	mov	word ptr cs:[gTileReqWidth], ax
	mov	word ptr cs:[gTileReqHeight], bx
	mov	ax, 1
	mov	bx, ax
L_1BF4:
	mov	word ptr cs:[gTileWidthCells], ax
	mov	word ptr cs:[gTileHeightCells], bx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
SetTileFormat	ENDP

; TGA.OVL 1C02 (export 31, gDrawShape): DrawShape(page, void far *shape, x, y) draws a GetSpriteCell cell at
; pixel (x AND NOT 1, y), clipped to the region: each byte is the screen's AND gKeepMasks of the cell's byte, OR the
; cell's byte; a byte of two transparent pixels is skipped
DrawShape	PROC	NEAR
	push	bp
	mov	bp, sp
	sub	sp, 1Ah
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
	mov	di, ax
	sar	bx, 1
	sub	bx, ax
	mov	word ptr [bp - 10h], bx
	mov	ax, word ptr cs:[gRegionRight]
	sub	ax, word ptr cs:[gRegionLeft]
	mov	word ptr [bp - 0Eh], ax
	mov	ax, word ptr [bp - 14h]
	mov	ds, ax
	inc	si
	lodsw
	push	ax
	xor	ah, ah
	mov	word ptr [bp - 0Ch], ax
	lodsb
	mov	word ptr [bp - 2], ax
	pop	ax
	xchg	al, ah
	xor	ah, ah
	shl	ax, 1
	shl	ax, 1
	mov	word ptr [bp - 0Ah], ax
	mov	word ptr [bp - 8], ax
	mov	ax, word ptr [bp - 12h]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_1C99
	neg	ax
	cmp	ax, word ptr [bp - 0Ch]
	jb	L_1C7B
L_1C79:
	jmp	short L_1CEC
L_1C7B:
	mov	dx, word ptr [bp - 0Ah]
	mul	dl
	add	si, ax
	mov	ax, word ptr [bp - 0Ch]
	add	ax, word ptr [bp - 12h]
	sub	ax, word ptr cs:[gRegionTop]
	js	L_1C79
	mov	word ptr [bp - 0Ch], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr [bp - 12h], ax
L_1C99:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr [bp - 12h]
	js	L_1CEC
	je	L_1CEC
	cmp	ax, word ptr [bp - 0Ch]
	jae	L_1CAC
	mov	word ptr [bp - 0Ch], ax
L_1CAC:
	mov	word ptr [bp - 6], 0
	mov	ax, word ptr [bp - 10h]
	or	ax, ax
	jns	L_1CCC
	add	word ptr [bp - 0Ah], ax
	neg	ax
	add	si, ax
	cmp	ax, word ptr [bp - 8]
	jae	L_1CEC
	xor	ax, ax
	mov	word ptr [bp - 10h], ax
	inc	word ptr [bp - 6]
L_1CCC:
	mov	ax, word ptr [bp - 0Eh]
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr [bp - 10h]
	js	L_1CEC
	je	L_1CEC
	mov	word ptr [bp - 4], 0
	cmp	ax, word ptr [bp - 0Ah]
	jae	L_1CEE
	mov	word ptr [bp - 0Ah], ax
	inc	word ptr [bp - 4]
	jmp	short L_1CEE
L_1CEC:
	jmp	short L_1D46
L_1CEE:
	mov	ax, word ptr [bp - 12h]
	xchg	bx, ax
	shl	bx, 1
	mov	bx, word ptr cs:[bx + gRowOffsets]
	add	bx, word ptr [bp - 10h]
	xchg	bx, ax
	add	di, ax
	mov	ax, BANK
	sub	ax, word ptr [bp - 0Ah]
	mov	word ptr [bp - 12h], ax
	mov	bx, offset gKeepMasks
L_1D0B:
	mov	cx, word ptr [bp - 0Ah]
L_1D0E:
	lodsb
	or	al, al
	je	L_1D41
L_1D13:
	mov	ah, al
	xlat	byte ptr cs:[bx]
	and	al, byte ptr es:[di]
	or	al, ah
	stosb
	loop	L_1D0E
L_1D1F:
	add	si, word ptr [bp - 8]
	sub	si, word ptr [bp - 0Ah]
	add	di, word ptr [bp - 12h]
	js	L_1D31
	dec	word ptr [bp - 0Ch]
	jne	L_1D0B
	jmp	short L_1D46
L_1D31:
	sub	di, BANK_WRAP
	dec	word ptr [bp - 0Ch]
	jne	L_1D0B
	jmp	short L_1D46
L_1D3C:
	lodsb
	or	al, al
	jne	L_1D13
L_1D41:
	inc	di
	loop	L_1D3C
	jmp	short L_1D1F
L_1D46:
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
	pop	bp		; QUIRK 1D52: dead code, the retf above ends DrawShape
	retf
DrawShape	ENDP

; TGA.OVL 1D54 (export 32, gSaveRectSize): SaveRectSize(cols, rows) returns cols * rows * 4, the bytes SaveRect
; needs (cols clamped to 1 to 40, rows to 1 to 200)
SaveRectSize	PROC	NEAR
	push	bp
	mov	bp, sp
	push	cx
	push	dx
	mov	ax, word ptr [bp + 6]
	cmp	ax, 1
	jge	L_1D64
	mov	ax, 1
L_1D64:
	cmp	ax, SCREEN_COLS
	jle	L_1D6C
	mov	ax, SCREEN_COLS
L_1D6C:
	mov	dx, word ptr [bp + 8]
	cmp	dx, 1
	jge	L_1D77
	mov	dx, 1
L_1D77:
	cmp	dx, SCREEN_HEIGHT
	jle	L_1D80
	mov	dx, SCREEN_HEIGHT
L_1D80:
	mul	dx
	shl	ax, 1
	shl	ax, 1
	pop	dx
	pop	cx
	pop	bp
	retf
SaveRectSize	ENDP

; cs:1D8A: DrawGlyph's and XorRect's state
gBlitBankSkip	dw	0		; screen bytes from a row's end to the next line in the next bank
gBlitBankWrap	dw	0		; and subtracted from the last bank to the next line in the first
	db	4 dup (0)		; 1D8E: not used
gBlitBytes	dw	0		; the glyph's bytes in a row, less one; XorRect: the pixels of the first group
gBlitWidth	db	0		; DrawGlyph: the glyph's width; XorRect: the whole groups after the first
gBlitRows	db	0
gBlitX	db	0		; x, then x AND 1
gBlitBank	db	0		; y, then the bank (y AND 3)
gBlitColour	db	0		; DrawText's colours
gBlitBack	db	0
gColourPatterns LABEL WORD		; for each of the 16 colours, a word of four pixels in it
Colour = 0
	REPT	16
	dw	Colour * 1111h
Colour = Colour + 1
	ENDM
gTextPattern	dw	0		; the colours' patterns (gColourPatterns)
gBackPattern	dw	0
gTextLeft	dw	0		; DrawText: the x a line starts at
gLineHeight	dw	0		; the font's height
gCharWidth	dw	0		; the character's width
gLeftMask	db	0		; XorRect: the masks of the first and last groups
gRightMask	db	0
gGlyphBytes	db	0		; DrawGlyph: the bytes a glyph row is drawn in, less one
gFontSeg	dw	0		; the font's segment

; cs:1DC9: for each width of 1 to 24 pixels, a set of eight bit masks, one per starting bit, MSB first:
; gBitMaskTable[w - 1] -> gMaskSet<w>, whose word p points at the mask bytes for a span of w bits
; starting at bit p (DrawGlyph's glyph rows, XorRect's first group)
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

; cs:21A1: for each four bits of a glyph row (bit 3 the leftmost), a word of four pixels, 0Fh where the bit is
; set (the low byte holds the two left pixels)
gNibblePixels LABEL WORD
GlyphBits = 0
	REPT	16
	dw	((GlyphBits SHR 3) * 0F0h) OR (((GlyphBits SHR 2) AND 1) * 0Fh) OR (((GlyphBits SHR 1) AND 1) * 0F000h) OR ((GlyphBits AND 1) * 0F00h)
GlyphBits = GlyphBits + 1
	ENDM

; TGA.OVL 21C1 (near): DrawGlyph(ch, x, y), as MCGA.OVL 241E: each four glyph bits become a word of four pixels
; through gNibblePixels, drawn in gTextPattern (set bits) and gBackPattern (clear bits), each unless its colour
; is 0
DrawGlyph	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	ds
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
	jb	L_21F4
L_21F1:
	jmp	L_23D6
L_21F4:
	mov	ax, dx
	mov	byte ptr cs:[gBlitX], al
	xor	cx, cx
	mov	cl, byte ptr es:[si + FONT_WIDTH]
	mov	byte ptr cs:[gBlitWidth], cl
	add	ax, cx
	cmp	ax, SCREEN_WIDTH
	ja	L_21F1
	mov	di, dx
	mov	dx, word ptr [bp + 8]
	cmp	dx, SCREEN_HEIGHT
	jae	L_21F1
	mov	ax, dx
	mov	byte ptr cs:[gBlitBank], al
	mov	cl, byte ptr es:[si + FONT_HEIGHT]
	mov	byte ptr cs:[gBlitRows], cl
	add	ax, cx
	cmp	ax, SCREEN_HEIGHT
	ja	L_21F1
	add	si, bx
	xor	ah, ah
	mov	al, byte ptr cs:[gBlitWidth]
	dec	al
	cmp	al, 18h
	jae	L_21F1
	mov	bp, ax
	add	al, 7
	shr	al, 1
	shr	al, 1
	shr	al, 1
	mov	byte ptr cs:[gGlyphBytes], al
	shl	bp, 1
	mov	bp, word ptr cs:[bp + gBitMaskTable]
	shr	di, 1
	and	byte ptr cs:[gBlitX], 1
	xor	ah, ah
	mov	al, byte ptr cs:[gBlitX]
	shl	ax, 1
	add	bp, ax
	mov	bp, word ptr cs:[bp]
	mov	ax, dx
	and	ax, 3FCh
	mov	dx, SCREEN_COLS
	mul	dx
	and	byte ptr cs:[gBlitBank], 3
	je	L_2286
	xor	ch, ch
	mov	cl, byte ptr cs:[gBlitBank]
L_2281:
	add	ax, BANK
	loop	L_2281
L_2286:
	add	di, ax
	xor	ah, ah
	mov	al, byte ptr cs:[gGlyphBytes]
	mov	word ptr cs:[gBlitBytes], ax
	shl	ax, 1
	inc	ax
	shl	ax, 1
	mov	bx, ax
	mov	ax, BANK
	sub	ax, bx
	mov	word ptr cs:[gBlitBankSkip], ax
	mov	ax, bx
	sub	ax, LINE_BYTES
	add	ax, 3 * BANK
	mov	word ptr cs:[gBlitBankWrap], ax
	call	DrawPageSeg
	mov	dx, es
	mov	es, ax
	mov	ds, dx
	mov	bh, byte ptr cs:[gBlitColour]
	mov	cl, 4
	shl	bh, cl
	or	bh, byte ptr cs:[gBlitBack]
	mov	cl, byte ptr cs:[gBlitX]
L_22CA:
	mov	ch, byte ptr cs:[gGlyphBytes]
	xor	ax, ax
	mov	bl, ah
L_22D3:
	push	cx
	xor	al, al
	mov	ah, byte ptr [si]
	shr	ax, cl
	xchg	al, ah
	or	al, bl
	mov	bl, ah
	mov	cl, bh
	mov	ch, al
	push	bx
	xor	ah, ah
	mov	dx, 0
	test	cl, 0F0h
	je	L_2304
	and	al, byte ptr cs:[bp]
	mov	bx, ax
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	shl	bx, 1
	mov	dx, word ptr cs:[bx + gNibblePixels]
L_2304:
	test	cl, 0Fh
	jne	L_230E
	mov	ax, 0
	jmp	short L_2325
L_230E:
	not	al
	and	al, byte ptr cs:[bp]
	mov	bx, ax
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gNibblePixels]
L_2325:
	mov	bx, ax
	or	ax, dx
	not	ax
	and	ax, word ptr es:[di]
	and	bx, word ptr cs:[gBackPattern]
	and	dx, word ptr cs:[gTextPattern]
	or	ax, bx
	or	ax, dx
	mov	word ptr es:[di], ax
	add	di, 2
	mov	al, ch
	xor	ah, ah
	mov	dx, 0
	test	cl, 0F0h
	je	L_235E
	and	al, byte ptr cs:[bp]
	mov	bx, ax
	and	bx, 0Fh
	shl	bx, 1
	mov	dx, word ptr cs:[bx + gNibblePixels]
L_235E:
	test	cl, 0Fh
	jne	L_2368
	mov	ax, 0
	jmp	short L_237A
L_2368:
	not	al
	and	al, byte ptr cs:[bp]
	mov	bx, ax
	and	bx, 0Fh
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gNibblePixels]
L_237A:
	mov	bx, ax
	or	ax, dx
	not	ax
	and	ax, word ptr es:[di]
	and	bx, word ptr cs:[gBackPattern]
	and	dx, word ptr cs:[gTextPattern]
	or	ax, bx
	or	ax, dx
	mov	word ptr es:[di], ax
	pop	bx
	pop	cx
	cmp	ch, 0
	je	L_23A5
	add	di, 2
	inc	si
	inc	bp
	dec	ch
	jmp	L_22D3
L_23A5:
	sub	bp, word ptr cs:[gBlitBytes]
	mov	al, byte ptr cs:[gBlitBank]
	inc	al
	and	al, 3
	mov	byte ptr cs:[gBlitBank], al
	je	L_23C7
	add	di, word ptr cs:[gBlitBankSkip]
	dec	byte ptr cs:[gBlitRows]
	je	L_23D6
	jmp	L_22CA
L_23C7:
	sub	di, word ptr cs:[gBlitBankWrap]
	dec	byte ptr cs:[gBlitRows]
	je	L_23D6
	jmp	L_22CA
L_23D6:
	pop	si
	pop	es
	pop	di
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	ret
DrawGlyph	ENDP

; TGA.OVL 23E0 (export 33, gDrawText): DrawText(char far *text, x, y, colour, background), as MCGA.OVL's; the
; two colours become patterns through gColourPatterns
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
	jge	L_2420
	xor	bx, bx
L_2420:
	cmp	bx, SCREEN_WIDTH
	jb	L_2429
L_2426:
	jmp	L_24D4
L_2429:
	mov	word ptr cs:[gTextLeft], bx
	mov	cx, word ptr [bp + 0Ch]
	cmp	cx, 0
	jge	L_2438
	xor	cx, cx
L_2438:
	cmp	cx, SCREEN_HEIGHT
	jge	L_2426
	mov	al, byte ptr [bp + 0Eh]
	mov	byte ptr cs:[gBlitColour], al
	mov	al, byte ptr [bp + 10h]
	mov	byte ptr cs:[gBlitBack], al
	push	di
	mov	al, byte ptr cs:[gBlitColour]
	and	ax, 0Fh
	mov	di, ax
	shl	di, 1
	mov	ax, word ptr cs:[di + gColourPatterns]
	mov	word ptr cs:[gTextPattern], ax
	mov	al, byte ptr cs:[gBlitBack]
	and	ax, 0Fh
	mov	di, ax
	shl	di, 1
	mov	ax, word ptr cs:[di + gColourPatterns]
	mov	word ptr cs:[gBackPattern], ax
	pop	di
L_2476:
	cmp	byte ptr es:[si], 0
	je	L_24D4
	cmp	byte ptr es:[si], 0Dh
	jne	L_248F
	mov	bx, word ptr cs:[gTextLeft]
	add	cx, word ptr cs:[gLineHeight]
	inc	si
	jmp	short L_2476
L_248F:
	push	es
	mov	es, word ptr cs:[gFontSeg]
	xor	ah, ah
	mov	al, byte ptr es:[di + FONT_WIDTH]
	mov	word ptr cs:[gCharWidth], ax
	mov	dx, bx
	add	dx, ax
	pop	es
	cmp	dx, SCREEN_WIDTH
	jbe	L_24B5
	mov	bx, word ptr cs:[gTextLeft]
	add	cx, word ptr cs:[gLineHeight]
L_24B5:
	cmp	cx, SCREEN_HEIGHT
	jae	L_24D4
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
	jmp	short L_2476
L_24D4:
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

; TGA.OVL 24DE (export 8, gXorRect): XorRect(x1, y1, x2, y2, colour) XORs the pattern of colour into the
; rectangle on the draw page, eight pixels (four bytes) a group, the first and last through gBitMaskTable and
; gNibblePixels
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
	jge	L_24F1
	xor	di, di
L_24F1:
	cmp	di, SCREEN_WIDTH
	jl	L_24FA
	mov	di, SCREEN_WIDTH - 1
L_24FA:
	mov	dx, word ptr [bp + 8]
	cmp	dx, 0
	jge	L_2504
	xor	dx, dx
L_2504:
	cmp	dx, SCREEN_HEIGHT
	jl	L_250D
	mov	dx, SCREEN_HEIGHT - 1
L_250D:
	mov	ax, word ptr [bp + 0Ah]
	cmp	ax, 0
	jge	L_2517
	xor	ax, ax
L_2517:
	cmp	ax, SCREEN_WIDTH
	jl	L_251F
	mov	ax, SCREEN_WIDTH - 1
L_251F:
	cmp	ax, di
	jge	L_2524
	xchg	di, ax
L_2524:
	mov	bx, di
	and	bl, 7
	mov	byte ptr cs:[gLeftMask], bl
	mov	bh, al
	and	bh, 7
	sub	ax, di
	inc	ax
	mov	cx, ax
	cmp	ax, 8
	jle	L_2547
	xor	ch, ch
	mov	cl, 8
	sub	cl, bl
	add	cl, bh
	inc	cl
L_2547:
	mov	word ptr cs:[gBlitBytes], cx
	sub	ax, cx
	shr	ax, 1
	shr	ax, 1
	shr	ax, 1
	mov	byte ptr cs:[gBlitWidth], al
	mov	ax, word ptr [bp + 0Ch]
	cmp	ax, 0
	jge	L_2562
	xor	ax, ax
L_2562:
	cmp	ax, SCREEN_HEIGHT
	jl	L_256A
	mov	ax, SCREEN_HEIGHT - 1
L_256A:
	cmp	ax, dx
	jge	L_256F
	xchg	dx, ax
L_256F:
	mov	byte ptr cs:[gBlitBank], dl
	sub	ax, dx
	inc	ax
	mov	byte ptr cs:[gBlitRows], al
	mov	ax, word ptr [bp + 0Eh]
	mov	byte ptr cs:[gBlitColour], al
	mov	cx, word ptr cs:[gBlitBytes]
	dec	cx
	mov	bp, cx
	add	cl, 7
	shr	cl, 1
	shr	cl, 1
	shr	cl, 1
	shl	bp, 1
	mov	bp, word ptr cs:[bp + gBitMaskTable]
	push	di
	mov	al, byte ptr cs:[gBlitColour]
	and	ax, 0Fh
	mov	di, ax
	shl	di, 1
	mov	ax, word ptr cs:[di + gColourPatterns]
	mov	word ptr cs:[gTextPattern], ax
	pop	di
	shr	di, 1
	xor	ah, ah
	mov	al, byte ptr cs:[gLeftMask]
	shl	ax, 1
	add	bp, ax
	mov	bp, word ptr cs:[bp]
	mov	al, byte ptr cs:[bp]
	mov	byte ptr cs:[gLeftMask], al
	mov	al, 0
	or	cl, cl
	je	L_25D3
	inc	bp
	mov	al, byte ptr cs:[bp]
L_25D3:
	mov	byte ptr cs:[gRightMask], al
	mov	ax, dx
	and	ax, 3FCh
	mov	dx, SCREEN_COLS
	mul	dx
	and	byte ptr cs:[gBlitBank], 3
	je	L_25F5
	xor	ch, ch
	mov	cl, byte ptr cs:[gBlitBank]
L_25F0:
	add	ax, BANK
	loop	L_25F0
L_25F5:
	add	di, ax
	xor	ah, ah
	mov	al, byte ptr cs:[gBlitWidth]
	inc	ax
	cmp	byte ptr cs:[gRightMask], 0
	je	L_2607
	inc	ax
L_2607:
	shl	ax, 1
	shl	ax, 1
	mov	bx, ax
	mov	ax, BANK
	sub	ax, bx
	mov	word ptr cs:[gBlitBankSkip], ax
	mov	ax, bx
	sub	ax, LINE_BYTES
	add	ax, 3 * BANK
	mov	word ptr cs:[gBlitBankWrap], ax
	call	DrawPageSeg
	mov	es, ax
	mov	ch, byte ptr cs:[gBlitWidth]
	shl	ch, 1
L_262E:
	xor	ah, ah
	mov	al, byte ptr cs:[gLeftMask]
	mov	cl, al
	mov	bx, ax
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	shl	bx, 1
	mov	ax, word ptr es:[di]
	mov	dx, word ptr cs:[gTextPattern]
	and	dx, word ptr cs:[bx + gNibblePixels]
	xor	ax, dx
	mov	word ptr es:[di], ax
	add	di, 2
	mov	bl, cl
	and	bx, 0Fh
	shl	bx, 1
	mov	ax, word ptr es:[di]
	mov	dx, word ptr cs:[gTextPattern]
	and	dx, word ptr cs:[bx + gNibblePixels]
	xor	ax, dx
	mov	word ptr es:[di], ax
	add	di, 2
	or	ch, ch
	je	L_2688
	mov	bx, word ptr cs:[gTextPattern]
	mov	cl, ch
L_267E:
	xor	word ptr es:[di], bx
	add	di, 2
	dec	cl
	jne	L_267E
L_2688:
	mov	al, byte ptr cs:[gRightMask]
	or	al, al
	je	L_26D1
	xor	ah, ah
	mov	cl, al
	mov	bx, ax
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	shl	bx, 1
	mov	ax, word ptr es:[di]
	mov	dx, word ptr cs:[gTextPattern]
	and	dx, word ptr cs:[bx + gNibblePixels]
	xor	ax, dx
	mov	word ptr es:[di], ax
	add	di, 2
	mov	bl, cl
	and	bx, 0Fh
	shl	bx, 1
	mov	ax, word ptr es:[di]
	mov	dx, word ptr cs:[gTextPattern]
	and	dx, word ptr cs:[bx + gNibblePixels]
	xor	ax, dx
	mov	word ptr es:[di], ax
	add	di, 2
L_26D1:
	mov	al, byte ptr cs:[gBlitBank]
	inc	al
	and	al, 3
	mov	byte ptr cs:[gBlitBank], al
	je	L_26EE
	add	di, word ptr cs:[gBlitBankSkip]
	dec	byte ptr cs:[gBlitRows]
	je	L_26FD
	jmp	L_262E
L_26EE:
	sub	di, word ptr cs:[gBlitBankWrap]
	dec	byte ptr cs:[gBlitRows]
	je	L_26FD
	jmp	L_262E
L_26FD:
	pop	es
	pop	dx
	pop	di
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
XorRect	ENDP

; TGA.OVL 2705 (export 36, gDrawShapeRemapped): DrawShape with each of the cell's bytes translated through
; gRemapTable
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
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	es, ax
	mov	si, word ptr [bp + 8]
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gRemapSeg], ax
	mov	bx, word ptr [bp + 0Ch]
	mov	ax, word ptr [bp + 0Eh]
	mov	word ptr cs:[gRemapY], ax
	mov	ax, word ptr cs:[gRegionLeft]
	shl	ax, 1
	shl	ax, 1
	mov	di, ax
	mov	byte ptr cs:[gOddX], 0
	sar	bx, 1
	rcl	byte ptr cs:[gOddX], 1
	sub	bx, ax
	mov	word ptr cs:[gRemapX], bx
	mov	ax, word ptr cs:[gRegionRight]
	sub	ax, word ptr cs:[gRegionLeft]
	mov	word ptr cs:[gRemapRegionCols], ax
	push	ds
	inc	si
	mov	ax, word ptr cs:[gRemapSeg]
	mov	ds, ax
	lodsw
	inc	si
	pop	ds
	push	ax
	and	ax, 0FFh
	mov	word ptr cs:[gRemapRows], ax
	pop	ax
	xchg	al, ah
	and	ax, 0FFh
	shl	ax, 1
	shl	ax, 1
	mov	word ptr cs:[gRemapVisibleBytes], ax
	mov	word ptr cs:[gRemapRowBytes], ax
	mov	ax, word ptr cs:[gRemapY]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_27B5
	neg	ax
	cmp	ax, word ptr cs:[gRemapRows]
	jae	L_27EA
	mov	dx, word ptr cs:[gRemapVisibleBytes]
	mul	dl
	add	si, ax
	mov	ax, word ptr cs:[gRemapRows]
	add	ax, word ptr cs:[gRemapY]
	sub	ax, word ptr cs:[gRegionTop]
	js	L_2825
	mov	word ptr cs:[gRemapRows], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gRemapY], ax
L_27B5:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gRemapY]
	js	L_2825
	je	L_2825
	cmp	ax, word ptr cs:[gRemapRows]
	jae	L_27CD
	mov	word ptr cs:[gRemapRows], ax
L_27CD:
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr cs:[gRemapX]
	or	ax, ax
	jns	L_27F9
	add	word ptr cs:[gRemapVisibleBytes], ax
	neg	ax
	add	si, ax
	cmp	ax, word ptr cs:[gRemapRowBytes]
L_27EA:
	jae	L_2825
	xor	ax, ax
	mov	word ptr cs:[gRemapX], ax
	mov	word ptr cs:[gSkipLeft], 1
L_27F9:
	mov	ax, word ptr cs:[gRemapRegionCols]
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr cs:[gRemapX]
	js	L_2825
	je	L_2825
	mov	word ptr cs:[gSkipRight], 0
	cmp	ax, word ptr cs:[gRemapVisibleBytes]
	jae	L_2827
	mov	word ptr cs:[gRemapVisibleBytes], ax
	mov	word ptr cs:[gSkipRight], 1
	jmp	short L_2827
L_2825:
	jmp	short L_2899
L_2827:
	mov	bx, word ptr cs:[gRemapY]
	shl	bx, 1
	add	di, word ptr cs:[bx + gRowOffsets]
	add	di, word ptr cs:[gRemapX]
	mov	ax, BANK
	sub	ax, word ptr cs:[gRemapVisibleBytes]
	mov	word ptr cs:[gRemapY], ax
	mov	dx, word ptr cs:[gRemapSeg]
	xor	bx, bx
L_284B:
	mov	cx, word ptr cs:[gRemapVisibleBytes]
	mov	bp, offset gRemapTable
	push	ds
	mov	ds, dx
L_2856:
	lodsb
	or	al, al
	je	L_2894
L_285B:
	mov	bx, bp
	xlat	byte ptr cs:[bx]
	xor	bx, bx
	mov	bl, al
	mov	bl, byte ptr cs:[bx + gKeepMasks]
	and	bl, byte ptr es:[di]
	or	al, bl
	stosb
	loop	L_2856
L_2870:
	pop	ds
	add	si, word ptr cs:[gRemapRowBytes]
	sub	si, word ptr cs:[gRemapVisibleBytes]
	add	di, word ptr cs:[gRemapY]
	jns	L_2886
	sub	di, BANK_WRAP
L_2886:
	dec	word ptr cs:[gRemapRows]
	jne	L_284B
	jmp	short L_2899
L_288F:
	lodsb
	or	al, al
	jne	L_285B
L_2894:
	inc	di
	loop	L_288F
	jmp	short L_2870
L_2899:
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

; TGA.OVL 28A3 (export 37, gDrawShapeFlippedRemapped): DrawShapeRemapped mirrored left to right
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
	mov	word ptr cs:[gSkipLeft], 0
	xor	di, di
	mov	ax, word ptr [bp + 6]
	call	PageSeg
	mov	word ptr cs:[gDstSeg], ax
	mov	si, word ptr [bp + 8]
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gRemapSeg], ax
	mov	bx, word ptr [bp + 0Ch]
	mov	ax, word ptr [bp + 0Eh]
	mov	word ptr cs:[gRemapY], ax
	mov	ax, word ptr cs:[gRegionLeft]
	shl	ax, 1
	shl	ax, 1
	add	di, ax
	mov	byte ptr cs:[gOddX], 0
	sar	bx, 1
	rcl	byte ptr cs:[gOddX], 1
	sub	bx, ax
	mov	word ptr cs:[gRemapX], bx
	mov	ax, word ptr cs:[gRegionRight]
	sub	ax, word ptr cs:[gRegionLeft]
	mov	word ptr cs:[gRemapRegionCols], ax
	push	ds
	inc	si
	mov	ax, word ptr cs:[gRemapSeg]
	mov	ds, ax
	lodsw
	inc	si
	pop	ds
	push	ax
	and	ax, 0FFh
	mov	word ptr cs:[gRemapRows], ax
	pop	ax
	xchg	al, ah
	and	ax, 0FFh
	shl	ax, 1
	shl	ax, 1
	mov	word ptr cs:[gRemapVisibleBytes], ax
	mov	word ptr cs:[gRemapRowBytes], ax
	mov	ax, word ptr cs:[gRemapY]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_295E
	neg	ax
	cmp	ax, word ptr cs:[gRemapRows]
	jae	L_2999
	mov	dx, word ptr cs:[gRemapVisibleBytes]
	mul	dl
	add	si, ax
	mov	ax, word ptr cs:[gRemapRows]
	add	ax, word ptr cs:[gRemapY]
	sub	ax, word ptr cs:[gRegionTop]
	js	L_2999
	mov	word ptr cs:[gRemapRows], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gRemapY], ax
L_295E:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gRemapY]
	js	L_2999
	je	L_2999
	cmp	ax, word ptr cs:[gRemapRows]
	jae	L_2976
	mov	word ptr cs:[gRemapRows], ax
L_2976:
	mov	ax, word ptr cs:[gRemapX]
	or	ax, ax
	jns	L_299C
	add	word ptr cs:[gRemapVisibleBytes], ax
	neg	ax
	cmp	ax, word ptr cs:[gRemapRowBytes]
	jae	L_2999
	xor	ax, ax
	mov	word ptr cs:[gRemapX], ax
	inc	word ptr cs:[gSkipLeft]
	jmp	short L_299C
L_2999:
	jmp	L_2A63
L_299C:
	mov	ax, word ptr cs:[gRemapRegionCols]
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr cs:[gRemapX]
	js	L_2999
	je	L_2999
	mov	word ptr cs:[gSkipRight], 0
	cmp	ax, word ptr cs:[gRemapVisibleBytes]
	ja	L_29CD
	add	si, word ptr cs:[gRemapVisibleBytes]
	sub	si, ax
	mov	word ptr cs:[gRemapVisibleBytes], ax
	mov	word ptr cs:[gSkipRight], 1
L_29CD:
	mov	bx, word ptr cs:[gRemapY]
	shl	bx, 1
	add	di, word ptr cs:[bx + gRowOffsets]
	add	di, word ptr cs:[gRemapX]
	mov	ax, BANK
	sub	ax, word ptr cs:[gRemapVisibleBytes]
	mov	word ptr cs:[gRemapY], ax
	mov	ax, word ptr cs:[gRemapVisibleBytes]
	sub	ax, LINE_BYTES
	add	ax, 3 * BANK
	mov	word ptr cs:[gRemapX], ax
	mov	dx, word ptr cs:[gRemapSeg]
	dec	si
	mov	ax, word ptr cs:[gRemapUnused]
	xchg	ah, al
L_2A04:
	xor	bx, bx
	mov	cx, word ptr cs:[gDstSeg]
	mov	es, cx
	mov	cx, word ptr cs:[gRemapVisibleBytes]
	add	si, cx
	push	ds
	mov	ds, dx
	mov	bp, offset gRemapTable
L_2A1A:
	mov	al, byte ptr [si]
	or	al, al
	je	L_2A5D
L_2A20:
	mov	bx, bp
	xlat	byte ptr cs:[bx]
	xor	bx, bx
	mov	bl, al
	mov	bl, byte ptr cs:[bx + gNibbleSwap]
	mov	al, bl
	mov	bl, byte ptr cs:[bx + gKeepMasks]
	and	bl, byte ptr es:[di]
	or	al, bl
	stosb
	dec	si
	loop	L_2A1A
L_2A3D:
	pop	ds
	add	si, word ptr cs:[gRemapRowBytes]
	add	di, word ptr cs:[gRemapY]
	jns	L_2A4E
	sub	di, BANK_WRAP
L_2A4E:
	dec	word ptr cs:[gRemapRows]
	jne	L_2A04
	jmp	short L_2A63
L_2A57:
	mov	bl, byte ptr [si]
	or	bl, bl
	jne	L_2A20
L_2A5D:
	inc	di
	dec	si
	loop	L_2A57
	jmp	short L_2A3D
L_2A63:
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

; TGA.OVL 2A6D (export 35, gSetRemapTable): SetRemapTable(void far *colours) builds gRemapTable from 16
; colours: the byte of pixels (h, l) maps to (colours[h], colours[l])
SetRemapTable	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	di
	push	ds
	push	es
	mov	bx, word ptr [bp + 6]
	mov	ax, word ptr [bp + 8]
	mov	ds, ax
	mov	di, offset gRemapTable
	mov	ax, cs
	mov	es, ax
	xor	dl, dl
	mov	cl, 4
L_2A8A:
	mov	al, dl
	xlat
	shl	al, cl
	mov	dh, al
	xor	ch, ch
L_2A93:
	mov	al, ch
	xlat
	or	al, dh
	stosb
	inc	ch
	cmp	ch, 10h
	jne	L_2A93
	inc	dl
	cmp	dl, 10h
	jne	L_2A8A
	pop	es
	pop	ds
	pop	di
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
SetRemapTable	ENDP

DriverEnd	LABEL	BYTE		; 2AB0: the end of the image, DriverEntry's size
DRIVER	ENDS
	END
