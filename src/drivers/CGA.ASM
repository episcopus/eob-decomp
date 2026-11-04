; CGA.OVL: the graphics driver for the CGA choice, 320 by 200 pixels in 4 colours (BIOS mode 4), two bits a
; pixel, four pixels a byte (leftmost in the high bits), 80 bytes a line, the even lines at B800:0000 and the
; odd lines at B800:2000. The game loads and links it as it does MCGA.OVL (whitepaper 2.5): the same entry
; jump, the same slot table of seven imports and forty exports (cs:0003), the same calls. The pages the game
; gives the driver are in this screen format (16 KB; ClearPage clears 2000h words), except the art pages, which
; hold one 16-colour pixel a byte as on the other drivers: CopyPage and GetSpriteCell turn those into CGA
; pixels through two dither tables (cs:04B1 for even lines, cs:05B1 for odd), which SetDitherTables builds
; from a 32-byte colour map the caller passes: map[c] and map[16 + c] are the two CGA colours of colour c,
; set in a checkerboard. The game's gCgaMapDefault (ds:113F) and the 32-byte segments gCgaMapPlayfld
; (2069:0000), gCgaMapInvent (2067:0000) and their like are those maps; the other drivers ignore the pointer.
;
; Seven exports do nothing here: DrawIndexedTile, SetTileSet, DrawMaskedCell16, DrawShapeMasked and its mirror,
; SetPalette and SetTileFormat return at once (push bp, mov bp,sp, pop bp, retf), each followed by the body it
; had, as dead code (QUIRK). The routines are otherwise MCGA.OVL's in the CGA format, at other offsets; the
; comments say where they differ. Routines are far (retf) unless their comment says near. TASM 2.0 and TLINK
; 3.0 build the whole file from this source (make rebuild PROGRAM=CGA.OVL).

SLOT_COUNT	equ	47		; slots in the table at cs:0003: 7 imports and 40 exports
SCREEN_WIDTH	equ	320		; pixels in a line, and bytes in a line of an art page
SCREEN_HEIGHT	equ	200
SCREEN_COLS	equ	40		; 8-pixel columns in a line
LINE_BYTES	equ	80		; bytes in a CGA line
ODD_BANK	equ	2000h		; offset of the odd lines' bank
FONT_HEIGHT	equ	102h		; the font's byte offsets: its height in lines
FONT_WIDTH	equ	103h		; and its width in pixels
MASK_PLANE	equ	8000		; bytes in one bit plane of page 4 (the dead masked draws)

DRIVER	SEGMENT BYTE PUBLIC 'CODE'
	ASSUME	cs:DRIVER, ds:NOTHING, es:NOTHING, ss:NOTHING

; CGA.OVL 0000: the image starts with a jump to DriverEntry; the game calls segment:0000 far after loading
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
	dd	gMirrorBytes		; export 38, slot 45, ds:80DE
	dd	gRemapTable		; export 39, slot 46, ds:80E2 gRemapTable
gSlotCount	dw	SLOT_COUNT		; the slots LinkDriver walks

; cs:00C1: the region and the shape draws' state (as MCGA.OVL's, in CGA bytes)
gSavedLeft	dw	0		; SaveDrawState keeps the region here (in columns and lines)
gSavedRight	dw	0
gSavedTop	dw	0
gSavedBottom	dw	0
gFillPattern	db	0		; FillRect's colour, repeated in the four pixels of a byte
gRegionLeft	dw	0		; the drawing region SetRegion sets: left and right in 8-pixel columns
gRegionRight	dw	0
gRegionTop	dw	0		; top and bottom in lines
gRegionBottom	dw	0
gShapeSeg	dw	0		; the shape draws' state: the shape's segment
gShapeX	dw	0
gShapeY	dw	0
gShapeRows	dw	0
gShapeWidth	dw	0
gShapeFullWidth	dw	0
gRegionCols	dw	0
gSkipLeft	dw	0
gSkipRight	dw	0
gVisibleWidth	dw	0
gVisibleBytes	dw	0		; the visible bytes of a row
gShapeRowBytes	dw	0		; the shape's bytes in a row
	db	8 dup (0)		; 00EA: not used
gPixelShift	db	0		; the shape's x within its byte, in bits
gShapeRowBytes2	dw	0
gRowParity	dw	0		; the line's bank: 1 on an odd line
	db	2 dup (0)		; 00F7: not used

; cs:00F9 (export 38): each byte with its four pixels in the other order, for the mirrored draws
gMirrorBytes LABEL BYTE
MirrorByte = 0
	REPT	256
	db	((MirrorByte AND 3) SHL 6) OR ((MirrorByte AND 0Ch) SHL 2) OR ((MirrorByte AND 30h) SHR 2) OR (MirrorByte SHR 6)
MirrorByte = MirrorByte + 1
	ENDM

; cs:01F9 (export 39, the game's gRemapTable): each CGA byte with its four pixels remapped, built by
; SetRemapTable; the remapped draws translate the shape's bytes through it
gRemapTable	db	256 dup (0)

; cs:02F9: SetRemapTable's loop, the dead masked draws' pages, the sprite cutter, SetLineClip's rectangle
gRemapIndex0	db	0		; the pixel taken for each of the four positions
gRemapIndex1	db	0
gRemapIndex2	db	0
gRemapIndex3	db	0
gMaskPlaneSeg	dw	0		; DrawShapeMasked (dead): page 4's segment
gBackPageSeg	dw	0		; and page 2's
gMaskBit	db	0
gMaskLevel	dw	0
	db	2 dup (0)		; 0304: not used
gCellBytes	dw	0		; the cell's pixel bytes
	db	2 dup (0)		; 0308: not used
gCellRowEnd	dw	0		; the end of the source row being cut
gOtherDitherTable	dw	0		; gDitherEven XOR gDitherOdd: XORed into the table pointer every line
	db	1 dup (0)		; 030E: not used
gLineClipLeft	dw	0		; the line clip rectangle, in pixels (SetLineClip)
gLineClipTop	dw	0
gLineClipRight	dw	13Fh
gLineClipBottom	dw	0C7h
gCellSeg	dw	0		; the allocated cell: segment
gCellOff	dw	0		; and offset
	db	4 dup (0)		; 031B: not used
gCellRowStart	dw	0		; the start of the source row

; cs:0321 (export 34, the game's gRowOffsetTableOff): the offset of each screen line in the CGA layout,
; the even lines in the first bank and the odd ones at ODD_BANK
gRowOffsets LABEL WORD
RowOffset = 0
	REPT	SCREEN_HEIGHT / 2
	dw	RowOffset, ODD_BANK + RowOffset
RowOffset = RowOffset + LINE_BYTES
	ENDM

; cs:04B1: the dither table of the even lines: for a byte of two 16-colour pixels (high nibble first), the four
; bits of their two CGA pixels; SetDitherTables builds it from a colour map (map[hi] SHL 2 OR map[16 + lo]).
; The image holds the table of the map map[c] = c SHR 2, map[16 + c] = c AND 3.
gDitherEven LABEL BYTE
DitherPair = 0
	REPT	256
	db	((DitherPair SHR 6) SHL 2) OR (DitherPair AND 3)
DitherPair = DitherPair + 1
	ENDM

; cs:05B1: the dither table of the odd lines (map[16 + hi] SHL 2 OR map[lo]), the other half of the checkerboard
gDitherOdd LABEL BYTE
DitherPair = 0
	REPT	256
	db	(DitherPair SHR 2) AND 0Fh
DitherPair = DitherPair + 1
	ENDM

; cs:06B1: pixel masks by x AND 3
gPixelBits	db	0C0h, 30h, 0Ch, 3		; the bits of the pixel
gPixelKeep	db	3Fh, 0CFh, 0F3h, 0FCh		; the bits of the other three
gFillLeftKeep	db	0, 0C0h, 0F0h, 0FCh		; FillRect: the bits left of a span's first pixel; then, for a span
	db	3Fh, 0Fh, 3, 0, 0Fh, 0CFh, 0C3h, 0C0h	; inside one byte, indexed by start + 4 * (end + 1),
	db	3, 0C3h, 0F3h, 0F0h, 0, 0C0h, 0F0h, 0FCh	; the bits outside it
gFillRightKeep	db	3Fh, 0Fh, 3, 0		; the bits right of a span's last pixel
gPixelShifts	db	6, 4, 2, 0		; the shift that brings the pixel down to bits 0 and 1

; cs:06D5: the tile draws' state (only the dead DrawIndexedTile, SetTileSet and SetTileFormat use it)
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

; CGA.OVL 0707 (export 0, gCopyPage): CopyPage(srcSeg, dstSeg, void far *map) turns a 64000-byte art page
; (one 16-colour pixel a byte) into a CGA screen page: each pair of pixels becomes two CGA pixels through
; gDitherEven or gDitherOdd by the line's parity, after SetDitherTables has built them from map when map is
; not null. The game's CopyPage (19EC:002A) passes its two extra words as map.
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
	push	bp
	mov	ax, word ptr [bp + 0Ah]
	mov	dx, word ptr [bp + 0Ch]
	or	ax, dx
	je	L_0723
	mov	ax, word ptr [bp + 0Ah]
	call	SetDitherTables
L_0723:
	mov	ds, word ptr [bp + 6]
	mov	es, word ptr [bp + 8]
	xor	si, si
	mov	di, si
	mov	cx, 3E80h
	xor	dh, dh
L_0732:
	mov	bx, offset gDitherEven
L_0735:
	mov	dl, LINE_BYTES
L_0737:
	lodsw
	and	ax, 0F0Fh
	shl	al, 1
	shl	al, 1
	shl	al, 1
	shl	al, 1
	or	al, ah
	xor	ah, ah
	mov	bp, bx
	add	bp, ax
	mov	al, byte ptr cs:[bp]
	shl	al, 1
	shl	al, 1
	shl	al, 1
	shl	al, 1
	mov	byte ptr cs:[gPixelPair], al
	lodsw
	and	ax, 0F0Fh
	shl	al, 1
	shl	al, 1
	shl	al, 1
	shl	al, 1
	or	al, ah
	xor	ah, ah
	mov	bp, bx
	add	bp, ax
	mov	al, byte ptr cs:[bp]
	or	al, byte ptr cs:[gPixelPair]
	stosb
	dec	dl
	je	L_0781
	loop	L_0737
	jmp	short L_0795
L_0781:
	xor	dh, 1
	je	L_078F
	mov	bx, offset gDitherOdd
	add	di, 1FB0h
	jmp	short L_0735
L_078F:
	sub	di, ODD_BANK
	jmp	short L_0732
L_0795:
	pop	bp
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

; cs:07A0: a 32-byte colour map in the format SetDitherTables reads (two CGA colours for each of 16), which
; nothing in the driver reads; the game passes its own (ds:113F and the like). Then CopyPage's and FillRect's
; byte variables.
gDefaultCgaMap LABEL BYTE
	db	0h, 2h, 3h, 1h, 1h, 3h, 2h, 3h, 2h, 1h, 2h, 0h, 0h, 2h, 1h, 3h
	db	0h, 2h, 1h, 0h, 1h, 3h, 2h, 2h, 0h, 3h, 2h, 0h, 0h, 2h, 1h, 3h
gPixelPair	db	0		; CopyPage: the first pixel pair's bits; FillRect: the start pixel within its byte
gFillEndBits	db	0		; FillRect: the end pixel within its byte

; CGA.OVL 07C2 (export 1, gRestoreRectToPage): RestoreRect on the page gDrawPage names instead of page 0
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
	pop	bp		; QUIRK 07E0: dead code, the jmp above skips it
	retf
RestoreRectToPage	ENDP

; CGA.OVL 07E2 (export 2, gRestoreRect): RestoreRect(col, y, cols, rows, void far *buf) copies a buffer of
; CGA bytes (cols * 2 a row) back to page 0, row by row through the two banks
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
	jge	L_0801
	xor	di, di
L_0801:
	cmp	di, SCREEN_COLS
	jl	L_0809
	mov	di, SCREEN_COLS - 1
L_0809:
	shl	di, 1
	mov	dx, word ptr [bp + 8]
	cmp	dx, 0
	jge	L_0815
	xor	dx, dx
L_0815:
	cmp	dx, SCREEN_HEIGHT
	jl	L_081E
	mov	dx, SCREEN_HEIGHT - 1
L_081E:
	mov	bx, word ptr [bp + 0Ah]
	mov	cx, word ptr [bp + 0Ch]
	cmp	bx, SCREEN_COLS
	jbe	L_082C
	mov	bx, SCREEN_COLS
L_082C:
	or	bx, bx
	je	L_087A
	js	L_087A
	cmp	cx, SCREEN_HEIGHT
	jbe	L_083B
	mov	cx, SCREEN_HEIGHT
L_083B:
	or	cx, cx
	je	L_087A
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
	mov	ax, ODD_BANK
	cmp	di, ODD_BANK
	jb	L_0868
	mov	ax, 0E050h
L_0868:
	push	bp
	mov	bp, di
L_086B:
	mov	cx, bx
	mov	di, bp
	rep movsw
	add	bp, ax
	xor	ax, 0C050h
	dec	dx
	jne	L_086B
	pop	bp
L_087A:
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

; CGA.OVL 0886 (export 3, gCopyRegion): CopyRegion(srcCol, srcY, dstCol, dstY, cols, rows, srcPage,
; dstPage), as MCGA.OVL's, two CGA bytes to a column
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
	jge	L_08EC
	or	ax, ax
	jns	L_08A0
	xor	ax, ax
L_08A0:
	shl	ax, 1
	mov	si, ax
	mov	ax, word ptr [bp + 8]
	cmp	ax, SCREEN_HEIGHT
	jge	L_08EC
	or	ax, ax
	jns	L_08B2
	xor	ax, ax
L_08B2:
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	si, ax
	mov	ax, word ptr [bp + 0Ah]
	cmp	ax, SCREEN_COLS
	jge	L_08EC
	or	ax, ax
	jns	L_08CB
	xor	ax, ax
L_08CB:
	shl	ax, 1
	mov	di, ax
	mov	ax, word ptr [bp + 0Ch]
	mov	cx, ax
	cmp	ax, SCREEN_HEIGHT
	jge	L_0951
	or	ax, ax
	jns	L_08DF
	xor	ax, ax
L_08DF:
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	di, ax
	jmp	short L_08EE
L_08EC:
	jmp	short L_0951
L_08EE:
	mov	ax, word ptr [bp + 12h]
	call	PageSeg
	mov	ds, ax
	mov	ax, word ptr [bp + 14h]
	call	PageSeg
	mov	es, ax
	mov	dx, word ptr [bp + 0Eh]
	cmp	dx, SCREEN_COLS
	ja	L_0951
	cmp	dx, 1
	jl	L_0951
	mov	ax, word ptr [bp + 10h]
	add	cx, ax
	cmp	cx, SCREEN_HEIGHT
	jbe	L_091C
	sub	cx, SCREEN_HEIGHT - 1
	sub	ax, cx
L_091C:
	or	ax, ax
	js	L_0951
	push	bp
	mov	bp, ODD_BANK
	cmp	si, ODD_BANK
	jb	L_092D
	mov	bp, 0E050h
L_092D:
	mov	bx, ODD_BANK
	cmp	di, ODD_BANK
	jb	L_0939
	mov	bx, 0E050h
L_0939:
	push	si
	push	di
	mov	cx, dx
	rep movsw
	pop	di
	pop	si
	add	si, bp
	add	di, bx
	xor	bp, 0C050h
	xor	bx, 0C050h
	dec	ax
	jne	L_0939
	pop	bp
L_0951:
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

; CGA.OVL 095B (export 4, gClearPage): fills the draw page's 16 KB (both banks) with colour 0
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
	mov	cx, ODD_BANK
	xor	ax, ax
	rep stosw
	pop	es
	pop	di
	pop	cx
	pop	ax
	pop	bp
	retf
ClearPage	ENDP

; CGA.OVL 0977 (export 5, gDrawCell8x8): DrawCell8x8(void far *src, col, y) copies 8 rows of 2 bytes from src
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
	jge	L_0995
	xor	ax, ax
L_0995:
	cmp	ax, SCREEN_HEIGHT - 7
	jl	L_099D
	mov	ax, SCREEN_HEIGHT - 8
L_099D:
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	mov	dx, word ptr [bp + 0Ah]
	cmp	dx, 0
	jge	L_09B0
	xor	dx, dx
L_09B0:
	cmp	dx, SCREEN_COLS
	jl	L_09B8
	mov	dx, SCREEN_COLS - 1
L_09B8:
	shl	dx, 1
	add	ax, dx
	mov	di, ax
	call	DrawPageSeg
	mov	es, ax
	mov	dx, 8
	mov	bx, 1FFEh
	cmp	di, ODD_BANK
	jb	L_09D2
	mov	bx, 0E04Eh
L_09D2:
	movsw
	add	di, bx
	xor	bx, 0FFB0h
	dec	dx
	jne	L_09D2
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

; CGA.OVL 09E5 (export 6, gDrawIndexedTile): returns at once; the dead body after it is MCGA.OVL's DrawIndexedTile
DrawIndexedTile	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
	push	ax		; QUIRK 09EA: dead code, DrawIndexedTile returns above; its body as MCGA.OVL has it
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
L_0A1E:
	mov	al, byte ptr es:[bx + di]
	mov	byte ptr cs:[bx + gTileColourRow], al
	dec	bx
	jge	L_0A1E
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
	dw	L_0A56, L_0A56, L_0A5E
L_0A56:
	mov	ax, word ptr [bp + 6]
	call	DrawTilePixels
	jmp	short L_0AA8
L_0A5E:
	jmp	short L_0AA8
DrawIndexedTile	ENDP

; CGA.OVL 0A60 (near, dead): DrawIndexedTile's pixel loop
DrawTilePixels	PROC	NEAR
	mov	dx, word ptr cs:[gTileParas]
	mul	dx
	mov	si, word ptr cs:[gTilePixelsPtr]
	add	ax, word ptr cs:[gTilePixelsPtr+2]
	mov	ds, ax
	mov	dx, word ptr cs:[gTileRows]
	xor	bh, bh
L_0A7A:
	mov	cx, word ptr cs:[gTileRowBytes]
L_0A7F:
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
	loop	L_0A7F
	add	di, word ptr cs:[gTileDstSkip]
	dec	dx
	jne	L_0A7A
	ret
L_0AA8:
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

; CGA.OVL 0AB3 (export 7, gDrawLine): DrawLine(x1, y1, x2, y2, colour), clipped to SetLineClip's rectangle as
; in MCGA.OVL; the colour picks a byte of gLineColourBytes, whose pixel at each x is drawn
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

; CGA.OVL 0ADD (near): the clipped line (ax = x1, bx = y1, cx = x2, dx = y2, colour di), as MCGA.OVL 080B;
; the line itself runs in one of four loops (gLineSteps) by its direction
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
	jl	L_0B25
	cmp	ax, word ptr cs:[gLineClipRight]
	jg	L_0B25
	cmp	bx, word ptr cs:[gLineClipTop]
	jl	L_0B25
	cmp	bx, word ptr cs:[gLineClipBottom]
	jg	L_0B25
	cmp	cx, word ptr cs:[gLineClipLeft]
	jl	L_0B25
	cmp	cx, word ptr cs:[gLineClipRight]
	jg	L_0B25
	cmp	dx, word ptr cs:[gLineClipTop]
	jl	L_0B25
	cmp	dx, word ptr cs:[gLineClipBottom]
	jle	L_0B4F
L_0B25:
	call	LineOutcode
	xchg	cx, ax
	xchg	bx, dx
	mov	di, si
	call	LineOutcode
	mov	bp, di
	or	bp, si
	je	L_0B4F
	test	di, si
	jne	L_0B52
	shl	si, 1
	call	word ptr cs:[si + gClipByOutcode]
	jb	L_0B25
	xchg	cx, ax
	xchg	bx, dx
	shl	di, 1
	call	word ptr cs:[di + gClipByOutcode]
	jmp	short L_0B25
L_0B4F:
	jmp	DrawLineInside
L_0B52:
	jmp	L_0E4C
gClipByOutcode	LABEL	WORD
	dw	ClipKeep, ClipToTop, ClipToBottom, ClipKeep
	dw	ClipToLeft, ClipToLeft, ClipToBottom, ClipKeep
	dw	ClipToRight, ClipToTop, ClipToRight, ClipKeep
	dw	ClipKeep, ClipKeep, ClipKeep, ClipKeep
DrawClippedLine	ENDP

; CGA.OVL 0B75 (near): outcode 0 and impossible codes: nothing to move, carry clear
ClipKeep	PROC	NEAR
	clc
	ret
ClipKeep	ENDP

; CGA.OVL 0B77 (near): moves the end (ax, bx) to the clip top along the line, carry set
ClipToTop	PROC	NEAR
	mov	si, word ptr cs:[gLineClipTop]
	call	MoveToLineY
	stc
	ret
ClipToTop	ENDP

; CGA.OVL 0B81 (near): moves the end to the clip bottom (by mirroring y and using MoveToLineY)
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

; CGA.OVL 0B95 (near): the x where the line from (ax, bx) to (cx, dx) meets y = si; returns ax = that x, bx = si
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

; CGA.OVL 0BAE (near): moves the end to the clip left edge, carry set
ClipToLeft	PROC	NEAR
	mov	si, word ptr cs:[gLineClipLeft]
	call	MoveToLineX
	stc
	ret
ClipToLeft	ENDP

; CGA.OVL 0BB8 (near): moves the end to the clip right edge (by mirroring x and using MoveToLineX)
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

; CGA.OVL 0BCC (near): the y where the line meets x = si; returns bx = that y, ax = si
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

; CGA.OVL 0BE1 (near): the outcode of the point (ax, bx) in si: 1 above the clip rectangle, 2 below, 4 left of
; it, 8 right of it; DrawLineInside (0C0C) follows, the line within the rectangle
LineOutcode	PROC	NEAR
	xor	si, si
	cmp	bx, word ptr cs:[gLineClipTop]
	jge	L_0BED
	or	si, 1
L_0BED:
	cmp	bx, word ptr cs:[gLineClipBottom]
	jle	L_0BF7
	or	si, 2
L_0BF7:
	cmp	ax, word ptr cs:[gLineClipLeft]
	jge	L_0C01
	or	si, 4
L_0C01:
	cmp	ax, word ptr cs:[gLineClipRight]
	jle	L_0C0B
	or	si, 8
L_0C0B:
	ret
DrawLineInside:
	cmp	dx, bx
	jge	L_0C13
	xchg	dx, bx
	xchg	cx, ax
L_0C13:
	xor	si, si
	sub	dx, bx
	sub	cx, ax
	jns	L_0C20
	neg	cx
	or	si, 1
L_0C20:
	cmp	cx, dx
	jge	L_0C29
	xchg	cx, dx
	or	si, 2
L_0C29:
	mov	word ptr cs:[gLineRowStep], cx
	shr	cx, 1
	mov	bp, cx
	mov	word ptr cs:[gLineMinor], dx
	mov	cl, 2
	xchg	bx, ax
	mov	word ptr cs:[gLineX], ax
	mov	dx, SCREEN_COLS
	mul	dx
	mov	di, bx
	shr	di, cl
	add	di, ax
	test	word ptr cs:[gLineX], 1
	je	L_0C56
	add	di, 1FD8h
L_0C56:
	and	bx, 3
	mov	bl, byte ptr cs:[bx + gLinePixelBits]
	mov	cx, word ptr cs:[gLineRowStep]
	inc	cx
	mov	ax, word ptr cs:[gLineColour]
	xor	ah, ah
	and	al, 3
	mov	bh, al
	shl	al, 1
	shl	al, 1
	or	bh, al
	shl	al, 1
	shl	al, 1
	or	bh, al
	shl	al, 1
	shl	al, 1
	or	bh, al
	xor	al, al
	shl	si, 1
	jmp	word ptr cs:[si + gLineSteps]
L_0C89:
	mov	si, word ptr cs:[gLineMinor]
	mov	ah, SCREEN_HEIGHT - 8
	mov	word ptr cs:[gLineCount], cx
	mov	cx, 1FFFh
	cmp	di, ODD_BANK
	jb	L_0CA1
	mov	cx, 0E04Fh
L_0CA1:
	or	al, bl
	sub	bp, si
	jns	L_0CC1
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	xor	al, al
	add	di, cx
	xor	cx, 0FFB0h
	add	bp, word ptr cs:[gLineRowStep]
L_0CC1:
	shr	bl, 1
	shr	bl, 1
	jb	L_0CDF
	dec	word ptr cs:[gLineCount]
	jne	L_0CA1
L_0CCE:
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	jmp	L_0E4C
L_0CDF:
	mov	bl, ah
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	xor	al, al
	dec	word ptr cs:[gLineCount]
	jne	L_0CA1
	je	L_0CCE
L_0CFA:
	mov	si, word ptr cs:[gLineMinor]
	mov	ah, 3
	mov	word ptr cs:[gLineCount], cx
	mov	cx, 1FFFh
	cmp	di, ODD_BANK
	jb	L_0D12
	mov	cx, 0E04Fh
L_0D12:
	or	al, bl
	sub	bp, si
	jns	L_0D32
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	xor	al, al
	add	di, cx
	xor	cx, 0FFB0h
	add	bp, word ptr cs:[gLineRowStep]
L_0D32:
	shl	bl, 1
	shl	bl, 1
	jb	L_0D50
	dec	word ptr cs:[gLineCount]
	jne	L_0D12
L_0D3F:
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	jmp	L_0E4C
L_0D50:
	mov	bl, ah
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	xor	al, al
	sub	di, 2
	dec	word ptr cs:[gLineCount]
	jne	L_0D12
	je	L_0D3F
L_0D6E:
	mov	si, word ptr cs:[gLineMinor]
	mov	al, bl
	mov	ah, SCREEN_HEIGHT - 8
	mov	word ptr cs:[gLineCount], cx
	mov	cx, 1FFFh
	cmp	di, ODD_BANK
	jb	L_0D88
	mov	cx, 0E04Fh
L_0D88:
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	add	di, cx
	xor	cx, 0FFB0h
	sub	bp, si
	jns	L_0DAA
	shr	bl, 1
	shr	bl, 1
	jb	L_0DB6
	add	bp, word ptr cs:[gLineRowStep]
L_0DAA:
	mov	al, bl
	dec	word ptr cs:[gLineCount]
	jne	L_0D88
	jmp	L_0E4C
L_0DB6:
	mov	bl, ah
	inc	di
	add	bp, word ptr cs:[gLineRowStep]
	mov	al, bl
	dec	word ptr cs:[gLineCount]
	jne	L_0D88
	jmp	L_0E4C
L_0DCA:
	mov	si, word ptr cs:[gLineMinor]
	mov	ah, 3
	mov	al, bl
	mov	word ptr cs:[gLineCount], cx
	mov	cx, 1FFFh
	cmp	di, ODD_BANK
	jb	L_0DE4
	mov	cx, 0E04Fh
L_0DE4:
	mov	dl, byte ptr es:[di]
	mov	dh, bh
	and	dh, al
	not	al
	and	al, dl
	or	al, dh
	stosb
	add	di, cx
	xor	cx, 0FFB0h
	sub	bp, si
	jns	L_0E06
	shl	bl, 1
	shl	bl, 1
	jb	L_0E11
	add	bp, word ptr cs:[gLineRowStep]
L_0E06:
	mov	al, bl
	dec	word ptr cs:[gLineCount]
	jne	L_0DE4
	jmp	short L_0E4C
L_0E11:
	mov	bl, ah
	dec	di
	add	bp, word ptr cs:[gLineRowStep]
	mov	al, bl
	dec	word ptr cs:[gLineCount]
	jne	L_0DE4
	jmp	short L_0E4C
LineOutcode	ENDP

; cs:0E24: DrawLine's state
gLineCount	dw	0		; pixels left
	db	2 dup (0)		; 0E26: not used
gLineRowStep	dw	0
gLineMinor	dw	0
gLineX	dw	0
gLinePixelBits	db	0C0h, 30h, 0Ch, 3	; the bits of a pixel, by x AND 3

gLineSteps	LABEL	WORD
	dw	L_0C89, L_0CFA, L_0D6E, L_0DCA
gLineColourBytes LABEL BYTE		; for each of the 16 colours, a CGA byte of four pixels to draw lines in
	db	0h, 0AAh, 55h, 55h, 55h, 0FFh, 0AAh, 0AAh, 55h, 55h, 55h, 0h, 0h, 0h, 0AAh, 0FFh
gLineColour	dw	0		; DrawLine's colour, kept inside the code (the jmp above skips it)

L_0E4C:
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
; CGA.OVL 0E56 (export 9, gFillRect): FillRect(x1, y1, x2, y2, colour) fills the rectangle on the draw page
; with colour AND 3 (gFillPattern), keeping the pixels outside it in its first and last bytes
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
	and	al, 3
	mov	ah, al
	shl	al, 1
	shl	al, 1
	or	ah, al
	mov	al, ah
	mov	cl, 4
	shl	al, cl
	or	al, ah
	mov	byte ptr cs:[gFillPattern], al
	call	DrawPageSeg
	mov	es, ax
	mov	ax, word ptr [bp + 6]
	cmp	ax, SCREEN_WIDTH
	jge	L_0EC6
	or	ax, ax
	jns	L_0E8D
	xor	ax, ax
L_0E8D:
	mov	bx, word ptr [bp + 8]
	cmp	bx, SCREEN_HEIGHT
	jge	L_0EC6
	or	bx, bx
	jns	L_0E9C
	xor	bx, bx
L_0E9C:
	mov	cx, word ptr [bp + 0Ah]
	cmp	cx, 0
	jge	L_0EA6
	xor	cx, cx
L_0EA6:
	cmp	cx, SCREEN_WIDTH
	jge	L_0EC6
	mov	dx, word ptr [bp + 0Ch]
	cmp	dx, 0
	jge	L_0EB6
	xor	dx, dx
L_0EB6:
	cmp	dx, SCREEN_HEIGHT
	jge	L_0EC6
	cmp	ax, cx
	ja	L_0EC6
	cmp	bx, dx
	ja	L_0EC6
	jmp	short L_0EC9
L_0EC6:
	jmp	L_0F9F
L_0EC9:
	cmp	cx, SCREEN_WIDTH
	jl	L_0ED2
	mov	cx, SCREEN_WIDTH - 1
L_0ED2:
	cmp	dx, SCREEN_HEIGHT
	jl	L_0EDB
	mov	dx, SCREEN_HEIGHT - 1
L_0EDB:
	sub	dx, bx
	shl	bx, 1
	mov	di, word ptr cs:[bx + gRowOffsets]
	mov	bp, ODD_BANK
	cmp	di, ODD_BANK
	jb	L_0EF0
	mov	bp, 0E050h
L_0EF0:
	mov	byte ptr cs:[gPixelPair], al
	and	byte ptr cs:[gPixelPair], 3
	mov	si, ax
	shr	ax, 1
	shr	ax, 1
	add	di, ax
	mov	byte ptr cs:[gFillEndBits], cl
	and	byte ptr cs:[gFillEndBits], 3
	mov	bx, cx
	shr	bx, 1
	shr	bx, 1
	cmp	bx, ax
	je	L_0F83
	inc	cx
	shr	cx, 1
	shr	cx, 1
	sub	cx, ax
	je	L_0F29
	cmp	byte ptr cs:[gPixelPair], 0
	je	L_0F29
	dec	cx
L_0F29:
	xor	bx, bx
	mov	si, di
L_0F2D:
	mov	bl, byte ptr cs:[gPixelPair]
	or	bx, bx
	je	L_0F4A
	mov	al, byte ptr cs:[bx + gFillLeftKeep]
	mov	ah, al
	and	ah, byte ptr es:[di]
	not	al
	and	al, byte ptr cs:[gFillPattern]
	or	al, ah
	stosb
L_0F4A:
	or	cx, cx
	je	L_0F58
	mov	ah, cl
	mov	al, byte ptr cs:[gFillPattern]
	rep stosb
	mov	cl, ah
L_0F58:
	mov	bl, byte ptr cs:[gFillEndBits]
	cmp	bl, 3
	je	L_0F76
	mov	al, byte ptr cs:[bx + gFillRightKeep]
	mov	ah, al
	and	ah, byte ptr es:[di]
	not	al
	and	al, byte ptr cs:[gFillPattern]
	or	al, ah
	stosb
L_0F76:
	add	si, bp
	xor	bp, 0C050h
	mov	di, si
	dec	dx
	jns	L_0F2D
	jmp	short L_0F9F
L_0F83:
	xor	cx, cx
	mov	bx, cx
	mov	bl, byte ptr cs:[gFillEndBits]
	inc	bl
	shl	bl, 1
	shl	bl, 1
	add	byte ptr cs:[gPixelPair], bl
	mov	byte ptr cs:[gFillEndBits], 3
	jmp	short L_0F29
L_0F9F:
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

; CGA.OVL 0FA8 (export 10, ds:806E): ReadCell8x8(page, void far *dst, col, y) copies 8 rows of 2 bytes of a
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
	jge	L_0FCB
	xor	si, si
L_0FCB:
	cmp	si, SCREEN_COLS
	jl	L_0FD3
	mov	si, SCREEN_COLS - 1
L_0FD3:
	shl	si, 1
	mov	bx, word ptr [bp + 0Eh]
	cmp	bx, 0
	jge	L_0FDF
	xor	bx, bx
L_0FDF:
	cmp	bx, SCREEN_HEIGHT - 8
	jle	L_0FE8
	mov	bx, SCREEN_HEIGHT - 8
L_0FE8:
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	si, ax
	mov	cx, 8
	mov	ax, 1FFEh
	cmp	si, ODD_BANK
	jb	L_1000
	mov	ax, 0E04Eh
L_1000:
	movsw
	add	si, ax
	xor	ax, 0FFB0h
	loop	L_1000
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

; CGA.OVL 1012 (near): ax = the segment of the page after the draw page (gDrawPage + 1)
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

; CGA.OVL 1035 (near): ax = the segment of the draw page, gPageSegs[gDrawPage & 1Fh]
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

; CGA.OVL 1057 (near): ax = the segment of page ax, gPageSegs[ax & 1Fh]
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

; CGA.OVL 106E (export 11, gGetPixel): GetPixel(x, y) returns the CGA colour (0 to 3) of the draw page's pixel
GetPixel	PROC	NEAR
	push	bp
	mov	bp, sp
	push	bx
	push	cx
	push	ds
	push	si
	mov	bx, word ptr [bp + 8]
	or	bx, bx
	js	L_10B9
	cmp	bx, SCREEN_HEIGHT
	jge	L_10B9
	shl	bx, 1
	mov	si, word ptr cs:[bx + gRowOffsets]
	mov	ax, word ptr [bp + 6]
	or	ax, ax
	js	L_10B9
	cmp	ax, SCREEN_WIDTH
	jge	L_10B9
	mov	bx, ax
	shr	bx, 1
	shr	bx, 1
	add	si, bx
	and	ax, 3
	mov	bx, ax
	mov	cl, byte ptr cs:[bx + gPixelShifts]
	call	DrawPageSeg
	mov	ds, ax
	lodsb
	xor	ah, ah
	or	cl, cl
	je	L_10B5
	shr	al, cl
L_10B5:
	and	al, 3
	jmp	short L_10BB
L_10B9:
	xor	ax, ax
L_10BB:
	pop	si
	pop	ds
	pop	cx
	pop	bx
	pop	bp
	retf
GetPixel	ENDP

; CGA.OVL 10C1 (export 12, ds:8076): MirrorCell16(void far *src, void far *dst) mirrors a 16 by 16 cell of CGA
; pixels (16 rows of 4 bytes) left to right through gMirrorBytes; no call of ds:8076 was found
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
L_10DA:
	lodsw
	call	MirrorPixelWord
	mov	word ptr es:[di + 2], bx
	lodsw
	call	MirrorPixelWord
	mov	word ptr es:[di], bx
	add	di, 4
	dec	dl
	jne	L_10DA
	jmp	short L_1105
MirrorCell16	ENDP

; CGA.OVL 10F2 (near): bx = the eight pixels of ax in the other order
MirrorPixelWord	PROC	NEAR
	xor	bx, bx
	mov	bl, al
	mov	bl, byte ptr cs:[bx + gMirrorBytes]
	xchg	ah, bl
	mov	bl, byte ptr cs:[bx + gMirrorBytes]
	mov	bh, ah
	ret
L_1105:
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

; CGA.OVL 110F: LinkDriver(void far *table), as MCGA.OVL 0BE6: exchanges the slot tables (imports in, exports
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
L_112D:
	xor	bx, bx
	lodsw
	mov	dx, ax
	lodsw
	or	bx, ax
	or	bx, dx
	je	L_1141
	xchg	dx, ax
	stosw
	xchg	dx, ax
	stosw
	loop	L_112D
	jmp	short L_115A
L_1141:
	sub	si, 4
	mov	dx, word ptr es:[di]
	add	di, 4
	or	dx, dx
	je	L_115D
	mov	word ptr [si], dx
	add	si, 2
	mov	word ptr [si], es
	add	si, 2
	loop	L_112D
L_115A:
	clc
	jmp	short L_115E
L_115D:
	stc
L_115E:
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

; CGA.OVL 1168: DriverEntry, the far call the game makes at segment:0000 once the driver is loaded: returns
; bx = the driver's size in paragraphs and ax = the offset of LinkDriver
DriverEntry	PROC	NEAR
	push	bp
	mov	bp, sp
	push	cx
	push	di
	push	es
	mov	ax, 0B800h		; QUIRK 116E: es:di = B800:0000 is set up and thrown away by the pops
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

; CGA.OVL 1185 (export 13, ds:807A): SetTileSet returns at once; the dead body after it is MCGA.OVL's
SetTileSet	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
	push	ax		; QUIRK 118A: dead code, SetTileSet returns above; its body as MCGA.OVL has it
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

; CGA.OVL 11B8 (exports 15 and 16, gGetSpriteCellRle and gGetSpriteCell): GetSpriteCell(col, y, cols, rows,
; void far *map) cuts the rectangle at column col, line y out of the art page after the draw page into a CGA
; cell from MemAlloc: a 4-byte header (0, rows, cols, rows), then rows of cols * 2 bytes of CGA pixels (through
; the dither tables, which SetDitherTables builds from map when it is not null), then the same number of mask
; bytes, 11b where the source pixel is 0 (transparent). Returns dx:ax the block.
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
	mov	ax, word ptr [bp + 0Eh]
	mov	dx, word ptr [bp + 10h]
	or	ax, dx
	je	L_11D2
	mov	ax, word ptr [bp + 0Eh]
	call	SetDitherTables
L_11D2:
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
	mov	bx, word ptr [bp + 8]
	shl	bx, cl
	mov	ax, bx
	shl	bx, 1
	shl	bx, 1
	add	ax, bx
	shl	ax, cl
	add	si, ax
	xor	bx, bx
	push	dx
	push	si
	mov	bp, offset gDitherEven
	mov	ax, bp
	xor	ax, offset gDitherOdd
	mov	word ptr cs:[gOtherDitherTable], ax
L_1241:
	mov	word ptr cs:[gCellRowStart], si
	mov	al, 8
	mul	dl
	add	ax, si
	mov	word ptr cs:[gCellRowEnd], ax
	mov	cl, 4
L_1252:
	lodsw
	and	ax, 0F0Fh
	shl	al, cl
	or	al, ah
	mov	bl, al
	add	bp, bx
	mov	al, byte ptr cs:[bp]
	sub	bp, bx
	shl	al, cl
	mov	ch, al
	lodsw
	and	ax, 0F0Fh
	shl	al, cl
	or	al, ah
	mov	bl, al
	add	bp, bx
	mov	al, byte ptr cs:[bp]
	sub	bp, bx
	or	al, ch
	stosb
	cmp	si, word ptr cs:[gCellRowEnd]
	jne	L_1252
	xor	bp, word ptr cs:[gOtherDitherTable]
	mov	si, word ptr cs:[gCellRowStart]
	add	si, SCREEN_WIDTH
	dec	dh
	jne	L_1241
	pop	si
	pop	dx
L_1298:
	mov	word ptr cs:[gCellRowStart], si
	mov	al, 8
	mul	dl
	add	ax, si
	mov	bp, ax
L_12A5:
	xor	ax, ax
	lodsb
	or	al, al
	jne	L_12AF
	or	ah, SCREEN_HEIGHT - 8
L_12AF:
	lodsb
	or	al, al
	jne	L_12B7
	or	ah, 30h
L_12B7:
	lodsb
	or	al, al
	jne	L_12BF
	or	ah, 0Ch
L_12BF:
	lodsb
	or	al, al
	jne	L_12C7
	or	ah, 3
L_12C7:
	mov	al, ah
	stosb
	cmp	bp, si
	jne	L_12A5
	mov	si, word ptr cs:[gCellRowStart]
	add	si, SCREEN_WIDTH
	dec	dh
	jne	L_1298
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

; CGA.OVL 12EC (export 17, ds:808A): DrawMaskedCell16 returns at once; the dead body after it is MCGA.OVL's
DrawMaskedCell16	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
	push	ax		; QUIRK 12F1: dead code, DrawMaskedCell16 returns above; its body as MCGA.OVL has it
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
L_1330:
	mov	cx, 8
L_1333:
	mov	al, byte ptr [bx]
	mov	ah, 0FFh
	test	al, 0F0h
	je	L_133E
	and	ah, 0Fh
L_133E:
	test	al, 0Fh
	je	L_1345
	and	ah, 0F0h
L_1345:
	and	ah, byte ptr [si]
	or	al, ah
	mov	ah, al
	and	ax, 0FF0h
	stosw
	inc	si
	inc	bx
	loop	L_1333
	add	di, 130h
	dec	dl
	jne	L_1330
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

; CGA.OVL 1365 (export 18, gCopyToPlanar): nothing to do in a packed-pixel mode
CopyToPlanar	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
CopyToPlanar	ENDP

; CGA.OVL 136A (export 19, ds:8092): SwapPageWithScreen(page) exchanges a page's 16 KB with page 0; no call of
; ds:8092 was found
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
	mov	cx, 1FA0h
	cld
L_138D:
	mov	ax, word ptr es:[di]
	movsw
	mov	word ptr [si - 2], ax
	loop	L_138D
	pop	si
	pop	es
	pop	ds
	pop	di
	pop	cx
	pop	ax
	pop	bp
	retf
SwapPageWithScreen	ENDP

; CGA.OVL 139E (export 20, gDrawShapeMaskedFlipped): returns at once; the dead body after it is MCGA.OVL's
DrawShapeMaskedFlipped	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
	push	ax		; QUIRK 13A3: dead code, DrawShapeMaskedFlipped returns above; its body as MCGA.OVL has it
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
	jns	L_147D
	add	word ptr cs:[gShapeRows], ax
	jg	L_1442
	jmp	L_14F5
L_1442:
	neg	ax
	mov	dx, ax
	mov	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipLeft], ax
	push	di
	push	es
	mov	di, si
	mov	ax, word ptr cs:[gShapeSeg]
	mov	es, ax
L_1458:
	mov	cx, word ptr cs:[gSkipLeft]
L_145D:
	xor	ax, ax
	repne scasb
	je	L_1465
	jcxz	L_146E
L_1465:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jne	L_145D
L_146E:
	dec	dx
	jne	L_1458
	mov	si, di
	pop	es
	pop	di
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gShapeY], ax
L_147D:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gShapeY]
	js	L_14F5
	je	L_14F5
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_1495
	mov	word ptr cs:[gShapeRows], ax
L_1495:
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr cs:[gShapeX]
	or	ax, ax
	jns	L_14BC
	add	word ptr cs:[gShapeWidth], ax
	neg	ax
	mov	word ptr cs:[gSkipLeft], ax
	cmp	ax, word ptr cs:[gShapeFullWidth]
	jae	L_14F5
	xor	ax, ax
	mov	word ptr cs:[gShapeX], ax
L_14BC:
	mov	word ptr cs:[gSkipRight], 0
	mov	ax, word ptr cs:[gRegionCols]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr cs:[gShapeX]
	js	L_14F5
	je	L_14F5
	cmp	ax, word ptr cs:[gShapeWidth]
	jae	L_14F8
	mov	word ptr cs:[gShapeWidth], ax
	mov	ax, word ptr cs:[gShapeFullWidth]
	sub	ax, word ptr cs:[gSkipLeft]
	sub	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipRight], ax
	jmp	short L_14F8
L_14F5:
	jmp	L_167F
L_14F8:
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
L_1528:
	mov	cx, word ptr cs:[gSkipRight]
	jcxz	L_1552
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_1537:
	xor	ax, ax
	repne scasb
	je	L_153F
	jcxz	L_1548
L_153F:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jg	L_1537
L_1548:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_1552:
	add	di, cx
	add	cx, word ptr cs:[gVisibleWidth]
	jle	L_15CF
L_155B:
	lodsb
	or	al, al
	je	L_15D1
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
L_1571:
	cmp	cx, 0
	jle	L_157B
	shl	al, 1
	dec	cx
	jmp	short L_1571
L_157B:
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
	jge	L_15A5
	mov	al, byte ptr es:[di + MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_15A5:
	cmp	word ptr cs:[gMaskLevel], 2
	jge	L_15B9
	mov	al, byte ptr es:[di + 2 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_15B9:
	cmp	word ptr cs:[gMaskLevel], 3
	jge	L_15D8
	mov	al, byte ptr es:[di + 3 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
	jmp	short L_15D8
L_15CF:
	jmp	short L_164D
L_15D1:
	jmp	short L_1646
L_15D3:
	jmp	short L_155B
L_15D5:
	jmp	L_1528
L_15D8:
	cmp	word ptr cs:[gMaskLevel], 4
	jge	L_15EC
	mov	al, byte ptr es:[di + 4 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_15EC:
	cmp	word ptr cs:[gMaskLevel], 5
	jge	L_1600
	mov	al, byte ptr es:[di + 5 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1600:
	cmp	word ptr cs:[gMaskLevel], 6
	jge	L_1614
	mov	al, byte ptr es:[di + 6 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_1614:
	mov	al, byte ptr es:[di + 7 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
	pop	ax
	pop	di
	cmp	bl, 0
	je	L_1636
	mov	es, word ptr cs:[gBackPageSeg]
	mov	bl, byte ptr es:[di]
	pop	es
	mov	byte ptr es:[di], bl
	dec	di
	jmp	short L_163F
L_1636:
	pop	es
	mov	byte ptr es:[di], al
	dec	di
	jmp	short L_163F
L_163D:
	jmp	short L_15D5
L_163F:
	pop	bx
	pop	cx
	pop	dx
	loop	L_15D3
	jmp	short L_164D
L_1646:
	lodsb
	sub	di, ax
	sub	cx, ax
	jg	L_15D3
L_164D:
	add	cx, word ptr cs:[gSkipLeft]
	jcxz	L_1677
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_165C:
	xor	ax, ax
	repne scasb
	je	L_1664
	jcxz	L_166D
L_1664:
	inc	cx
	mov	al, byte ptr es:[di]
	inc	di
	sub	cx, ax
	jne	L_165C
L_166D:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_1677:
	add	bp, LINE_BYTES
	mov	di, bp
	dec	dx
	jne	L_163D
L_167F:
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

; CGA.OVL 1689 (export 21, gDrawShapeMasked): returns at once; the dead body after it is MCGA.OVL's
DrawShapeMasked	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
	push	ax		; QUIRK 168E: dead code, DrawShapeMasked returns above; its body as MCGA.OVL has it
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
	jns	L_1768
	add	word ptr cs:[gShapeRows], ax
	jg	L_172D
	jmp	L_17E0
L_172D:
	neg	ax
	mov	dx, ax
	mov	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipLeft], ax
	push	di
	push	es
	mov	di, si
	mov	ax, word ptr cs:[gShapeSeg]
	mov	es, ax
L_1743:
	mov	cx, word ptr cs:[gSkipLeft]
L_1748:
	xor	ax, ax
	repne scasb
	je	L_1750
	jcxz	L_1759
L_1750:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jne	L_1748
L_1759:
	dec	dx
	jne	L_1743
	mov	si, di
	pop	es
	pop	di
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gShapeY], ax
L_1768:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gShapeY]
	js	L_17E0
	je	L_17E0
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_1780
	mov	word ptr cs:[gShapeRows], ax
L_1780:
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr cs:[gShapeX]
	or	ax, ax
	jns	L_17A7
	add	word ptr cs:[gShapeWidth], ax
	neg	ax
	mov	word ptr cs:[gSkipLeft], ax
	cmp	ax, word ptr cs:[gShapeFullWidth]
	jae	L_17E0
	xor	ax, ax
	mov	word ptr cs:[gShapeX], ax
L_17A7:
	mov	word ptr cs:[gSkipRight], 0
	mov	ax, word ptr cs:[gRegionCols]
	shl	ax, 1
	shl	ax, 1
	shl	ax, 1
	sub	ax, word ptr cs:[gShapeX]
	js	L_17E0
	je	L_17E0
	cmp	ax, word ptr cs:[gShapeWidth]
	jae	L_17E3
	mov	word ptr cs:[gShapeWidth], ax
	mov	ax, word ptr cs:[gShapeFullWidth]
	sub	ax, word ptr cs:[gSkipLeft]
	sub	ax, word ptr cs:[gShapeWidth]
	mov	word ptr cs:[gSkipRight], ax
	jmp	short L_17E3
L_17E0:
	jmp	L_196A
L_17E3:
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
L_1810:
	mov	cx, word ptr cs:[gSkipLeft]
	jcxz	L_183A
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_181F:
	xor	ax, ax
	repne scasb
	je	L_1827
	jcxz	L_1830
L_1827:
	mov	al, byte ptr es:[di]
	inc	di
	inc	cx
	sub	cx, ax
	jg	L_181F
L_1830:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_183A:
	sub	di, cx
	add	cx, word ptr cs:[gVisibleWidth]
	jle	L_18B9
L_1843:
	lodsb
	or	al, al
	je	L_18BB
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
L_1859:
	cmp	cx, 0
	jle	L_1863
	shl	al, 1
	dec	cx
	jmp	short L_1859
L_1863:
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
	jge	L_188D
	mov	al, byte ptr es:[di + MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_188D:
	cmp	word ptr cs:[gMaskLevel], 2
	jge	L_18A1
	mov	al, byte ptr es:[di + 2 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_18A1:
	cmp	word ptr cs:[gMaskLevel], 3
	jge	L_18BD
	mov	al, byte ptr es:[di + 3 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
	jmp	short L_18BD
	jmp	short L_190A		; QUIRK 18B7: dead code, the jmp above skips it
L_18B9:
	jmp	short L_1938
L_18BB:
	jmp	short L_1931
L_18BD:
	cmp	word ptr cs:[gMaskLevel], 4
	jge	L_18D1
	mov	al, byte ptr es:[di + 4 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_18D1:
	cmp	word ptr cs:[gMaskLevel], 5
	jge	L_18EA
	mov	al, byte ptr es:[di + 5 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
	jmp	short L_18EA
L_18E7:
	jmp	L_1843
L_18EA:
	cmp	word ptr cs:[gMaskLevel], 6
	jge	L_18FE
	mov	al, byte ptr es:[di + 6 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_18FE:
	mov	al, byte ptr es:[di + 7 * MASK_PLANE]
	and	al, byte ptr cs:[gMaskBit]
	or	bl, al
L_190A:
	pop	ax
	pop	di
	cmp	bl, 0
	je	L_1920
	mov	es, word ptr cs:[gBackPageSeg]
	mov	bl, byte ptr es:[di]
	pop	es
	mov	byte ptr es:[di], bl
	inc	di
	jmp	short L_1925
L_1920:
	pop	es
	mov	byte ptr es:[di], al
	inc	di
L_1925:
	pop	bx
	pop	cx
	pop	dx
	jmp	short L_192D
L_192A:
	jmp	L_1810
L_192D:
	loop	L_18E7
	jmp	short L_1938
L_1931:
	lodsb
	add	di, ax
	sub	cx, ax
	jg	L_18E7
L_1938:
	add	cx, word ptr cs:[gSkipRight]
	jcxz	L_1962
	mov	ax, ds
	mov	ds, bx
	mov	es, ax
	xchg	si, di
L_1947:
	xor	ax, ax
	repne scasb
	je	L_194F
	jcxz	L_1958
L_194F:
	inc	cx
	mov	al, byte ptr es:[di]
	inc	di
	sub	cx, ax
	jne	L_1947
L_1958:
	mov	ax, es
	mov	es, bx
	mov	ds, ax
	xchg	si, di
	xor	ax, ax
L_1962:
	add	bp, LINE_BYTES
	mov	di, bp
	dec	dx
	jne	L_192A
L_196A:
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

; CGA.OVL 1974 (export 22, gPutPixel): PutPixel(x, y, colour) sets the draw page's pixel to colour AND 3; off
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
	js	L_19C8
	cmp	bx, SCREEN_HEIGHT
	jge	L_19C8
	shl	bx, 1
	mov	di, word ptr cs:[bx + gRowOffsets]
	mov	ax, word ptr [bp + 6]
	or	ax, ax
	js	L_19C8
	cmp	ax, SCREEN_WIDTH
	jge	L_19C8
	mov	bx, ax
	and	bx, 3
	shr	ax, 1
	shr	ax, 1
	add	di, ax
	call	DrawPageSeg
	mov	ds, ax
	mov	ax, word ptr [bp + 0Ah]
	and	ax, 3
	mov	cl, byte ptr cs:[bx + gPixelShifts]
	or	cl, cl
	je	L_19BD
	shl	al, cl
L_19BD:
	mov	ah, byte ptr [di]
	and	ah, byte ptr cs:[bx + gPixelKeep]
	or	ah, al
	mov	byte ptr [di], ah
L_19C8:
	pop	di
	pop	ds
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
PutPixel	ENDP

; CGA.OVL 19CF (export 23, gRestoreDrawState): puts back the region SaveDrawState kept
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

; CGA.OVL 19F6 (export 24, gDrawShapeFlipped): DrawShape mirrored left to right (through gMirrorBytes)
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
	mov	di, ax
	mov	byte ptr cs:[gPixelShift], bl
	and	byte ptr cs:[gPixelShift], 3
	shl	byte ptr cs:[gPixelShift], 1
	sar	bx, 1
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
	mov	word ptr cs:[gVisibleBytes], ax
	mov	word ptr cs:[gShapeRowBytes], ax
	lodsb
	mov	word ptr cs:[gShapeRowBytes2], ax
	mov	ax, word ptr cs:[gShapeY]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_1AB5
	neg	ax
	cmp	ax, word ptr cs:[gShapeRows]
	jb	L_1A90
L_1A8D:
	jmp	L_1B2E
L_1A90:
	mov	dx, word ptr cs:[gVisibleBytes]
	mul	dl
	add	si, ax
	mov	ax, word ptr cs:[gShapeRows]
	add	ax, word ptr cs:[gShapeY]
	sub	ax, word ptr cs:[gRegionTop]
	js	L_1A8D
	mov	word ptr cs:[gShapeRows], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gShapeY], ax
L_1AB5:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gShapeY]
	js	L_1B2E
	je	L_1B2E
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_1ACD
	mov	word ptr cs:[gShapeRows], ax
L_1ACD:
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr cs:[gShapeX]
	or	ax, ax
	jns	L_1AFF
	add	word ptr cs:[gVisibleBytes], ax
	neg	ax
	cmp	ax, word ptr cs:[gShapeRowBytes]
	jae	L_1B2E
	jne	L_1AF4
	cmp	byte ptr cs:[gBlitX], 0
	je	L_1B2E
L_1AF4:
	xor	ax, ax
	mov	word ptr cs:[gShapeX], ax
	inc	word ptr cs:[gSkipLeft]
L_1AFF:
	mov	ax, word ptr cs:[gRegionCols]
	shl	ax, 1
	sub	ax, word ptr cs:[gShapeX]
	js	L_1B2E
	je	L_1B2E
	mov	word ptr cs:[gSkipRight], 0
	cmp	ax, word ptr cs:[gVisibleBytes]
	ja	L_1B31
	add	si, word ptr cs:[gVisibleBytes]
	sub	si, ax
	mov	word ptr cs:[gVisibleBytes], ax
	inc	word ptr cs:[gSkipRight]
	jmp	short L_1B31
L_1B2E:
	jmp	L_1BD4
L_1B31:
	mov	bx, word ptr cs:[gShapeY]
	mov	word ptr cs:[gRowParity], bx
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	ax, word ptr cs:[gShapeX]
	add	di, ax
	mov	ax, ODD_BANK
	sub	ax, word ptr cs:[gVisibleBytes]
	mov	word ptr cs:[gShapeY], ax
	mov	ax, word ptr cs:[gVisibleBytes]
	sub	ax, LINE_BYTES
	add	ax, ODD_BANK
	mov	word ptr cs:[gShapeX], ax
	dec	si
	mov	bp, si
	mov	al, byte ptr cs:[gShapeRowBytes]
	mov	dl, byte ptr cs:[gShapeRowBytes2]
	mul	dl
	add	bp, ax
	xor	bx, bx
	cmp	byte ptr cs:[gPixelShift], 0
	je	L_1B7F
	jmp	short L_1BDE
L_1B7F:
	mov	cx, word ptr cs:[gVisibleBytes]
	add	si, cx
	add	bp, cx
L_1B88:
	mov	bl, byte ptr ds:[bp]
	mov	ah, byte ptr cs:[bx + gMirrorBytes]
	mov	bl, byte ptr [si]
	mov	al, byte ptr cs:[bx + gMirrorBytes]
	and	ah, byte ptr es:[di]
	or	al, ah
	stosb
	dec	si
	dec	bp
	loop	L_1B88
	add	si, word ptr cs:[gShapeRowBytes]
	add	bp, word ptr cs:[gShapeRowBytes]
	inc	word ptr cs:[gRowParity]
	test	word ptr cs:[gRowParity], 1
	je	L_1BC8
	add	di, word ptr cs:[gShapeY]
	dec	word ptr cs:[gShapeRows]
	jne	L_1B7F
	je	L_1BD4
L_1BC8:
	sub	di, word ptr cs:[gShapeX]
	dec	word ptr cs:[gShapeRows]
	jne	L_1B7F
L_1BD4:
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
L_1BDE:
	mov	cx, word ptr cs:[gVisibleBytes]
	add	si, cx
	add	bp, cx
	mov	ch, cl
	mov	cl, byte ptr cs:[gPixelShift]
	mov	bx, offset gMirrorBytes
	cmp	word ptr cs:[gSkipLeft], 0
	je	L_1BFC
	call	DrawMirroredEdge
L_1BFC:
	or	ch, ch
	je	L_1C32
L_1C00:
	mov	al, byte ptr [si]
	mov	ah, byte ptr ds:[bp]
	mov	bx, offset gMirrorBytes
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
	je	L_1C5B
L_1C26:
	or	ax, bx
	mov	word ptr es:[di], ax
L_1C2B:
	dec	si
	dec	bp
	inc	di
	dec	ch
	jne	L_1C00
L_1C32:
	add	si, word ptr cs:[gShapeRowBytes]
	add	bp, word ptr cs:[gShapeRowBytes]
	mov	ax, word ptr cs:[gRowParity]
	and	ax, 1
	xor	ax, 1
	mov	word ptr cs:[gRowParity], ax
	je	L_1C6A
	add	di, word ptr cs:[gShapeY]
	dec	word ptr cs:[gShapeRows]
	jne	L_1BDE
L_1C58:
	jmp	L_1BD4
L_1C5B:
	cmp	word ptr cs:[gSkipRight], 0
	je	L_1C26
	or	al, bl
	mov	byte ptr es:[di], al
	jmp	short L_1C2B
L_1C6A:
	sub	di, word ptr cs:[gShapeX]
	dec	word ptr cs:[gShapeRows]
	je	L_1C58
	jmp	L_1BDE
DrawShapeFlipped	ENDP

; CGA.OVL 1C79 (near): DrawShapeFlipped's partial byte at an edge: the mirrored pixels of [si + 1] under the
; mask [bp + 1], shifted by cl, into es:[di]
DrawMirroredEdge	PROC	NEAR
	mov	al, byte ptr [si + 1]
	mov	ah, byte ptr ds:[bp + 1]
	mov	bx, offset gMirrorBytes
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

; CGA.OVL 1C9D (export 25, gSaveDrawState): keeps the region and sets it to the whole screen (0, 40, 0, 200)
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

; CGA.OVL 1CE0 (export 14, gSaveRectFromPage): SaveRect from the page gDrawPage names instead of page 0
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
	pop	bp		; QUIRK 1CFD: dead code, the jmp above skips it
	retf
SaveRectFromPage	ENDP

; CGA.OVL 1CFF (export 26, gSaveRect): SaveRect(col, y, cols, rows, void far *buf) copies a rectangle of page 0
; (cols * 2 bytes a row) into buf; returns the bytes written
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
	jge	L_1D1D
	xor	si, si
L_1D1D:
	cmp	si, SCREEN_COLS
	jl	L_1D25
	mov	si, SCREEN_COLS - 1
L_1D25:
	shl	si, 1
	mov	dx, word ptr [bp + 8]
	cmp	dx, 0
	jge	L_1D31
	xor	dx, dx
L_1D31:
	cmp	dx, SCREEN_HEIGHT
	jl	L_1D3A
	mov	dx, SCREEN_HEIGHT - 1
L_1D3A:
	mov	bx, word ptr [bp + 0Ah]
	cmp	bx, 0
	jg	L_1D46
L_1D42:
	xor	ax, ax
	jmp	short L_1DB7
L_1D46:
	cmp	bx, SCREEN_COLS
	jle	L_1D4E
	mov	bx, SCREEN_COLS
L_1D4E:
	mov	cx, word ptr [bp + 0Ch]
	cmp	cx, 0
	jle	L_1D42
	cmp	cx, SCREEN_HEIGHT
	jle	L_1D5F
	mov	cx, SCREEN_HEIGHT
L_1D5F:
	mov	di, word ptr [bp + 0Eh]
	push	di
	mov	es, word ptr [bp + 10h]
	cmp	bx, SCREEN_COLS
	jbe	L_1D6E
	mov	bx, SCREEN_COLS
L_1D6E:
	or	bx, bx
	jne	L_1D74
L_1D72:
	jmp	short L_1DB2
L_1D74:
	cmp	cx, SCREEN_HEIGHT
	jbe	L_1D7D
	mov	cx, SCREEN_HEIGHT
L_1D7D:
	or	cx, cx
	je	L_1D72
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
	mov	ax, ODD_BANK
	cmp	si, ODD_BANK
	jb	L_1DA4
	mov	ax, 0E050h
L_1DA4:
	push	si
	mov	cx, bx
	rep movsw
	pop	si
	add	si, ax
	xor	ax, 0C050h
	dec	dx
	jne	L_1DA4
L_1DB2:
	mov	ax, di
	pop	di
	sub	ax, di
L_1DB7:
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

; CGA.OVL 1DC2 (export 27, ds:80B2): SetLineClip(left, top, right, bottom) sets the rectangle DrawLine clips
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

; CGA.OVL 1DE5 (export 28, gSetRegion): SetRegion(left, right, top, bottom): the region the shape draws clip
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

; CGA.OVL 1E08 (export 29, gSetPalette): returns at once (a CGA palette is not loaded); the dead body after it is
; MCGA.OVL's DAC load
SetPalette	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
	push	ax		; QUIRK 1E0D: dead code, SetPalette returns above; its body as MCGA.OVL has it
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

; CGA.OVL 1E2A (export 30, ds:80BE): SetTileFormat returns at once; the dead body after it is MCGA.OVL's
SetTileFormat	PROC	NEAR
	push	bp
	mov	bp, sp
	pop	bp
	retf
	push	ax		; QUIRK 1E2F: dead code, SetTileFormat returns above; its body as MCGA.OVL has it
	push	bx
	push	cx
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gTileFormatArg3], ax
	mov	ax, word ptr [bp + 0Ch]
	mov	word ptr cs:[gTileFormatArg4], ax
	mov	ax, word ptr [bp + 6]
	mov	bx, word ptr [bp + 8]
	cmp	ax, bx
	jne	L_1E94
	cmp	ax, 3
	jge	L_1E94
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
	jmp	short L_1EC5
L_1E94:
	mov	word ptr cs:[gTileMode], 4
	mov	word ptr cs:[gTileParas], 2
	mov	word ptr cs:[gTileRowBytes], 4
	mov	word ptr cs:[gTileRows], 8
	mov	word ptr cs:[gTileDstSkip], 138h
	mov	word ptr cs:[gTileReqWidth], ax
	mov	word ptr cs:[gTileReqHeight], bx
	mov	ax, 1
	mov	bx, ax
L_1EC5:
	mov	word ptr cs:[gTileWidthCells], ax
	mov	word ptr cs:[gTileHeightCells], bx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
SetTileFormat	ENDP

; CGA.OVL 1ED3 (export 31, gDrawShape): DrawShape(page, void far *shape, x, y) draws a CGA cell (GetSpriteCell's
; format: header, pixels, masks) at pixel (x, y), clipped to the region: each byte is the screen's AND the mask,
; OR the pixels; at an x that is not a multiple of 4 the bytes are shifted
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
	mov	di, ax
	mov	byte ptr [bp - 16h], bl
	and	byte ptr [bp - 16h], 3
	shl	byte ptr [bp - 16h], 1
	sar	bx, 1
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
	mov	word ptr [bp - 0Ah], ax
	mov	word ptr [bp - 8], ax
	mov	ax, word ptr [bp - 12h]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_1F72
	neg	ax
	cmp	ax, word ptr [bp - 0Ch]
	jb	L_1F54
L_1F52:
	jmp	short L_1FCB
L_1F54:
	mov	dx, word ptr [bp - 0Ah]
	mul	dl
	add	si, ax
	mov	ax, word ptr [bp - 0Ch]
	add	ax, word ptr [bp - 12h]
	sub	ax, word ptr cs:[gRegionTop]
	js	L_1F52
	mov	word ptr [bp - 0Ch], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr [bp - 12h], ax
L_1F72:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr [bp - 12h]
	js	L_1FCB
	je	L_1FCB
	cmp	ax, word ptr [bp - 0Ch]
	jae	L_1F85
	mov	word ptr [bp - 0Ch], ax
L_1F85:
	mov	word ptr [bp - 6], 0
	mov	ax, word ptr [bp - 10h]
	or	ax, ax
	jns	L_1FAD
	add	word ptr [bp - 0Ah], ax
	neg	ax
	add	si, ax
	cmp	ax, word ptr [bp - 8]
	jae	L_1FCB
	jne	L_1FA5
	cmp	byte ptr [bp - 16h], 0
	je	L_1FCB
L_1FA5:
	xor	ax, ax
	mov	word ptr [bp - 10h], ax
	inc	word ptr [bp - 6]
L_1FAD:
	mov	ax, word ptr [bp - 0Eh]
	shl	ax, 1
	sub	ax, word ptr [bp - 10h]
	js	L_1FCB
	je	L_1FCB
	mov	word ptr [bp - 4], 0
	cmp	ax, word ptr [bp - 0Ah]
	ja	L_1FCE
	mov	word ptr [bp - 0Ah], ax
	inc	word ptr [bp - 4]
	jmp	short L_1FCE
L_1FCB:
	jmp	L_20D8
L_1FCE:
	mov	ax, word ptr [bp - 12h]
	mov	word ptr [bp - 18h], ax
	xchg	bx, ax
	shl	bx, 1
	mov	bx, word ptr cs:[bx + gRowOffsets]
	add	bx, word ptr [bp - 10h]
	xchg	bx, ax
	add	di, ax
	mov	ax, ODD_BANK
	sub	ax, word ptr [bp - 0Ah]
	mov	word ptr [bp - 12h], ax
	mov	ax, word ptr [bp - 0Ah]
	sub	ax, LINE_BYTES
	add	ax, ODD_BANK
	mov	word ptr [bp - 10h], ax
	mov	bx, si
	mov	al, byte ptr [bp - 8]
	mov	dl, byte ptr [bp - 2]
	mul	dl
	add	bx, ax
	cmp	byte ptr [bp - 16h], 0
	je	L_200E
	mov	dx, word ptr [bp - 14h]
	jmp	short L_2047
L_200E:
	mov	cx, word ptr [bp - 0Ah]
L_2011:
	mov	ah, byte ptr [bx]
	inc	bx
	and	ah, byte ptr es:[di]
	lodsb
	or	al, ah
	stosb
	loop	L_2011
	add	si, word ptr [bp - 8]
	sub	si, word ptr [bp - 0Ah]
	add	bx, word ptr [bp - 8]
	sub	bx, word ptr [bp - 0Ah]
	inc	byte ptr [bp - 18h]
	test	byte ptr [bp - 18h], 1
	je	L_203C
	add	di, word ptr [bp - 12h]
	dec	byte ptr [bp - 0Ch]
	jne	L_200E
	je	L_2044
L_203C:
	sub	di, word ptr [bp - 10h]
	dec	byte ptr [bp - 0Ch]
	jne	L_200E
L_2044:
	jmp	L_20D8
L_2047:
	mov	cx, word ptr [bp - 0Ah]
	mov	ch, cl
	mov	cl, byte ptr [bp - 16h]
	mov	word ptr [bp - 1Ah], ds
	mov	ds, dx
	cmp	byte ptr [bp - 6], 0
	je	L_2073
	mov	dx, word ptr es:[di]
	mov	ah, byte ptr [bx - 1]
	mov	al, 0FFh
	stc
	rcr	ax, cl
	and	dl, al
	mov	ah, byte ptr [si - 1]
	xor	al, al
	shr	ax, cl
	or	dl, al
	mov	byte ptr es:[di], dl
L_2073:
	or	ch, ch
	je	L_209E
L_2077:
	mov	dx, word ptr es:[di]
	mov	ah, byte ptr [bx]
	inc	bx
	mov	al, 0FFh
	stc
	rcr	ax, cl
	xchg	ah, al
	and	dx, ax
	lodsb
	mov	ah, al
	xor	al, al
	shr	ax, cl
	xchg	ah, al
	cmp	ch, 1
	je	L_20C2
L_2094:
	or	dx, ax
	mov	word ptr es:[di], dx
	inc	di
	dec	ch
	jne	L_2077
L_209E:
	mov	dx, ds
	mov	ds, word ptr [bp - 1Ah]
	add	si, word ptr [bp - 8]
	sub	si, word ptr [bp - 0Ah]
	add	bx, word ptr [bp - 8]
	sub	bx, word ptr [bp - 0Ah]
	inc	byte ptr [bp - 18h]
	test	byte ptr [bp - 18h], 1
	je	L_20D0
	add	di, word ptr [bp - 12h]
	dec	byte ptr [bp - 0Ch]
	je	L_20D8
L_20C0:
	jmp	short L_2047
L_20C2:
	cmp	byte ptr [bp - 4], 0
	je	L_2094
	or	dl, al
	mov	byte ptr es:[di], dl
	inc	di
	jmp	short L_209E
L_20D0:
	sub	di, word ptr [bp - 10h]
	dec	byte ptr [bp - 0Ch]
	jne	L_20C0
L_20D8:
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
	pop	bp		; QUIRK 20E4: dead code, the jmp above skips it
	retf
DrawShape	ENDP

; CGA.OVL 20E6 (near): SetDitherTables, dx:ax a 32-byte colour map: map[c] and map[16 + c] are the two CGA
; colours of colour c. Builds gDitherEven (map[hi] SHL 2 OR map[16 + lo]) and gDitherOdd (map[16 + hi] SHL 2 OR
; map[lo]) for every pixel pair, so that the two colours alternate in a checkerboard. CopyPage and GetSpriteCell
; call it with their map argument.
SetDitherTables	PROC	NEAR
	push	bp
	mov	bp, sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	ds
	push	bp
	mov	bp, ax
	mov	ds, dx
	mov	si, offset gDitherEven
	mov	di, offset gDitherOdd
	xor	cl, cl
	xor	dx, dx
L_20FF:
	xor	ax, ax
	mov	al, cl
	shr	al, 1
	shr	al, 1
	shr	al, 1
	shr	al, 1
	mov	dl, al
	mov	al, cl
	and	al, 0Fh
	mov	bx, bp
	add	bx, dx
	mov	ch, byte ptr [bx]
	sub	bx, dx
	shl	ch, 1
	shl	ch, 1
	add	bx, ax
	or	ch, byte ptr [bx + 10h]
	sub	bx, ax
	mov	byte ptr cs:[si], ch
	inc	si
	add	bx, dx
	mov	ch, byte ptr [bx + 10h]
	sub	bx, dx
	shl	ch, 1
	shl	ch, 1
	add	bx, ax
	or	ch, byte ptr [bx]
	mov	byte ptr cs:[di], ch
	inc	di
	inc	cl
	jne	L_20FF
	pop	bp
	pop	ds
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	ret
SetDitherTables	ENDP

; CGA.OVL 2149 (export 32, gSaveRectSize): SaveRectSize(cols, rows) returns cols * rows * 2, the bytes SaveRect
; needs (cols clamped to 1 to 40, rows to 1 to 200)
SaveRectSize	PROC	NEAR
	push	bp
	mov	bp, sp
	push	cx
	push	dx
	mov	ax, word ptr [bp + 6]
	cmp	ax, 1
	jge	L_2159
	mov	ax, 1
L_2159:
	cmp	ax, SCREEN_COLS
	jle	L_2161
	mov	ax, SCREEN_COLS
L_2161:
	mov	dx, word ptr [bp + 8]
	cmp	dx, 1
	jge	L_216C
	mov	dx, 1
L_216C:
	cmp	dx, SCREEN_HEIGHT
	jle	L_2175
	mov	dx, SCREEN_HEIGHT
L_2175:
	mul	dx
	shl	ax, 1
	pop	dx
	pop	cx
	pop	bp
	retf
SaveRectSize	ENDP

; cs:217D: DrawGlyph's and XorRect's state
gBlitEvenSkip	dw	0		; screen bytes from the end of an even row to the next row
gBlitOddSkip	dw	0		; and from an odd row
	db	4 dup (0)		; 2181: not used
gBlitBytes	dw	0
gBlitWidth	db	0
gBlitRows	db	0
gBlitX	db	0
gBlitColour	db	0
gBlitBack	db	0
gCgaColourOf	db	0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3	; the CGA colour of each of the 16 colours (c AND 3)
gColourPatterns	dw	0, 5555h, 0AAAAh, 0FFFFh	; a CGA colour in all eight pixels of a word
gTextPattern	dw	0
gBackPattern	dw	0
gTextLeft	dw	0
gLineHeight	dw	0
gCharWidth	dw	0
gLeftMask	db	0
gRightMask	db	0
gGlyphBytes	db	0
gFontSeg	dw	0

; cs:21B3: for each width of 1 to 24 pixels, a set of eight bit masks, one per starting bit, MSB first:
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

; cs:258B: each byte of a glyph row with every bit doubled, a word of eight CGA pixels, high byte first
; (bit k of the byte becomes bits 2k and 2k + 1 of the word)
gBitPairs LABEL WORD
	db	0h, 0h, 0h, 3h, 0h, 0Ch, 0h, 0Fh, 0h, 30h, 0h, 33h, 0h, 3Ch, 0h, 3Fh
	db	0h, 0C0h, 0h, 0C3h, 0h, 0CCh, 0h, 0CFh, 0h, 0F0h, 0h, 0F3h, 0h, 0FCh, 0h, 0FFh
	db	3h, 0h, 3h, 3h, 3h, 0Ch, 3h, 0Fh, 3h, 30h, 3h, 33h, 3h, 3Ch, 3h, 3Fh
	db	3h, 0C0h, 3h, 0C3h, 3h, 0CCh, 3h, 0CFh, 3h, 0F0h, 3h, 0F3h, 3h, 0FCh, 3h, 0FFh
	db	0Ch, 0h, 0Ch, 3h, 0Ch, 0Ch, 0Ch, 0Fh, 0Ch, 30h, 0Ch, 33h, 0Ch, 3Ch, 0Ch, 3Fh
	db	0Ch, 0C0h, 0Ch, 0C3h, 0Ch, 0CCh, 0Ch, 0CFh, 0Ch, 0F0h, 0Ch, 0F3h, 0Ch, 0FCh, 0Ch, 0FFh
	db	0Fh, 0h, 0Fh, 3h, 0Fh, 0Ch, 0Fh, 0Fh, 0Fh, 30h, 0Fh, 33h, 0Fh, 3Ch, 0Fh, 3Fh
	db	0Fh, 0C0h, 0Fh, 0C3h, 0Fh, 0CCh, 0Fh, 0CFh, 0Fh, 0F0h, 0Fh, 0F3h, 0Fh, 0FCh, 0Fh, 0FFh
	db	30h, 0h, 30h, 3h, 30h, 0Ch, 30h, 0Fh, 30h, 30h, 30h, 33h, 30h, 3Ch, 30h, 3Fh
	db	30h, 0C0h, 30h, 0C3h, 30h, 0CCh, 30h, 0CFh, 30h, 0F0h, 30h, 0F3h, 30h, 0FCh, 30h, 0FFh
	db	33h, 0h, 33h, 3h, 33h, 0Ch, 33h, 0Fh, 33h, 30h, 33h, 33h, 33h, 3Ch, 33h, 3Fh
	db	33h, 0C0h, 33h, 0C3h, 33h, 0CCh, 33h, 0CFh, 33h, 0F0h, 33h, 0F3h, 33h, 0FCh, 33h, 0FFh
	db	3Ch, 0h, 3Ch, 3h, 3Ch, 0Ch, 3Ch, 0Fh, 3Ch, 30h, 3Ch, 33h, 3Ch, 3Ch, 3Ch, 3Fh
	db	3Ch, 0C0h, 3Ch, 0C3h, 3Ch, 0CCh, 3Ch, 0CFh, 3Ch, 0F0h, 3Ch, 0F3h, 3Ch, 0FCh, 3Ch, 0FFh
	db	3Fh, 0h, 3Fh, 3h, 3Fh, 0Ch, 3Fh, 0Fh, 3Fh, 30h, 3Fh, 33h, 3Fh, 3Ch, 3Fh, 3Fh
	db	3Fh, 0C0h, 3Fh, 0C3h, 3Fh, 0CCh, 3Fh, 0CFh, 3Fh, 0F0h, 3Fh, 0F3h, 3Fh, 0FCh, 3Fh, 0FFh
	db	0C0h, 0h, 0C0h, 3h, 0C0h, 0Ch, 0C0h, 0Fh, 0C0h, 30h, 0C0h, 33h, 0C0h, 3Ch, 0C0h, 3Fh
	db	0C0h, 0C0h, 0C0h, 0C3h, 0C0h, 0CCh, 0C0h, 0CFh, 0C0h, 0F0h, 0C0h, 0F3h, 0C0h, 0FCh, 0C0h, 0FFh
	db	0C3h, 0h, 0C3h, 3h, 0C3h, 0Ch, 0C3h, 0Fh, 0C3h, 30h, 0C3h, 33h, 0C3h, 3Ch, 0C3h, 3Fh
	db	0C3h, 0C0h, 0C3h, 0C3h, 0C3h, 0CCh, 0C3h, 0CFh, 0C3h, 0F0h, 0C3h, 0F3h, 0C3h, 0FCh, 0C3h, 0FFh
	db	0CCh, 0h, 0CCh, 3h, 0CCh, 0Ch, 0CCh, 0Fh, 0CCh, 30h, 0CCh, 33h, 0CCh, 3Ch, 0CCh, 3Fh
	db	0CCh, 0C0h, 0CCh, 0C3h, 0CCh, 0CCh, 0CCh, 0CFh, 0CCh, 0F0h, 0CCh, 0F3h, 0CCh, 0FCh, 0CCh, 0FFh
	db	0CFh, 0h, 0CFh, 3h, 0CFh, 0Ch, 0CFh, 0Fh, 0CFh, 30h, 0CFh, 33h, 0CFh, 3Ch, 0CFh, 3Fh
	db	0CFh, 0C0h, 0CFh, 0C3h, 0CFh, 0CCh, 0CFh, 0CFh, 0CFh, 0F0h, 0CFh, 0F3h, 0CFh, 0FCh, 0CFh, 0FFh
	db	0F0h, 0h, 0F0h, 3h, 0F0h, 0Ch, 0F0h, 0Fh, 0F0h, 30h, 0F0h, 33h, 0F0h, 3Ch, 0F0h, 3Fh
	db	0F0h, 0C0h, 0F0h, 0C3h, 0F0h, 0CCh, 0F0h, 0CFh, 0F0h, 0F0h, 0F0h, 0F3h, 0F0h, 0FCh, 0F0h, 0FFh
	db	0F3h, 0h, 0F3h, 3h, 0F3h, 0Ch, 0F3h, 0Fh, 0F3h, 30h, 0F3h, 33h, 0F3h, 3Ch, 0F3h, 3Fh
	db	0F3h, 0C0h, 0F3h, 0C3h, 0F3h, 0CCh, 0F3h, 0CFh, 0F3h, 0F0h, 0F3h, 0F3h, 0F3h, 0FCh, 0F3h, 0FFh
	db	0FCh, 0h, 0FCh, 3h, 0FCh, 0Ch, 0FCh, 0Fh, 0FCh, 30h, 0FCh, 33h, 0FCh, 3Ch, 0FCh, 3Fh
	db	0FCh, 0C0h, 0FCh, 0C3h, 0FCh, 0CCh, 0FCh, 0CFh, 0FCh, 0F0h, 0FCh, 0F3h, 0FCh, 0FCh, 0FCh, 0FFh
	db	0FFh, 0h, 0FFh, 3h, 0FFh, 0Ch, 0FFh, 0Fh, 0FFh, 30h, 0FFh, 33h, 0FFh, 3Ch, 0FFh, 3Fh
	db	0FFh, 0C0h, 0FFh, 0C3h, 0FFh, 0CCh, 0FFh, 0CFh, 0FFh, 0F0h, 0FFh, 0F3h, 0FFh, 0FCh, 0FFh, 0FFh

; CGA.OVL 278B (near): DrawGlyph(ch, x, y), as MCGA.OVL 241E: each glyph bit becomes a CGA pixel through
; gBitPairs, drawn in gTextPattern (set bits) and gBackPattern (clear bits, unless the background is 0)
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
	jb	L_27BE
L_27BB:
	jmp	L_294E
L_27BE:
	mov	ax, dx
	mov	byte ptr cs:[gBlitX], al
	xor	cx, cx
	mov	cl, byte ptr es:[si + FONT_WIDTH]
	mov	byte ptr cs:[gBlitWidth], cl
	add	ax, cx
	cmp	ax, SCREEN_WIDTH
	ja	L_27BB
	mov	di, dx
	mov	dx, word ptr [bp + 8]
	cmp	dx, SCREEN_HEIGHT
	jae	L_27BB
	mov	ax, dx
	mov	byte ptr cs:[gRowParity], al
	mov	cl, byte ptr es:[si + FONT_HEIGHT]
	mov	byte ptr cs:[gBlitRows], cl
	add	ax, cx
	cmp	ax, SCREEN_HEIGHT
	ja	L_27BB
	add	si, bx
	xor	ah, ah
	mov	al, byte ptr cs:[gBlitWidth]
	dec	al
	cmp	al, 18h
	jae	L_27BB
	mov	bp, ax
	add	al, 7
	shr	al, 1
	shr	al, 1
	shr	al, 1
	mov	byte ptr cs:[gGlyphBytes], al
	shl	bp, 1
	mov	bp, word ptr cs:[bp + gBitMaskTable]
	shr	di, 1
	shr	di, 1
	and	byte ptr cs:[gBlitX], 3
	xor	ah, ah
	mov	al, byte ptr cs:[gBlitX]
	shl	ax, 1
	add	bp, ax
	mov	bp, word ptr cs:[bp]
	mov	ax, dx
	and	ax, 3FEh
	mov	dx, SCREEN_COLS
	mul	dx
	and	word ptr cs:[gRowParity], 1
	je	L_2849
	add	ax, ODD_BANK
L_2849:
	add	di, ax
	xor	ah, ah
	mov	al, byte ptr cs:[gGlyphBytes]
	mov	word ptr cs:[gBlitBytes], ax
	shl	ax, 1
	mov	bx, ax
	mov	ax, ODD_BANK
	sub	ax, bx
	mov	word ptr cs:[gBlitEvenSkip], ax
	mov	ax, bx
	sub	ax, LINE_BYTES
	add	ax, ODD_BANK
	mov	word ptr cs:[gBlitOddSkip], ax
	call	DrawPageSeg
	mov	dx, es
	mov	es, ax
	mov	ds, dx
	mov	bh, byte ptr cs:[gBlitColour]
	mov	cl, 4
	shl	bh, cl
	or	bh, byte ptr cs:[gBlitBack]
L_2885:
	mov	ch, byte ptr cs:[gGlyphBytes]
	xor	ax, ax
	mov	bl, ah
L_288E:
	xor	al, al
	mov	ah, byte ptr [si]
	mov	cl, byte ptr cs:[gBlitX]
	shr	ax, cl
	xchg	al, ah
	or	al, bl
	mov	bl, ah
	mov	cl, bh
	push	bx
	xor	ah, ah
	mov	dx, 0
	test	cl, 0F0h
	je	L_28B9
	and	al, byte ptr cs:[bp]
	mov	bx, ax
	shl	bx, 1
	mov	dx, word ptr cs:[bx + gBitPairs]
L_28B9:
	test	cl, 0Fh
	je	L_28F6
	not	al
	and	al, byte ptr cs:[bp]
	mov	bx, ax
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gBitPairs]
	mov	bx, ax
	or	ax, dx
	not	ax
	and	ax, word ptr es:[di]
	and	bx, word ptr cs:[gBackPattern]
	and	dx, word ptr cs:[gTextPattern]
	or	ax, bx
	or	ax, dx
	pop	bx
	mov	word ptr es:[di], ax
	cmp	ch, 0
	je	L_2923
	add	di, 2
	inc	si
	inc	bp
	dec	ch
	jmp	short L_288E
L_28F6:
	mov	ax, 0
	mov	bx, ax
	or	ax, dx
	not	ax
	and	ax, word ptr es:[di]
	and	bx, word ptr cs:[gBackPattern]
	and	dx, word ptr cs:[gTextPattern]
	or	ax, bx
	or	ax, dx
	pop	bx
	mov	word ptr es:[di], ax
	cmp	ch, 0
	je	L_2923
	add	di, 2
	inc	si
	inc	bp
	dec	ch
	jmp	L_288E
L_2923:
	sub	bp, word ptr cs:[gBlitBytes]
	xor	word ptr cs:[gRowParity], 1
	je	L_293F
	add	di, word ptr cs:[gBlitEvenSkip]
	dec	byte ptr cs:[gBlitRows]
	je	L_294E
	jmp	L_2885
L_293F:
	sub	di, word ptr cs:[gBlitOddSkip]
	dec	byte ptr cs:[gBlitRows]
	je	L_294E
	jmp	L_2885
L_294E:
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

; CGA.OVL 2958 (export 33, gDrawText): DrawText(char far *text, x, y, colour, background), as MCGA.OVL's; the
; two colours become CGA patterns through gCgaColourOf and gColourPatterns
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
	jge	L_2998
	xor	bx, bx
L_2998:
	cmp	bx, SCREEN_WIDTH
	jb	L_29A1
L_299E:
	jmp	L_2A60
L_29A1:
	mov	word ptr cs:[gTextLeft], bx
	mov	cx, word ptr [bp + 0Ch]
	cmp	cx, 0
	jge	L_29B0
	xor	cx, cx
L_29B0:
	cmp	cx, SCREEN_HEIGHT
	jge	L_299E
	mov	al, byte ptr [bp + 0Eh]
	mov	byte ptr cs:[gBlitColour], al
	mov	al, byte ptr [bp + 10h]
	mov	byte ptr cs:[gBlitBack], al
	push	di
	mov	al, byte ptr cs:[gBlitColour]
	and	ax, 0Fh
	mov	di, ax
	mov	al, byte ptr cs:[di + gCgaColourOf]
	and	ax, 3
	mov	di, ax
	shl	di, 1
	mov	ax, word ptr cs:[di + gColourPatterns]
	mov	word ptr cs:[gTextPattern], ax
	mov	al, byte ptr cs:[gBlitBack]
	and	ax, 0Fh
	mov	di, ax
	mov	al, byte ptr cs:[di + gCgaColourOf]
	and	ax, 3
	mov	di, ax
	shl	di, 1
	mov	ax, word ptr cs:[di + gColourPatterns]
	mov	word ptr cs:[gBackPattern], ax
	pop	di
L_2A02:
	cmp	byte ptr es:[si], 0
	je	L_2A60
	cmp	byte ptr es:[si], 0Dh
	jne	L_2A1B
	mov	bx, word ptr cs:[gTextLeft]
	add	cx, word ptr cs:[gLineHeight]
	inc	si
	jmp	short L_2A02
L_2A1B:
	push	es
	mov	es, word ptr cs:[gFontSeg]
	xor	ah, ah
	mov	al, byte ptr es:[di + FONT_WIDTH]
	mov	word ptr cs:[gCharWidth], ax
	mov	dx, bx
	add	dx, ax
	pop	es
	cmp	dx, SCREEN_WIDTH
	jbe	L_2A41
	mov	bx, word ptr cs:[gTextLeft]
	add	cx, word ptr cs:[gLineHeight]
L_2A41:
	cmp	cx, SCREEN_HEIGHT
	jae	L_2A60
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
	jmp	short L_2A02
L_2A60:
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

; CGA.OVL 2A6A (export 8, gXorRect): XorRect(x1, y1, x2, y2, colour) XORs the CGA pattern of colour into every
; pixel of the rectangle on the draw page
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
	jge	L_2A7D
	xor	di, di
L_2A7D:
	cmp	di, SCREEN_WIDTH
	jl	L_2A86
	mov	di, SCREEN_WIDTH - 1
L_2A86:
	mov	dx, word ptr [bp + 8]
	cmp	dx, 0
	jge	L_2A90
	xor	dx, dx
L_2A90:
	cmp	dx, SCREEN_HEIGHT
	jl	L_2A99
	mov	dx, SCREEN_HEIGHT - 1
L_2A99:
	mov	ax, word ptr [bp + 0Ah]
	cmp	ax, 0
	jge	L_2AA3
	xor	ax, ax
L_2AA3:
	cmp	ax, SCREEN_WIDTH
	jl	L_2AAB
	mov	ax, SCREEN_WIDTH - 1
L_2AAB:
	cmp	ax, di
	jge	L_2AB0
	xchg	di, ax
L_2AB0:
	mov	bx, di
	and	bl, 7
	mov	byte ptr cs:[gLeftMask], bl
	mov	bh, al
	and	bh, 7
	sub	ax, di
	inc	ax
	mov	cx, ax
	cmp	ax, 8
	jle	L_2AD3
	xor	ch, ch
	mov	cl, 8
	sub	cl, bl
	add	cl, bh
	inc	cl
L_2AD3:
	mov	word ptr cs:[gBlitBytes], cx
	sub	ax, cx
	shr	ax, 1
	shr	ax, 1
	shr	ax, 1
	mov	byte ptr cs:[gBlitWidth], al
	mov	ax, word ptr [bp + 0Ch]
	cmp	ax, 0
	jge	L_2AEE
	xor	ax, ax
L_2AEE:
	cmp	ax, SCREEN_HEIGHT
	jl	L_2AF6
	mov	ax, SCREEN_HEIGHT - 1
L_2AF6:
	cmp	ax, dx
	jge	L_2AFB
	xchg	dx, ax
L_2AFB:
	mov	byte ptr cs:[gRowParity], dl
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
	mov	al, byte ptr cs:[di + gCgaColourOf]
	and	ax, 3
	mov	di, ax
	shl	di, 1
	mov	ax, word ptr cs:[di + gColourPatterns]
	mov	word ptr cs:[gTextPattern], ax
	pop	di
	shr	di, 1
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
	je	L_2B6B
	inc	bp
	mov	al, byte ptr cs:[bp]
L_2B6B:
	mov	byte ptr cs:[gRightMask], al
	mov	ax, dx
	and	ax, 3FEh
	mov	dx, SCREEN_COLS
	mul	dx
	and	word ptr cs:[gRowParity], 1
	je	L_2B84
	add	ax, ODD_BANK
L_2B84:
	add	di, ax
	xor	ah, ah
	mov	al, byte ptr cs:[gBlitWidth]
	inc	ax
	cmp	byte ptr cs:[gRightMask], 0
	je	L_2B96
	inc	ax
L_2B96:
	shl	ax, 1
	mov	bx, ax
	mov	ax, ODD_BANK
	sub	ax, bx
	mov	word ptr cs:[gBlitEvenSkip], ax
	mov	ax, bx
	sub	ax, LINE_BYTES
	add	ax, ODD_BANK
	mov	word ptr cs:[gBlitOddSkip], ax
	call	DrawPageSeg
	mov	es, ax
	mov	ch, byte ptr cs:[gBlitWidth]
L_2BB9:
	xor	bh, bh
	mov	bl, byte ptr cs:[gLeftMask]
	shl	bx, 1
	mov	ax, word ptr es:[di]
	mov	dx, word ptr cs:[gTextPattern]
	and	dx, word ptr cs:[bx + gBitPairs]
	xor	ax, dx
	mov	word ptr es:[di], ax
	add	di, 2
	or	ch, ch
	je	L_2BEC
	mov	bx, word ptr cs:[gTextPattern]
	mov	cl, ch
L_2BE2:
	xor	word ptr es:[di], bx
	add	di, 2
	dec	cl
	jne	L_2BE2
L_2BEC:
	mov	bl, byte ptr cs:[gRightMask]
	or	bl, bl
	je	L_2C0E
	xor	bh, bh
	shl	bx, 1
	mov	ax, word ptr es:[di]
	mov	dx, word ptr cs:[gTextPattern]
	and	dx, word ptr cs:[bx + gBitPairs]
	xor	ax, dx
	mov	word ptr es:[di], ax
	add	di, 2
L_2C0E:
	xor	word ptr cs:[gRowParity], 1
	je	L_2C24
	add	di, word ptr cs:[gBlitEvenSkip]
	dec	byte ptr cs:[gBlitRows]
	jne	L_2BB9
	jmp	short L_2C30
L_2C24:
	sub	di, word ptr cs:[gBlitOddSkip]
	dec	byte ptr cs:[gBlitRows]
	jne	L_2BB9
L_2C30:
	pop	es
	pop	dx
	pop	di
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	retf
XorRect	ENDP

; CGA.OVL 2C38 (export 35, gSetRemapTable): SetRemapTable(void far *colours) builds gRemapTable from four bytes
; (each a CGA colour in all four pixel positions): every byte of four pixels p0 to p3 maps to colours[p0] to
; colours[p3] in the same positions
SetRemapTable	PROC	NEAR
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
	mov	si, word ptr [bp + 6]
	mov	ds, word ptr [bp + 8]
	mov	ax, cs
	mov	es, ax
	mov	di, offset gRemapTable
	xor	ax, ax
	mov	byte ptr cs:[gRemapIndex0], al
	mov	byte ptr cs:[gRemapIndex1], al
	mov	byte ptr cs:[gRemapIndex2], al
	mov	byte ptr cs:[gRemapIndex3], al
	mov	bx, ax
	mov	dx, 0FC03h
	mov	cx, 4
L_2C6A:
	mov	bl, byte ptr cs:[gRemapIndex0]
	mov	al, byte ptr [bx + si]
	and	al, SCREEN_HEIGHT - 8
L_2C73:
	mov	bl, byte ptr cs:[gRemapIndex1]
	mov	ah, byte ptr [bx + si]
	and	ah, 30h
	and	al, SCREEN_HEIGHT - 8
	or	al, ah
L_2C81:
	mov	bl, byte ptr cs:[gRemapIndex2]
	mov	ah, byte ptr [bx + si]
	and	ah, 0Ch
	and	al, 0F0h
	or	al, ah
L_2C8F:
	mov	bl, byte ptr cs:[gRemapIndex3]
	mov	ah, byte ptr [bx + si]
	and	ah, dl
	and	al, dh
	or	al, ah
	stosb
	inc	byte ptr cs:[gRemapIndex3]
	cmp	byte ptr cs:[gRemapIndex3], cl
	jne	L_2C8F
	mov	byte ptr cs:[gRemapIndex3], ch
	inc	byte ptr cs:[gRemapIndex2]
	cmp	byte ptr cs:[gRemapIndex2], cl
	jne	L_2C81
	mov	byte ptr cs:[gRemapIndex2], ch
	inc	byte ptr cs:[gRemapIndex1]
	cmp	byte ptr cs:[gRemapIndex1], cl
	jne	L_2C73
	mov	byte ptr cs:[gRemapIndex1], ch
	inc	byte ptr cs:[gRemapIndex0]
	cmp	byte ptr cs:[gRemapIndex0], cl
	jne	L_2C6A
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
SetRemapTable	ENDP

; CGA.OVL 2CE6 (export 37, gDrawShapeFlippedRemapped): DrawShapeRemapped mirrored left to right
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
	mov	si, word ptr [bp + 8]
	mov	ax, word ptr [bp + 0Ah]
	mov	word ptr cs:[gShapeSeg], ax
	mov	bx, word ptr [bp + 0Ch]
	mov	cx, word ptr [bp + 0Eh]
	mov	word ptr cs:[gShapeY], cx
	mov	ax, word ptr cs:[gRegionLeft]
	shl	ax, 1
	mov	di, ax
	mov	byte ptr cs:[gPixelShift], bl
	and	byte ptr cs:[gPixelShift], 3
	shl	byte ptr cs:[gPixelShift], 1
	sar	bx, 1
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
	mov	word ptr cs:[gVisibleBytes], ax
	mov	word ptr cs:[gShapeRowBytes], ax
	lodsb
	mov	word ptr cs:[gShapeRowBytes2], ax
	mov	ax, word ptr cs:[gShapeY]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_2DA5
	neg	ax
	cmp	ax, word ptr cs:[gShapeRows]
	jb	L_2D80
L_2D7D:
	jmp	L_2E1E
L_2D80:
	mov	dx, word ptr cs:[gVisibleBytes]
	mul	dl
	add	si, ax
	mov	ax, word ptr cs:[gShapeRows]
	add	ax, word ptr cs:[gShapeY]
	sub	ax, word ptr cs:[gRegionTop]
	js	L_2D7D
	mov	word ptr cs:[gShapeRows], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr cs:[gShapeY], ax
L_2DA5:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr cs:[gShapeY]
	js	L_2E1E
	je	L_2E1E
	cmp	ax, word ptr cs:[gShapeRows]
	jae	L_2DBD
	mov	word ptr cs:[gShapeRows], ax
L_2DBD:
	mov	word ptr cs:[gSkipLeft], 0
	mov	ax, word ptr cs:[gShapeX]
	or	ax, ax
	jns	L_2DEF
	add	word ptr cs:[gVisibleBytes], ax
	neg	ax
	cmp	ax, word ptr cs:[gShapeRowBytes]
	jae	L_2E1E
	jne	L_2DE4
	cmp	byte ptr cs:[gBlitX], 0
	je	L_2E1E
L_2DE4:
	xor	ax, ax
	mov	word ptr cs:[gShapeX], ax
	inc	word ptr cs:[gSkipLeft]
L_2DEF:
	mov	ax, word ptr cs:[gRegionCols]
	shl	ax, 1
	sub	ax, word ptr cs:[gShapeX]
	js	L_2E1E
	je	L_2E1E
	mov	word ptr cs:[gSkipRight], 0
	cmp	ax, word ptr cs:[gVisibleBytes]
	ja	L_2E21
	add	si, word ptr cs:[gVisibleBytes]
	sub	si, ax
	mov	word ptr cs:[gVisibleBytes], ax
	inc	word ptr cs:[gSkipRight]
	jmp	short L_2E21
L_2E1E:
	jmp	L_2EC9
L_2E21:
	mov	bx, word ptr cs:[gShapeY]
	mov	word ptr cs:[gRowParity], bx
	shl	bx, 1
	mov	ax, word ptr cs:[bx + gRowOffsets]
	add	ax, word ptr cs:[gShapeX]
	add	di, ax
	mov	ax, ODD_BANK
	sub	ax, word ptr cs:[gVisibleBytes]
	mov	word ptr cs:[gShapeY], ax
	mov	ax, word ptr cs:[gVisibleBytes]
	sub	ax, LINE_BYTES
	add	ax, ODD_BANK
	mov	word ptr cs:[gShapeX], ax
	dec	si
	mov	bp, si
	mov	al, byte ptr cs:[gShapeRowBytes]
	mov	dl, byte ptr cs:[gShapeRowBytes2]
	mul	dl
	add	bp, ax
	xor	bx, bx
	cmp	byte ptr cs:[gPixelShift], 0
	je	L_2E6F
	jmp	short L_2ED3
L_2E6F:
	mov	cx, word ptr cs:[gVisibleBytes]
	add	si, cx
	add	bp, cx
L_2E78:
	mov	bl, byte ptr ds:[bp]
	mov	ah, byte ptr cs:[bx + gMirrorBytes]
	mov	bl, byte ptr [si]
	mov	bl, byte ptr cs:[bx + gRemapTable]
	mov	al, byte ptr cs:[bx + gMirrorBytes]
	and	ah, byte ptr es:[di]
	or	al, ah
	stosb
	dec	si
	dec	bp
	loop	L_2E78
	add	si, word ptr cs:[gShapeRowBytes]
	add	bp, word ptr cs:[gShapeRowBytes]
	inc	word ptr cs:[gRowParity]
	test	word ptr cs:[gRowParity], 1
	je	L_2EBD
	add	di, word ptr cs:[gShapeY]
	dec	word ptr cs:[gShapeRows]
	jne	L_2E6F
	je	L_2EC9
L_2EBD:
	sub	di, word ptr cs:[gShapeX]
	dec	word ptr cs:[gShapeRows]
	jne	L_2E6F
L_2EC9:
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
L_2ED3:
	mov	cx, word ptr cs:[gVisibleBytes]
	add	si, cx
	add	bp, cx
	mov	ch, cl
	mov	cl, byte ptr cs:[gPixelShift]
	mov	bx, offset gMirrorBytes
	cmp	word ptr cs:[gSkipLeft], 0
	je	L_2EF1
	call	DrawRemappedMirroredEdge
L_2EF1:
	or	ch, ch
	je	L_2F2C
L_2EF5:
	mov	al, byte ptr [si]
	mov	bx, offset gRemapTable
	xlat	byte ptr cs:[bx]
	mov	ah, byte ptr ds:[bp]
	mov	bx, offset gMirrorBytes
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
	je	L_2F55
L_2F20:
	or	ax, bx
	mov	word ptr es:[di], ax
L_2F25:
	dec	si
	dec	bp
	inc	di
	dec	ch
	jne	L_2EF5
L_2F2C:
	add	si, word ptr cs:[gShapeRowBytes]
	add	bp, word ptr cs:[gShapeRowBytes]
	mov	ax, word ptr cs:[gRowParity]
	and	ax, 1
	xor	ax, 1
	mov	word ptr cs:[gRowParity], ax
	je	L_2F64
	add	di, word ptr cs:[gShapeY]
	dec	word ptr cs:[gShapeRows]
	jne	L_2ED3
L_2F52:
	jmp	L_2EC9
L_2F55:
	cmp	word ptr cs:[gSkipRight], 0
	je	L_2F20
	or	al, bl
	mov	byte ptr es:[di], al
	jmp	short L_2F25
L_2F64:
	sub	di, word ptr cs:[gShapeX]
	dec	word ptr cs:[gShapeRows]
	je	L_2F52
	jmp	L_2ED3
DrawShapeFlippedRemapped	ENDP

; CGA.OVL 2F73 (near): DrawShapeFlippedRemapped's partial byte at an edge, as DrawMirroredEdge with the pixels
; remapped
DrawRemappedMirroredEdge	PROC	NEAR
	mov	al, byte ptr [si + 1]
	mov	bx, offset gRemapTable
	xlat	byte ptr cs:[bx]
	mov	ah, byte ptr ds:[bp + 1]
	mov	bx, offset gMirrorBytes
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
DrawRemappedMirroredEdge	ENDP

gRemapShift	dw	0		; DrawShapeRemapped's bit shift, kept inside the code (the ret above ends DrawRemappedMirroredEdge)

; CGA.OVL 2F9E (export 36, gDrawShapeRemapped): DrawShape with the cell's pixel bytes translated through
; gRemapTable
DrawShapeRemapped	PROC	NEAR
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
	mov	di, ax
	mov	byte ptr [bp - 16h], bl
	and	byte ptr [bp - 16h], 3
	shl	byte ptr [bp - 16h], 1
	sar	bx, 1
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
	mov	word ptr [bp - 0Ah], ax
	mov	word ptr [bp - 8], ax
	mov	ax, word ptr [bp - 12h]
	sub	ax, word ptr cs:[gRegionTop]
	jns	L_303D
	neg	ax
	cmp	ax, word ptr [bp - 0Ch]
	jb	L_301F
L_301D:
	jmp	short L_309A
L_301F:
	mov	dx, word ptr [bp - 0Ah]
	mul	dl
	add	si, ax
	mov	ax, word ptr [bp - 0Ch]
	add	ax, word ptr [bp - 12h]
	sub	ax, word ptr cs:[gRegionTop]
	js	L_301D
	mov	word ptr [bp - 0Ch], ax
	mov	ax, word ptr cs:[gRegionTop]
	mov	word ptr [bp - 12h], ax
L_303D:
	mov	ax, word ptr cs:[gRegionBottom]
	sub	ax, word ptr [bp - 12h]
	js	L_309A
	je	L_309A
	cmp	ax, word ptr [bp - 0Ch]
	jae	L_3050
	mov	word ptr [bp - 0Ch], ax
L_3050:
	mov	word ptr [bp - 6], 0
	mov	ax, word ptr [bp - 10h]
	or	ax, ax
	jns	L_3078
	add	word ptr [bp - 0Ah], ax
	neg	ax
	add	si, ax
	cmp	ax, word ptr [bp - 8]
	jae	L_309A
	jne	L_3070
	cmp	byte ptr [bp - 16h], 0
	je	L_309A
L_3070:
	xor	ax, ax
	mov	word ptr [bp - 10h], ax
	inc	word ptr [bp - 6]
L_3078:
	mov	ax, word ptr [bp - 0Eh]
	shl	ax, 1
	sub	ax, word ptr [bp - 10h]
	js	L_309A
	je	L_309A
	mov	word ptr cs:[gRemapShift], 0
	cmp	ax, word ptr [bp - 0Ah]
	ja	L_309D
	mov	word ptr [bp - 0Ah], ax
	inc	word ptr cs:[gRemapShift]
	jmp	short L_309D
L_309A:
	jmp	L_31C9
L_309D:
	mov	ax, word ptr [bp - 12h]
	mov	word ptr [bp - 18h], ax
	xchg	bx, ax
	shl	bx, 1
	mov	bx, word ptr cs:[bx + gRowOffsets]
	add	bx, word ptr [bp - 10h]
	xchg	bx, ax
	add	di, ax
	mov	ax, ODD_BANK
	sub	ax, word ptr [bp - 0Ah]
	mov	word ptr [bp - 12h], ax
	mov	ax, word ptr [bp - 0Ah]
	sub	ax, LINE_BYTES
	add	ax, ODD_BANK
	mov	word ptr [bp - 10h], ax
	mov	bx, si
	mov	al, byte ptr [bp - 8]
	mov	dl, byte ptr [bp - 2]
	mul	dl
	add	bx, ax
	mov	dx, offset gRemapTable
	cmp	byte ptr [bp - 16h], 0
	je	L_30E0
	mov	dx, word ptr [bp - 14h]
	jmp	short L_311F
L_30E0:
	mov	cx, word ptr [bp - 0Ah]
L_30E3:
	mov	ah, byte ptr [bx]
	inc	bx
	and	ah, byte ptr es:[di]
	lodsb
	xchg	bx, dx
	xlat	byte ptr cs:[bx]
	xchg	bx, dx
	or	al, ah
	stosb
	loop	L_30E3
	add	si, word ptr [bp - 8]
	sub	si, word ptr [bp - 0Ah]
	add	bx, word ptr [bp - 8]
	sub	bx, word ptr [bp - 0Ah]
	inc	byte ptr [bp - 18h]
	test	byte ptr [bp - 18h], 1
	je	L_3114
	add	di, word ptr [bp - 12h]
	dec	byte ptr [bp - 0Ch]
	jne	L_30E0
	je	L_311C
L_3114:
	sub	di, word ptr [bp - 10h]
	dec	byte ptr [bp - 0Ch]
	jne	L_30E0
L_311C:
	jmp	L_31C9
L_311F:
	mov	cx, word ptr [bp - 0Ah]
	mov	ch, cl
	mov	cl, byte ptr [bp - 16h]
	mov	word ptr [bp - 1Ah], ds
	mov	ds, dx
	cmp	byte ptr [bp - 6], 0
	je	L_3154
	mov	dx, word ptr es:[di]
	mov	ah, byte ptr [bx - 1]
	mov	al, 0FFh
	stc
	rcr	ax, cl
	and	dl, al
	mov	al, byte ptr [si - 1]
	push	bx
	mov	bx, offset gRemapTable
	xlat	byte ptr cs:[bx]
	mov	ah, al
	pop	bx
	xor	al, al
	shr	ax, cl
	or	dl, al
	mov	byte ptr es:[di], dl
L_3154:
	or	ch, ch
	je	L_318C
	push	bp
	mov	bp, bx
	mov	bx, offset gRemapTable
L_315E:
	mov	dx, word ptr es:[di]
	mov	ah, byte ptr ds:[bp]
	inc	bp
	mov	al, 0FFh
	stc
	rcr	ax, cl
	xchg	ah, al
	and	dx, ax
	lodsb
	xlat	byte ptr cs:[bx]
	mov	ah, al
	xor	al, al
	shr	ax, cl
	xchg	ah, al
	cmp	ch, 1
	je	L_31B1
L_317F:
	or	dx, ax
	mov	word ptr es:[di], dx
	inc	di
	dec	ch
	jne	L_315E
L_3189:
	mov	bx, bp
	pop	bp
L_318C:
	mov	dx, ds
	mov	ds, word ptr [bp - 1Ah]
	add	si, word ptr [bp - 8]
	sub	si, word ptr [bp - 0Ah]
	add	bx, word ptr [bp - 8]
	sub	bx, word ptr [bp - 0Ah]
	inc	byte ptr [bp - 18h]
	test	byte ptr [bp - 18h], 1
	je	L_31C1
	add	di, word ptr [bp - 12h]
	dec	byte ptr [bp - 0Ch]
	je	L_31C9
L_31AE:
	jmp	L_311F
L_31B1:
	cmp	byte ptr cs:[gRemapShift], 0
	je	L_317F
	or	dl, al
	mov	byte ptr es:[di], dl
	inc	di
	jmp	short L_3189
L_31C1:
	sub	di, word ptr [bp - 10h]
	dec	byte ptr [bp - 0Ch]
	jne	L_31AE
L_31C9:
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
	pop	bp		; QUIRK 31D5: dead code, the jmp above skips it
	retf
DrawShapeRemapped	ENDP

DriverEnd	LABEL	BYTE		; 31D7: the end of the image, DriverEntry's size
; QUIRK 31D7: the file's header asks for one paragraph more than the image (minalloc 1), as TLINK writes for
; uninitialised bytes after the last initialised one. Nothing reads past DriverEnd, so how many there were (1 to
; 16 give this header) is not known; one is written.
	db	?
DRIVER	ENDS
	END
