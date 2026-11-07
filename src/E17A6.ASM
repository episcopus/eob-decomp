; Unit at EOB.EXE 17A6:0000 (6694 bytes). Graphics support for the CGA, Tandy, EGA, MCGA and
; VGA modes: three 256-byte translation tables, the direction helper DirectionFromPoints
; (17A6:0314), the plane-table loader SetPlaneTables (17A6:037E), the tile blitter DrawTile
; (17A6:03E7), the sprite unpackers BlitSpriteDefault (17A6:0FFA), BlitSpriteCga (17A6:11CD)
; and BlitSpriteEga (17A6:1715), the distance helper Distance (17A6:0F61), the neighbour
; search FindNeighbour (17A6:0F9D), the mask builder BuildMask (17A6:18EA) and the icon
; translator TranslateImage (17A6:197E). The code writes the sequencer map mask data port
; 3C5 and the graphics controller (3CE/3CF); it never writes the sequencer index port 3C4,
; it relies on the index its caller left. DrawTile stores through ES:DI, where ES is 0A200h
; in EGA mode (L0419) and the gArtPage far pointer otherwise (just before L041F); the
; A400/A600 segments hold the source tile planes (L04B8, L0535 and L0576). The scratch words at
; CS:0300..03E6 and CS:0FEF..0FF9 hold the saved far pointer, the port shadows and the
; current source and destination pointers. The overlays call the blitters; the call sites
; below come from the disassembly of build/re/eob-overlays/ovl03.bin and ovl04.bin, where
; the resident segment reads 00A8: overlay 3 (ovl3:0872, 08A8, 08DA, 0934) uses
; SetPlaneTables, ExpandNibbles and BuildMask, overlay 4 (ovl4:0EEC, 0F08, 0F1D, 1F6A) uses
; the three sprite unpackers and DrawTile. Names are inferred from the code and the callers;
; the exact sprite formats are not established.

        .MODEL  LARGE

        EXTRN   _GetFarPointer:FAR      ; @ 1A72:033A (E1A72.ASM)
        EXTRN   _SetFarPointer:FAR      ; @ 1A72:02FE (E1A72.ASM)

        .DATA
        EXTRN   gGraphicsChoice:WORD              ; @ ds:8102: graphics mode word
        EXTRN   gArtPage:DWORD          ; @ ds:EFC2: far pointer to the saved screen buffer
        EXTRN   gRowOffsetTableOff:WORD              ; @ ds:80CE: driver row offset table, offset (17A6:18FF)
        EXTRN   gRowOffsetTableSeg:WORD              ; @ ds:80D0: driver row offset table, segment (17A6:1913)

; @data
; the data of e17a6, ds:7FBC to ds:7FEC (48 bytes, no relocations), the original's bytes in address
; order: three tables the code indexes, no other reader in src/.

; 16 bytes, indexed at 17A6:0373 by the 4-bit mask DirectionFromPoints builds from the two
; cells' differences: the direction 0 to 7, 0FFh for none.
        PUBLIC  _gDirectionFromMask ; @ ds:7FBC
_gDirectionFromMask label byte
        db      0FFh, 06h, 02h, 0FFh, 04h, 05h, 03h, 0FFh, 00h, 07h, 01h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh

; 8 words, the offsets of a cell's eight neighbours on a 32-cell-wide map (-33, -32, -31, -1, 1,
; 31, 32, 33), which FindNeighbour adds to a cell index (17A6:0FB8).
        PUBLIC  _gNeighbourOffsets ; @ ds:7FCC
_gNeighbourOffsets label byte
        dw      0FFDFh, 0FFE0h, 0FFE1h, 0FFFFh, 0001h, 001Fh, 0020h, 0021h

; 8 words, two per direction (FindNeighbour's third argument times 4 is the byte index, 17A6:0FD3):
; the offsets of the cells two and three steps ahead (-64, -96; 2, 3; 64, 96; -2, -3).
        PUBLIC  _gAheadOffsets2And3 ; @ ds:7FDC
_gAheadOffsets2And3 label byte
        dw      0FFC0h, 0FFA0h, 0002h, 0003h, 0040h, 0060h, 0FFFEh, 0FFFDh
        .CODE

; ---- data 0000..0100
; 17A6:0000: 256-byte bit-reversal table: gBitRevTable[i] is i with its eight bits reversed
; (0 -> 00h, 1 -> 80h, 2 -> 40h, ...). No instruction in this unit reads it: the CGA sprite
; rows (17A6:128B, 12EC) translate through gTable10CD at CS:10CD instead.
gBitRevTable:
        db      00h, 80h, 40h, 0C0h, 20h, 0A0h, 60h, 0E0h, 10h, 90h, 50h, 0D0h, 30h, 0B0h, 70h, 0F0h
        db      08h, 88h, 48h, 0C8h, 28h, 0A8h, 68h, 0E8h, 18h, 98h, 58h, 0D8h, 38h, 0B8h, 78h, 0F8h
        db      04h, 84h, 44h, 0C4h, 24h, 0A4h, 64h, 0E4h, 14h, 94h, 54h, 0D4h, 34h, 0B4h, 74h, 0F4h
        db      0Ch, 8Ch, 4Ch, 0CCh, 2Ch, 0ACh, 6Ch, 0ECh, 1Ch, 9Ch, 5Ch, 0DCh, 3Ch, 0BCh, 7Ch, 0FCh
        db      02h, 82h, 42h, 0C2h, 22h, 0A2h, 62h, 0E2h, 12h, 92h, 52h, 0D2h, 32h, 0B2h, 72h, 0F2h
        db      0Ah, 8Ah, 4Ah, 0CAh, 2Ah, 0AAh, 6Ah, 0EAh, 1Ah, 9Ah, 5Ah, 0DAh, 3Ah, 0BAh, 7Ah, 0FAh
        db      06h, 86h, 46h, 0C6h, 26h, 0A6h, 66h, 0E6h, 16h, 96h, 56h, 0D6h, 36h, 0B6h, 76h, 0F6h
        db      0Eh, 8Eh, 4Eh, 0CEh, 2Eh, 0AEh, 6Eh, 0EEh, 1Eh, 9Eh, 5Eh, 0DEh, 3Eh, 0BEh, 7Eh, 0FEh
        db      01h, 81h, 41h, 0C1h, 21h, 0A1h, 61h, 0E1h, 11h, 91h, 51h, 0D1h, 31h, 0B1h, 71h, 0F1h
        db      09h, 89h, 49h, 0C9h, 29h, 0A9h, 69h, 0E9h, 19h, 99h, 59h, 0D9h, 39h, 0B9h, 79h, 0F9h
        db      05h, 85h, 45h, 0C5h, 25h, 0A5h, 65h, 0E5h, 15h, 95h, 55h, 0D5h, 35h, 0B5h, 75h, 0F5h
        db      0Dh, 8Dh, 4Dh, 0CDh, 2Dh, 0ADh, 6Dh, 0EDh, 1Dh, 9Dh, 5Dh, 0DDh, 3Dh, 0BDh, 7Dh, 0FDh
        db      03h, 83h, 43h, 0C3h, 23h, 0A3h, 63h, 0E3h, 13h, 93h, 53h, 0D3h, 33h, 0B3h, 73h, 0F3h
        db      0Bh, 8Bh, 4Bh, 0CBh, 2Bh, 0ABh, 6Bh, 0EBh, 1Bh, 9Bh, 5Bh, 0DBh, 3Bh, 0BBh, 7Bh, 0FBh
        db      07h, 87h, 47h, 0C7h, 27h, 0A7h, 67h, 0E7h, 17h, 97h, 57h, 0D7h, 37h, 0B7h, 77h, 0F7h
        db      0Fh, 8Fh, 4Fh, 0CFh, 2Fh, 0AFh, 6Fh, 0EFh, 1Fh, 9Fh, 5Fh, 0DFh, 3Fh, 0BFh, 7Fh, 0FFh
; ---- data 0100..0200
; 17A6:0100: 256-byte table: the bit reversal of gBitRevTable with each adjacent bit pair
; swapped (0 -> 00h, 1 -> 40h, 2 -> 80h, 3 -> C0h, 4 -> 10h, ...). Read by the EGA tile
; path (17A6:0E53, its cs:[bx+0100h] reads) and by the CGA tile path in DrawTile (L06D6 to
; L07AB, reached from the mode 0 dispatch). 17A6:0EEE reads no table; the EGA sprite row
; (17A6:179E) reads gTable1615.
gBitRevPairTable:
        db      00h, 40h, 80h, 0C0h, 10h, 50h, 90h, 0D0h, 20h, 60h, 0A0h, 0E0h, 30h, 70h, 0B0h, 0F0h
        db      04h, 44h, 84h, 0C4h, 14h, 54h, 94h, 0D4h, 24h, 64h, 0A4h, 0E4h, 34h, 74h, 0B4h, 0F4h
        db      08h, 48h, 88h, 0C8h, 18h, 58h, 98h, 0D8h, 28h, 68h, 0A8h, 0E8h, 38h, 78h, 0B8h, 0F8h
        db      0Ch, 4Ch, 8Ch, 0CCh, 1Ch, 5Ch, 9Ch, 0DCh, 2Ch, 6Ch, 0ACh, 0ECh, 3Ch, 7Ch, 0BCh, 0FCh
        db      01h, 41h, 81h, 0C1h, 11h, 51h, 91h, 0D1h, 21h, 61h, 0A1h, 0E1h, 31h, 71h, 0B1h, 0F1h
        db      05h, 45h, 85h, 0C5h, 15h, 55h, 95h, 0D5h, 25h, 65h, 0A5h, 0E5h, 35h, 75h, 0B5h, 0F5h
        db      09h, 49h, 89h, 0C9h, 19h, 59h, 99h, 0D9h, 29h, 69h, 0A9h, 0E9h, 39h, 79h, 0B9h, 0F9h
        db      0Dh, 4Dh, 8Dh, 0CDh, 1Dh, 5Dh, 9Dh, 0DDh, 2Dh, 6Dh, 0ADh, 0EDh, 3Dh, 7Dh, 0BDh, 0FDh
        db      02h, 42h, 82h, 0C2h, 12h, 52h, 92h, 0D2h, 22h, 62h, 0A2h, 0E2h, 32h, 72h, 0B2h, 0F2h
        db      06h, 46h, 86h, 0C6h, 16h, 56h, 96h, 0D6h, 26h, 66h, 0A6h, 0E6h, 36h, 76h, 0B6h, 0F6h
        db      0Ah, 4Ah, 8Ah, 0CAh, 1Ah, 5Ah, 9Ah, 0DAh, 2Ah, 6Ah, 0AAh, 0EAh, 3Ah, 7Ah, 0BAh, 0FAh
        db      0Eh, 4Eh, 8Eh, 0CEh, 1Eh, 5Eh, 9Eh, 0DEh, 2Eh, 6Eh, 0AEh, 0EEh, 3Eh, 7Eh, 0BEh, 0FEh
        db      03h, 43h, 83h, 0C3h, 13h, 53h, 93h, 0D3h, 23h, 63h, 0A3h, 0E3h, 33h, 73h, 0B3h, 0F3h
        db      07h, 47h, 87h, 0C7h, 17h, 57h, 97h, 0D7h, 27h, 67h, 0A7h, 0E7h, 37h, 77h, 0B7h, 0F7h
        db      0Bh, 4Bh, 8Bh, 0CBh, 1Bh, 5Bh, 9Bh, 0DBh, 2Bh, 6Bh, 0ABh, 0EBh, 3Bh, 7Bh, 0BBh, 0FBh
        db      0Fh, 4Fh, 8Fh, 0CFh, 1Fh, 5Fh, 9Fh, 0DFh, 2Fh, 6Fh, 0AFh, 0EFh, 3Fh, 7Fh, 0BFh, 0FFh
; ---- data 0200..0300
; 17A6:0200: 256-byte nibble-swap table: gNibbleSwapTable[i] is i with its two nibbles
; exchanged (0 -> 00h, 1 -> 10h, 2 -> 20h, ...). Read by BlitMaskedByte (17A6:0D20, its
; cs:[bx+0200h] reads), by the mode 1 path in DrawTile (L08DE to L0A33) and by ExpandNibbles
; (17A6:0F1F, L0F44). 17A6:0E53 reads the 0100h table instead.
gNibbleSwapTable:
        db      00h, 10h, 20h, 30h, 40h, 50h, 60h, 70h, 80h, 90h, 0A0h, 0B0h, 0C0h, 0D0h, 0E0h, 0F0h
        db      01h, 11h, 21h, 31h, 41h, 51h, 61h, 71h, 81h, 91h, 0A1h, 0B1h, 0C1h, 0D1h, 0E1h, 0F1h
        db      02h, 12h, 22h, 32h, 42h, 52h, 62h, 72h, 82h, 92h, 0A2h, 0B2h, 0C2h, 0D2h, 0E2h, 0F2h
        db      03h, 13h, 23h, 33h, 43h, 53h, 63h, 73h, 83h, 93h, 0A3h, 0B3h, 0C3h, 0D3h, 0E3h, 0F3h
        db      04h, 14h, 24h, 34h, 44h, 54h, 64h, 74h, 84h, 94h, 0A4h, 0B4h, 0C4h, 0D4h, 0E4h, 0F4h
        db      05h, 15h, 25h, 35h, 45h, 55h, 65h, 75h, 85h, 95h, 0A5h, 0B5h, 0C5h, 0D5h, 0E5h, 0F5h
        db      06h, 16h, 26h, 36h, 46h, 56h, 66h, 76h, 86h, 96h, 0A6h, 0B6h, 0C6h, 0D6h, 0E6h, 0F6h
        db      07h, 17h, 27h, 37h, 47h, 57h, 67h, 77h, 87h, 97h, 0A7h, 0B7h, 0C7h, 0D7h, 0E7h, 0F7h
        db      08h, 18h, 28h, 38h, 48h, 58h, 68h, 78h, 88h, 98h, 0A8h, 0B8h, 0C8h, 0D8h, 0E8h, 0F8h
        db      09h, 19h, 29h, 39h, 49h, 59h, 69h, 79h, 89h, 99h, 0A9h, 0B9h, 0C9h, 0D9h, 0E9h, 0F9h
        db      0Ah, 1Ah, 2Ah, 3Ah, 4Ah, 5Ah, 6Ah, 7Ah, 8Ah, 9Ah, 0AAh, 0BAh, 0CAh, 0DAh, 0EAh, 0FAh
        db      0Bh, 1Bh, 2Bh, 3Bh, 4Bh, 5Bh, 6Bh, 7Bh, 8Bh, 9Bh, 0ABh, 0BBh, 0CBh, 0DBh, 0EBh, 0FBh
        db      0Ch, 1Ch, 2Ch, 3Ch, 4Ch, 5Ch, 6Ch, 7Ch, 8Ch, 9Ch, 0ACh, 0BCh, 0CCh, 0DCh, 0ECh, 0FCh
        db      0Dh, 1Dh, 2Dh, 3Dh, 4Dh, 5Dh, 6Dh, 7Dh, 8Dh, 9Dh, 0ADh, 0BDh, 0CDh, 0DDh, 0EDh, 0FDh
        db      0Eh, 1Eh, 2Eh, 3Eh, 4Eh, 5Eh, 6Eh, 7Eh, 8Eh, 9Eh, 0AEh, 0BEh, 0CEh, 0DEh, 0EEh, 0FEh
        db      0Fh, 1Fh, 2Fh, 3Fh, 4Fh, 5Fh, 6Fh, 7Fh, 8Fh, 9Fh, 0AFh, 0BFh, 0CFh, 0DFh, 0EFh, 0FFh
; ---- data 0300..0314
gSavedFarOff dw 0                ; 0300: offset saved by 17A6:0446
gSavedFarSeg dw 0                ; 0302: segment saved by 17A6:0446
gSaveBuf db 16 dup (0)           ; 0304: backing store for the far pointer that _SetFarPointer
                                 ; redirects (the game's 16 bytes are copied in by the L0446 block
                                 ; and back by L04EE; src/E1A72.ASM, the 1A72:02FE comment). Its
                                 ; port shadows: bytes 3, 6, 11, 12 and 15 take the EGA register
                                 ; values written by DrawTile's EGA tile paths (the L0461, L04EE
                                 ; and L0586/L05AB blocks) and by SetPlaneAndWrite (17A6:0EEE).
; ---- 0314..037E
; 17A6:0314: return a direction 0..7 (0xFF when none) from two packed map cells (y * 32 + x, a
; 10-bit value): divide each by 32 into its x remainder and y quotient, compare the remainders
; and quotients, and index the 16-byte table at ds:7FBC (index 0 to 15; FF is the "no
; direction" value at indexes 0, 3, 7 and 11 to 15). Callers pass (obj->id, gPartyCell),
; gPartyCell at ds:FBCA, and use the result as an orientation (08E6:000B) and a direction
; (08E6:00C3, 08E6:0430, 08E6:0765, 0D1C:09CB).
        PUBLIC  _DirectionFromPoints              ; @ 17A6:0314
_DirectionFromPoints PROC FAR
        push bp
        mov bp, sp
        push bx
        push cx
        push dx
        push di
        push si
        mov ax, word ptr [bp+6]
        mov dl, 20h
        div dl
        mov bx, ax
        mov ax, word ptr [bp+8]
        div dl
        xor cx, cx
        xor dx, dx
        mov cl, ah
        mov dl, al
        xor ax, ax
        mov al, bh
        xor bh, bh
        mov si, 0
        sub bx, dx
        mov di, bx
        jns L0343
        neg di
L0343:
        sub cx, ax
        mov ax, cx
        jns L034B
        neg ax
L034B:
        mov dx, bx
        shl dx, 1
        cmp dx, ax
        jl L0356
        or si, 8
L0356:
        neg dx
        cmp dx, ax
        jl L035F
        or si, 4
L035F:
        mov dx, cx
        shl dx, 1
        cmp dx, di
        jl L036A
        or si, 2
L036A:
        neg dx
        cmp dx, di
        jl L0371
        inc si
L0371:
        xor ax, ax
        mov al, byte ptr [si+_gDirectionFromMask]
        pop si
        pop di
        pop dx
        pop cx
        pop bx
        pop bp
        retf
_DirectionFromPoints ENDP

; ---- 037E..03AF
; 17A6:037E: copy two 16-byte plane tables from the far pointers at [bp+6]/[bp+8] and
; [bp+0A]/[bp+0C] into CS:03AF (gPlaneTableA) and CS:03BF (gPlaneTableB). Called from overlay
; 3 (ovl3:0872, in LoadWallTextures, src/EOV3.C:472) with the first pointer at ds:F4D6 and
; the second at ds:FB4D.
        PUBLIC  _SetPlaneTables              ; @ 17A6:037E
_SetPlaneTables PROC FAR
        push bp
        mov bp, sp
        push cx
        push di
        push si
        push ds
        push es
        mov si, word ptr [bp+6]
        mov ax, word ptr [bp+8]
        mov ds, ax
        push cs
        pop es
        mov di, 3AFh
        mov cx, 8
        rep movsw
        mov si, word ptr [bp+000Ah]
        mov ax, word ptr [bp+000Ch]
        mov ds, ax
        mov di, 3BFh
        mov cx, 8
        rep movsw
        pop es
        pop ds
        pop si
        pop di
        pop cx
        pop bp
        retf
_SetPlaneTables ENDP

; ---- data 03AF..03BF
; 17A6:03AF: 16-byte plane table, filled by SetPlaneTables (17A6:037E) from the first far
; pointer (ds:F4D6); DrawTile's MCGA/VGA branch (L0ADA onward, taken for gGraphicsChoice 3,
; 4 and any other value) indexes it with a nibble to pick the plane of a pixel.
gPlaneTableA:
        db      00h, 01h, 02h, 03h, 04h, 05h, 06h, 07h, 08h, 09h, 0Ah, 0Bh, 0Ch, 0Dh, 0Eh, 0Fh
; ---- data 03BF..03CF
; 17A6:03BF: 16-byte plane table, filled by SetPlaneTables (17A6:037E) from the second far
; pointer (ds:FB4D); the same MCGA/VGA branch uses it when gFlipPlanes is 0.
gPlaneTableB:
        db      00h, 01h, 02h, 03h, 04h, 05h, 06h, 07h, 08h, 09h, 0Ah, 0Bh, 0Ch, 0Dh, 0Eh, 0Fh
; ---- data 03CF..03E7
; 17A6:03CF: scratch words of DrawTile (17A6:03E7) and its helpers. gPlaneSelect and
; gSrcPlaneSelect are 0 or 1 (the plane the source byte belongs to); gTileRows, gTileRows2
; and gTileCols are the loop counters; gSrcOff is the source far pointer's offset and gSrcSeg2
; its segment (at entry); gSrcSeg is a plane segment (A400 or A600 plus a paragraph, L0535 to
; L0545), not the source far pointer; gDstSeg is the destination segment; gFlipPlanes and
; gSecondPlane choose the plane table and the second-plane path.
gPlaneSelect dw 0
gSrcPlaneSelect dw 0
gTileRows dw 0
gTileRows2 dw 0
gTileCols dw 0
gSrcPlaneOffset dw 0
gSrcSeg dw 0
gDstSeg dw 0
gSrcOff dw 0
gSrcSeg2 dw 0
gFlipPlanes dw 0
gSecondPlane dw 0
; ---- 03E7..0D20
; 17A6:03E7: draw a tile from the far pointer at [bp+6]/[bp+8] into the screen buffer,
; dispatching on the graphics mode gGraphicsChoice: 2 (EGA) saves the far pointer and programs
; the sequencer and graphics controller, 0 (CGA) and 1 (Tandy) have their own paths, and the
; last branch takes 3 and 4 (MCGA, VGA) as well as any other value. Called from overlay 4
; (ovl4:1F6A).
        PUBLIC  _DrawTile              ; @ 17A6:03E7
_DrawTile PROC FAR
        push bp
        mov bp, sp
        push bx
        push cx
        push dx
        push di
        push si
        push ds
        push es
        mov si, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        mov word ptr cs:[gSrcOff], si
        mov word ptr cs:[gSrcSeg2], dx
        mov bx, 0ADB0h
        mov cx, 212Eh
        mov bp, cx
        cmp word ptr [gGraphicsChoice], 2
        je L0419
        mov di, word ptr [gArtPage]
        mov ax, word ptr [gArtPage+2]
        jmp short L041F
L0419:
        mov ax, 0A200h
        mov di, 0
L041F:
        mov es, ax
        mov word ptr cs:[gTileRows], 0Fh
        cmp word ptr [gGraphicsChoice], 2
        je L0446
        cmp word ptr [gGraphicsChoice], 0
        je L0443
        cmp word ptr [gGraphicsChoice], 1
        je L0440
        jmp L0ADA
L0440:
        jmp L0856
L0443:
        jmp L063E
L0446:
        call far ptr _GetFarPointer
        mov word ptr cs:[gSavedFarOff], ax
        mov word ptr cs:[gSavedFarSeg], dx
        push cs
        mov ax, 304h
        push ax
        call far ptr _SetFarPointer
        add sp, 4
L0461:
        mov dx, 3CFh
        mov al, 0
        cli
        mov byte ptr cs:[gSaveBuf+15], al
        out dx, al
        sti
        mov cx, 16h
L0470:
        mov word ptr cs:[gPlaneSelect], 0
        mov ds, bp
        mov ax, word ptr [bx]
        cmp ax, 8000h
        jb L0483
        jmp L0518
L0483:
        test ax, 4000h
        je L0492
        mov word ptr cs:[gPlaneSelect], 1
        and ax, 3FFFh
L0492:
        cmp ax, 0
        jne L04AA
        mov ax, word ptr [bx+0294h]
        test ax, 4000h
        je L04AA
        mov word ptr cs:[gPlaneSelect], 1
        and ax, 3FFFh
L04AA:
        add bx, 2
        mov si, 0
        test ax, 1
        je L04B8
        mov si, 8
L04B8:
        shr ax, 1
        add ax, 0A400h
        cmp word ptr cs:[gPlaneSelect], 1
        jne L04C8
        add ax, 600h
L04C8:
        mov ds, ax
        push cx
        mov cx, 8
L04CE:
        movsb
        add di, 27h
        loop L04CE
        sub di, 13Fh
        pop cx
L04D9:
        loop L0470
        add di, 12Ah
        mov ax, 0ADB0h
        mov ds, ax
        dec word ptr cs:[gTileRows]
        je L04EE
        jmp L0461
L04EE:
        mov dx, 3CEh
        mov ax, 0FF08h
        cli
        mov byte ptr cs:[gSaveBuf+6], al
        out dx, al
        inc dx
        mov al, ah
        mov byte ptr cs:[gSaveBuf+15], al
        out dx, al
        sti
        push word ptr cs:[gSavedFarSeg]
        push word ptr cs:[gSavedFarOff]
        call far ptr _SetFarPointer
        add sp, 4
        jmp L0D17
L0518:
        sub ax, 8000h
        test ax, 4000h
        je L052A
        and ax, 3FFFh
        mov word ptr cs:[gPlaneSelect], 1
L052A:
        mov si, 0
        test ax, 1
        je L0535
        mov si, 8
L0535:
        shr ax, 1
        add ax, 0A400h
        cmp word ptr cs:[gPlaneSelect], 1
        jne L0545
        add ax, 600h
L0545:
        mov word ptr cs:[gSrcPlaneOffset], si
        mov word ptr cs:[gSrcSeg], ax
        mov word ptr cs:[gSrcPlaneSelect], 0
        mov ax, word ptr [bx+0294h]
        test ax, 4000h
        je L0568
        sub ax, 4000h
        mov word ptr cs:[gSrcPlaneSelect], 1
L0568:
        add bx, 2
        mov si, 0
        test ax, 1
        je L0576
        mov si, 8
L0576:
        shr ax, 1
        add ax, 0A400h
        cmp word ptr cs:[gSrcPlaneSelect], 1
        jne L0586
        add ax, 600h
L0586:
        mov word ptr cs:[gDstSeg], ax
        push cx
        push bx
        mov bx, si
        mov si, word ptr cs:[gSrcPlaneOffset]
        mov ds, word ptr cs:[gSrcSeg]
        mov dx, 3CFh
        mov al, 0FFh
        cli
        mov byte ptr cs:[gSaveBuf+15], al
        out dx, al
        sti
        mov word ptr cs:[gTileCols], 8
L05AB:
        mov dx, 3CEh
        mov ax, 805h
        cli
        mov byte ptr cs:[gSaveBuf+6], al
        out dx, al
        inc dx
        mov al, ah
        mov byte ptr cs:[gSaveBuf+12], al
        out dx, al
        sti
        mov ah, byte ptr [si]
        xor al, al
        cli
        mov byte ptr cs:[gSaveBuf+12], al
        out dx, al
        sti
        dec dx
        mov al, 4
        cli
        mov byte ptr cs:[gSaveBuf+6], al
        out dx, al
        sti
        mov cl, 0
        call _SetPlaneAndWrite
        mov cl, 1
        call _SetPlaneAndWrite
        mov cl, 2
        call _SetPlaneAndWrite
        mov cl, 3
        call _SetPlaneAndWrite
        mov dx, 3CFh
        mov al, 0
        cli
        mov byte ptr cs:[gSaveBuf+11], al
        out dx, al
        sti
        add di, 28h
        inc si
        inc bx
        dec word ptr cs:[gTileCols]
        jne L05AB
        sub di, 13Fh
        pop bx
        pop cx
        mov dx, 3C5h
        mov al, 0Fh
        cli
        mov byte ptr cs:[gSaveBuf+3], al
        out dx, al
        sti
        mov dx, 3CEh
        mov ax, 205h
        cli
        mov byte ptr cs:[gSaveBuf+6], al
        out dx, al
        inc dx
        mov al, ah
        mov byte ptr cs:[gSaveBuf+12], al
        out dx, al
        sti
        dec dx
        mov ax, 8
        cli
        mov byte ptr cs:[gSaveBuf+6], al
        out dx, al
        inc dx
        mov al, ah
        mov byte ptr cs:[gSaveBuf+15], al
        out dx, al
        sti
        jmp L04D9
L063E:
        mov cx, 16h
L0641:
        mov word ptr cs:[gPlaneSelect], 0
        mov ds, bp
        mov ax, word ptr [bx]
        cmp ah, 80h
        jb L0657
        sub ah, 80h
        jmp L07C7
L0657:
        test ah, 40h
        je L0666
        mov word ptr cs:[gPlaneSelect], 1
        and ah, 3Fh
L0666:
        cmp ax, 0
        jne L067E
        mov ax, word ptr [bx+0294h]
        test ah, 40h
        je L067E
        mov word ptr cs:[gPlaneSelect], 1
        and ah, 3Fh
L067E:
        add bx, 2
        push si
        shl ax, 1
        add ax, dx
        mov ds, ax
        cmp word ptr cs:[gPlaneSelect], 1
        jne L0692
        jmp short L06D6
L0692:
        movsb
        inc si
        movsb
        inc si
        add di, 1FFEh
        movsb
        inc si
        movsb
        inc si
        sub di, 1FB2h
        movsb
        inc si
        movsb
        inc si
        add di, 1FFEh
        movsb
        inc si
        movsb
        inc si
        sub di, 1FB2h
        movsb
        inc si
        movsb
        inc si
        add di, 1FFEh
        movsb
        inc si
        movsb
        inc si
        sub di, 1FB2h
        movsb
        inc si
        movsb
        inc si
        add di, 1FFEh
        movsb
        inc si
        movsb
        inc si
        sub di, 20F0h
        pop si
        jmp L07AB
L06D6:
        push bx
        xor bx, bx
        add si, 2
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        sub si, 2
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        add si, 6
        add di, 1FFEh
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        sub si, 2
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        add si, 6
        sub di, 1FB2h
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        sub si, 2
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        add si, 6
        add di, 1FFEh
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        sub si, 2
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        add si, 6
        sub di, 1FB2h
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        sub si, 2
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        add si, 6
        add di, 1FFEh
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        sub si, 2
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        add si, 6
        sub di, 1FB2h
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        sub si, 2
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        add si, 6
        add di, 1FFEh
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        sub si, 2
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0100h]
        stosb
        sub di, 20F0h
        pop bx
        pop si
L07AB:
        dec cx
        je L07B1
        jmp L0641
L07B1:
        add di, 114h
        mov ax, 212Eh
        mov ds, ax
        dec word ptr cs:[gTileRows]
        je L07C4
        jmp L063E
L07C4:
        jmp L0D17
L07C7:
        test ax, 4000h
        je L07D6
        and ax, 3FFFh
        mov word ptr cs:[gPlaneSelect], 1
L07D6:
        shl ax, 1
        add ax, word ptr cs:[gSrcSeg2]
        mov word ptr cs:[gSrcSeg], ax
        mov si, word ptr cs:[gSrcOff]
        push si
        mov word ptr cs:[gSrcPlaneSelect], 0
        mov ax, word ptr [bx+0294h]
        test ax, 4000h
        je L0801
        and ax, 3FFFh
        mov word ptr cs:[gSrcPlaneSelect], 1
L0801:
        add bx, 2
        push bx
        mov bx, word ptr cs:[gSrcOff]
        shl ax, 1
        add ax, word ptr cs:[gSrcSeg2]
        mov word ptr cs:[gDstSeg], ax
        push bp
        call _BlitMaskedWord
        add di, 1FFEh
        call _BlitMaskedWord
        sub di, 1FB2h
        call _BlitMaskedWord
        add di, 1FFEh
        call _BlitMaskedWord
        sub di, 1FB2h
        call _BlitMaskedWord
        add di, 1FFEh
        call _BlitMaskedWord
        sub di, 1FB2h
        call _BlitMaskedWord
        add di, 1FFEh
        call _BlitMaskedWord
        sub di, 20F0h
        pop bp
        pop bx
        pop si
        xor ch, ch
        jmp L07AB
L0856:
        mov cx, 16h
L0859:
        mov ds, bp
        mov ax, word ptr [bx]
        mov word ptr cs:[gPlaneSelect], 0
        cmp ax, 8000h
        jb L086F
        sub ax, 8000h
        jmp L0A4F
L086F:
        test ax, 4000h
        je L087E
        mov word ptr cs:[gPlaneSelect], 1
        and ax, 3FFFh
L087E:
        cmp ax, 0
        jne L0896
        mov ax, word ptr [bx+0294h]
        test ax, 4000h
        je L0896
        mov word ptr cs:[gPlaneSelect], 1
        and ax, 3FFFh
L0896:
        add bx, 2
        push si
        shl ax, 1
        add ax, dx
        mov ds, ax
        cmp word ptr cs:[gPlaneSelect], 1
        jne L08AA
        jmp short L08DE
L08AA:
        movsw
        movsw
        add di, 1FFCh
        movsw
        movsw
        add di, 1FFCh
        movsw
        movsw
        add di, 1FFCh
        movsw
        movsw
        sub di, 5F64h
        movsw
        movsw
        add di, 1FFCh
        movsw
        movsw
        add di, 1FFCh
        movsw
        movsw
        add di, 1FFCh
        movsw
        movsw
        sub di, 60A0h
        pop si
        jmp L0A33
L08DE:
        push bx
        xor bx, bx
        mov bl, byte ptr [si+3]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+2]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+1]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        add si, 4
        add di, 1FFCh
        mov bl, byte ptr [si+3]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+2]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+1]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        add si, 4
        add di, 1FFCh
        mov bl, byte ptr [si+3]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+2]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+1]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        add si, 4
        add di, 1FFCh
        mov bl, byte ptr [si+3]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+2]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+1]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        add si, 4
        sub di, 5F64h
        mov bl, byte ptr [si+3]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+2]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+1]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        add si, 4
        add di, 1FFCh
        mov bl, byte ptr [si+3]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+2]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+1]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        add si, 4
        add di, 1FFCh
        mov bl, byte ptr [si+3]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+2]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+1]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        add si, 4
        add di, 1FFCh
        mov bl, byte ptr [si+3]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+2]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si+1]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0200h]
        stosb
        add si, 4
        sub di, 60A0h
        pop bx
        pop si
L0A33:
        dec cx
        je L0A39
        jmp L0859
L0A39:
        add di, 0E8h
        mov ax, 212Eh
        mov ds, ax
        dec word ptr cs:[gTileRows]
        je L0A4C
        jmp L0856
L0A4C:
        jmp L0D17
L0A4F:
        test ax, 4000h
        je L0A5E
        and ax, 3FFFh
        mov word ptr cs:[gPlaneSelect], 1
L0A5E:
        shl ax, 1
        add ax, word ptr cs:[gSrcSeg2]
        mov word ptr cs:[gSrcSeg], ax
        mov si, word ptr cs:[gSrcOff]
        push si
        mov word ptr cs:[gSrcPlaneSelect], 0
        mov ax, word ptr [bx+0294h]
        test ax, 4000h
        je L0A89
        and ax, 3FFFh
        mov word ptr cs:[gSrcPlaneSelect], 1
L0A89:
        add bx, 2
        push bx
        mov bx, word ptr cs:[gSrcOff]
        shl ax, 1
        add ax, word ptr cs:[gSrcSeg2]
        mov word ptr cs:[gDstSeg], ax
        call _BlitMaskedByte
        add di, 1FFCh
        call _BlitMaskedByte
        add di, 1FFCh
        call _BlitMaskedByte
        add di, 1FFCh
        call _BlitMaskedByte
        sub di, 5F64h
        call _BlitMaskedByte
        add di, 1FFCh
        call _BlitMaskedByte
        add di, 1FFCh
        call _BlitMaskedByte
        add di, 1FFCh
        call _BlitMaskedByte
        sub di, 60A0h
        pop bx
        pop si
        jmp L0A33
L0ADA:
        mov word ptr cs:[gTileRows2], 16h
L0AE1:
        mov word ptr cs:[gFlipPlanes], 0
        mov ds, bp
        mov ax, word ptr [bx]
        mov word ptr cs:[gPlaneSelect], 0
        mov word ptr cs:[gSecondPlane], 0
        cmp ax, 8000h
        jb L0B09
        sub ax, 8000h
        mov word ptr cs:[gSecondPlane], ax
        mov ax, 0
L0B09:
        test ax, 4000h
        je L0B18
        mov word ptr cs:[gPlaneSelect], 1
        and ax, 3FFFh
L0B18:
        cmp ax, 0
        jne L0B37
        mov word ptr cs:[gFlipPlanes], 1
        mov ax, word ptr [bx+0294h]
        test ax, 4000h
        je L0B37
        mov word ptr cs:[gPlaneSelect], 1
        and ax, 3FFFh
L0B37:
        add bx, 2
        shl ax, 1
        add ax, word ptr cs:[gSrcSeg2]
        mov ds, ax
        mov si, word ptr cs:[gSrcOff]
        cmp word ptr cs:[gPlaneSelect], 1
        je L0BAA
        push bx
        xor bx, bx
        mov dx, 8
L0B56:
        mov cx, 4
L0B59:
        mov bl, byte ptr [si]
        inc si
        mov al, bl
        and al, 0F0h
        shr al, 1
        shr al, 1
        shr al, 1
        shr al, 1
        mov ah, bl
        and ah, 0Fh
        mov bl, ah
        cmp word ptr cs:[gFlipPlanes], 1
        jne L0B7E
        mov ah, byte ptr cs:[bx+gPlaneTableA]
        jmp short L0B83
L0B7E:
        mov ah, byte ptr cs:[bx+gPlaneTableB]
L0B83:
        mov bl, al
        cmp word ptr cs:[gFlipPlanes], 1
        jne L0B94
        mov al, byte ptr cs:[bx+gPlaneTableA]
        jmp short L0B99
L0B94:
        mov al, byte ptr cs:[bx+gPlaneTableB]
L0B99:
        stosw
        loop L0B59
        add di, 138h
        dec dx
        jne L0B56
        sub di, 9F8h
        pop bx
        jmp short L0C0A
L0BAA:
        push bx
        xor bx, bx
        mov dx, 8
L0BB0:
        mov cx, 4
        add si, 3
L0BB6:
        mov bl, byte ptr [si]
        dec si
        mov ah, bl
        and ah, 0F0h
        shr ah, 1
        shr ah, 1
        shr ah, 1
        shr ah, 1
        mov al, bl
        and al, 0Fh
        mov bl, ah
        cmp word ptr cs:[gFlipPlanes], 1
        jne L0BDB
        mov ah, byte ptr cs:[bx+gPlaneTableA]
        jmp short L0BE0
L0BDB:
        mov ah, byte ptr cs:[bx+gPlaneTableB]
L0BE0:
        mov bl, al
        cmp word ptr cs:[gFlipPlanes], 1
        jne L0BF1
        mov al, byte ptr cs:[bx+gPlaneTableA]
        jmp short L0BF6
L0BF1:
        mov al, byte ptr cs:[bx+gPlaneTableB]
L0BF6:
        stosw
        loop L0BB6
        add si, 5
        add di, 138h
        dec dx
        jne L0BB0
        sub di, 9F8h
        pop bx
        jmp short L0C0A
L0C0A:
        cmp word ptr cs:[gSecondPlane], 0
        jne L0C32
        dec word ptr cs:[gTileRows2]
        je L0C1C
        jmp L0AE1
L0C1C:
        add di, 950h
        mov ax, 212Eh
        mov ds, ax
        dec word ptr cs:[gTileRows]
        je L0C2F
        jmp L0ADA
L0C2F:
        jmp L0D17
L0C32:
        sub di, 8
        mov word ptr cs:[gPlaneSelect], 0
        mov ax, word ptr cs:[gSecondPlane]
        test ax, 4000h
        je L0C4F
        and ax, 3FFFh
        mov word ptr cs:[gPlaneSelect], 1
L0C4F:
        shl ax, 1
        add ax, word ptr cs:[gSrcSeg2]
        mov ds, ax
        mov si, word ptr cs:[gSrcOff]
        cmp word ptr cs:[gPlaneSelect], 1
        je L0CBB
        push bx
        xor bx, bx
        mov dx, 8
L0C6B:
        mov cx, 4
L0C6E:
        mov bl, byte ptr [si]
        inc si
        mov ah, bl
        and ah, 0F0h
        shr ah, 1
        shr ah, 1
        shr ah, 1
        shr ah, 1
        mov al, bl
        and al, 0Fh
        mov bl, ah
        mov ah, byte ptr cs:[bx+gPlaneTableB]
        mov bl, al
        mov al, byte ptr cs:[bx+gPlaneTableB]
        cmp ah, 0
        je L0C98
        mov byte ptr es:[di], ah
L0C98:
        inc di
        cmp al, 0
        je L0CA0
        mov byte ptr es:[di], al
L0CA0:
        inc di
        xor bx, bx
        loop L0C6E
        add di, 138h
        dec dx
        jne L0C6B
        sub di, 9F8h
        pop bx
        mov word ptr cs:[gSecondPlane], 0
        jmp L0C0A
L0CBB:
        push bx
        xor bx, bx
        mov dx, 8
L0CC1:
        mov cx, 4
        add si, 3
L0CC7:
        mov bl, byte ptr [si]
        dec si
        mov al, bl
        and al, 0F0h
        shr al, 1
        shr al, 1
        shr al, 1
        shr al, 1
        mov ah, bl
        and ah, 0Fh
        mov bl, ah
        mov ah, byte ptr cs:[bx+gPlaneTableB]
        mov bl, al
        mov al, byte ptr cs:[bx+gPlaneTableB]
        cmp ah, 0
        je L0CF1
        mov byte ptr es:[di], ah
L0CF1:
        inc di
        cmp al, 0
        je L0CF9
        mov byte ptr es:[di], al
L0CF9:
        inc di
        xor bx, bx
        loop L0CC7
        add si, 5
        add di, 138h
        dec dx
        jne L0CC1
        sub di, 9F8h
        pop bx
        mov word ptr cs:[gSecondPlane], 0
        jmp L0C0A
L0D17:
        pop es
        pop ds
        pop si
        pop di
        pop dx
        pop cx
        pop bx
        pop bp
        retf
_DrawTile ENDP

; ---- 0D20..0E53
; 17A6:0D20: one masked byte of the EGA tile path: read a byte from the destination at
; CS:03DD (gDstSeg) with offset BX and a byte from the source at CS:03DB (gSrcSeg) with the
; source offset SI (gSrcOff, CS:03DF, is loaded by the caller), translate each through the
; table at CS:0200 when its plane is 1, combine them, and store at ES:DI. Called from
; DrawTile (17A6:0A9D..0AD1).
        PUBLIC  _BlitMaskedByte              ; @ 17A6:0D20
_BlitMaskedByte PROC NEAR
        push dx
        mov dx, bx
        mov ds, word ptr cs:[gDstSeg]
        cmp word ptr cs:[gSrcPlaneSelect], 1
        jne L0D3E
        mov ah, byte ptr [bx+3]
        xor bx, bx
        mov bl, ah
        mov ah, byte ptr cs:[bx+0200h]
        jmp short L0D41
L0D3E:
        mov ah, byte ptr [bx]
        inc dx
L0D41:
        mov ds, word ptr cs:[gSrcSeg]
        cmp word ptr cs:[gPlaneSelect], 1
        jne L0D5A
        xor bx, bx
        mov bl, byte ptr [si+3]
        mov al, byte ptr cs:[bx+0200h]
        jmp short L0D5B
L0D5A:
        lodsb
L0D5B:
        test al, 0F0h
        je L0D62
        and ah, 0Fh
L0D62:
        test al, 0Fh
        je L0D69
        and ah, 0F0h
L0D69:
        or al, ah
        stosb
        mov bx, dx
        mov ds, word ptr cs:[gDstSeg]
        cmp word ptr cs:[gSrcPlaneSelect], 1
        jne L0D89
        mov ah, byte ptr [bx+2]
        xor bx, bx
        mov bl, ah
        mov ah, byte ptr cs:[bx+0200h]
        jmp short L0D8C
L0D89:
        mov ah, byte ptr [bx]
        inc dx
L0D8C:
        mov ds, word ptr cs:[gSrcSeg]
        cmp word ptr cs:[gPlaneSelect], 1
        jne L0DA5
        xor bx, bx
        mov bl, byte ptr [si+2]
        mov al, byte ptr cs:[bx+0200h]
        jmp short L0DA6
L0DA5:
        lodsb
L0DA6:
        test al, 0F0h
        je L0DAD
        and ah, 0Fh
L0DAD:
        test al, 0Fh
        je L0DB4
        and ah, 0F0h
L0DB4:
        or al, ah
        stosb
        mov bx, dx
        mov ds, word ptr cs:[gDstSeg]
        cmp word ptr cs:[gSrcPlaneSelect], 1
        jne L0DD4
        mov ah, byte ptr [bx+1]
        xor bx, bx
        mov bl, ah
        mov ah, byte ptr cs:[bx+0200h]
        jmp short L0DD7
L0DD4:
        mov ah, byte ptr [bx]
        inc dx
L0DD7:
        mov ds, word ptr cs:[gSrcSeg]
        cmp word ptr cs:[gPlaneSelect], 1
        jne L0DF0
        xor bx, bx
        mov bl, byte ptr [si+1]
        mov al, byte ptr cs:[bx+0200h]
        jmp short L0DF1
L0DF0:
        lodsb
L0DF1:
        test al, 0F0h
        je L0DF8
        and ah, 0Fh
L0DF8:
        test al, 0Fh
        je L0DFF
        and ah, 0F0h
L0DFF:
        or al, ah
        stosb
        mov bx, dx
        mov ds, word ptr cs:[gDstSeg]
        cmp word ptr cs:[gSrcPlaneSelect], 1
        jne L0E21
        mov ah, byte ptr [bx]
        add dx, 4
        xor bx, bx
        mov bl, ah
        mov ah, byte ptr cs:[bx+0200h]
        jmp short L0E24
L0E21:
        mov ah, byte ptr [bx]
        inc dx
L0E24:
        mov ds, word ptr cs:[gSrcSeg]
        cmp word ptr cs:[gPlaneSelect], 1
        jne L0E3D
        mov bl, byte ptr [si]
        add si, 4
        mov al, byte ptr cs:[bx+0200h]
        jmp short L0E3E
L0E3D:
        lodsb
L0E3E:
        test al, 0F0h
        je L0E45
        and ah, 0Fh
L0E45:
        test al, 0Fh
        je L0E4C
        and ah, 0F0h
L0E4C:
        or al, ah
        stosb
        mov bx, dx
        pop dx
        ret
_BlitMaskedByte ENDP

; ---- 0E53..0EEE
; 17A6:0E53: one masked word of the EGA tile path: read a word from the destination and a
; word from the source, translate both bytes of each through the table at CS:0100, combine
; them, and store at ES:DI. Called from DrawTile (17A6:0816..0847).
        PUBLIC  _BlitMaskedWord              ; @ 17A6:0E53
_BlitMaskedWord PROC NEAR
        mov ds, word ptr cs:[gSrcSeg]
        cmp word ptr cs:[gPlaneSelect], 1
        jne L0E7B
        add si, 2
        lodsw
        sub si, 4
        push bx
        xor bx, bx
        mov bl, ah
        mov ah, byte ptr cs:[bx+0100h]
        mov bl, al
        mov al, byte ptr cs:[bx+0100h]
        pop bx
        jmp short L0E7C
L0E7B:
        lodsw
L0E7C:
        mov ds, word ptr cs:[gDstSeg]
        cmp word ptr cs:[gSrcPlaneSelect], 1
        jne L0E99
        mov ch, byte ptr [bx+2]
        push bx
        xor bx, bx
        mov bl, ch
        mov ch, byte ptr cs:[bx+0100h]
        pop bx
        jmp short L0E9E
L0E99:
        mov ch, byte ptr [bx]
        add bx, 2
L0E9E:
        and ah, ch
        or al, ah
        stosb
        cmp word ptr cs:[gSrcPlaneSelect], 1
        jne L0EBD
        mov ch, byte ptr [bx]
        add bx, 4
        push bx
        xor bx, bx
        mov bl, ch
        mov ch, byte ptr cs:[bx+0100h]
        pop bx
        jmp short L0EC2
L0EBD:
        mov ch, byte ptr [bx]
        add bx, 2
L0EC2:
        mov ds, word ptr cs:[gSrcSeg]
        cmp word ptr cs:[gPlaneSelect], 1
        jne L0EE7
        lodsw
        add si, 2
        push bx
        xor bx, bx
        mov bl, ah
        mov ah, byte ptr cs:[bx+0100h]
        mov bl, al
        mov al, byte ptr cs:[bx+0100h]
        pop bx
        jmp short L0EE8
L0EE7:
        lodsw
L0EE8:
        and ah, ch
        or al, ah
        stosb
        ret
_BlitMaskedWord ENDP

; ---- 0EEE..0F1F
; 17A6:0EEE: select plane CL in the sequencer (3C5) and the graphics controller (3CF),
; then combine the destination byte at DS:BX (DS is gDstSeg) with the mask in AH and the
; source byte at DS:SI (DS is gSrcSeg), and store it at ES:DI. Called from
; DrawTile (17A6:05D7..05E6).
        PUBLIC  _SetPlaneAndWrite              ; @ 17A6:0EEE
_SetPlaneAndWrite PROC NEAR
        mov dx, 3CFh
        mov al, cl
        cli
        mov byte ptr cs:[gSaveBuf+11], al
        out dx, al
        sti
        mov dx, 3C5h
        mov al, 1
        shl al, cl
        cli
        mov byte ptr cs:[gSaveBuf+3], al
        out dx, al
        sti
        mov ds, word ptr cs:[gDstSeg]
        mov al, byte ptr [bx]
        mov ds, word ptr cs:[gSrcSeg]
        and al, ah
        or al, byte ptr [si]
        mov ch, byte ptr es:[di]
        mov byte ptr es:[di], al
        ret
_SetPlaneAndWrite ENDP

; ---- 0F1F..0F61
; 17A6:0F1F: copy 0x1F40 groups of four source bytes from the far pointer at [bp+6]/[bp+8] to
; [bp+0A]/[bp+0C], nibble-swapping each byte through the table at CS:0200, one output byte per
; source byte. A group is read backwards from si + 3, then si advances by 4. The name rests on
; the caller's comment, "expands the planes into gStagingPage" (src/EOV3.C:456 to :457). Called
; from overlay 3 (ovl3:08A8, 08DA).
        PUBLIC  _ExpandNibbles              ; @ 17A6:0F1F
_ExpandNibbles PROC FAR
        push bp
        mov bp, sp
        push bx
        push cx
        push dx
        push di
        push si
        push ds
        push es
        mov si, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        mov di, word ptr [bp+000Ah]
        mov ax, word ptr [bp+000Ch]
        mov ds, dx
        mov es, ax
        mov cx, 1F40h
        xor bx, bx
L0F3E:
        add si, 3
        mov dx, 4
L0F44:
        mov bl, byte ptr [si]
        mov al, byte ptr cs:[bx+0200h]
        mov byte ptr es:[di], al
        inc di
        dec si
        dec dx
        jne L0F44
        add si, 5
        loop L0F3E
        pop es
        pop ds
        pop si
        pop di
        pop dx
        pop cx
        pop bx
        pop bp
        retf
_ExpandNibbles ENDP

; ---- 0F61..0F9D
; 17A6:0F61: return an approximate distance between two packed map cells (y * 32 + x, a 10-bit
; value; the low five bits of each are x, the rest y): the larger of the two absolute
; differences plus half the smaller (the octagonal metric). Callers use it as a sound
; attenuation (177D:026A) and a proximity test (08E6:00C3, 08E6:0765, 08E6:1410, ovl6:09D2).
        PUBLIC  _Distance              ; @ 17A6:0F61
_Distance PROC FAR
        push bp
        mov bp, sp
        push bx
        push cx
        push dx
        mov ax, word ptr [bp+6]
        mov bx, word ptr [bp+8]
        mov cl, 3
        mov dl, al
        and dl, 1Fh
        shl ax, cl
        mov dh, ah
        mov al, bl
        and al, 1Fh
        shl bx, cl
        mov ah, bh
        sub ah, dh
        jns L0F86
        neg ah
L0F86:
        sub al, dl
        jns L0F8C
        neg al
L0F8C:
        cmp al, ah
        jl L0F92
        xchg al, ah
L0F92:
        shr al, 1
        add al, ah
        xor ah, ah
        pop dx
        pop cx
        pop bx
        pop bp
        retf
_Distance ENDP

; ---- 0F9D..0FEF
; 17A6:0F9D: search the 8 neighbour offsets at ds:7FCC and then a pair of the 4 offsets at
; ds:7FDC for the one that maps [bp+6] to [bp+8] modulo 0x400; return the matched value or
; 0xFFFF. The third argument [bp+0Ah] picks the pair by the word index 2 * dx into ds:7FDC
; (dx = 0 gives 7FDC/7FDE, dx = 1 gives 7FE0/7FE2; see the L0FB6 setup and L0FD1).
; Purpose unknown: no caller in this build. Checked: a byte scan of eob/EOB.EXE and
; build/re/eob-overlays/*.bin finds no far call 9A 9D 0F A8 00 (the overlays reach resident
; units with segment 00A8, e.g. ovl3:0872 calls 17A6:037E), and src/ has no caller; a call
; through a far pointer (src/E1DE7.C:71) cannot be excluded that way.
        PUBLIC  _FindNeighbour              ; @ 17A6:0F9D
_FindNeighbour PROC FAR
        push bp
        mov bp, sp
        push bx
        push cx
        push dx
        push di
        push si
        push ds
        push es
        mov si, word ptr [bp+6]
        mov di, word ptr [bp+8]
        mov dx, word ptr [bp+000Ah]
        mov cx, 8
        mov bx, 0
L0FB6:
        mov ax, si
        add ax, word ptr [bx+_gNeighbourOffsets]
        and ax, 3FFh
        cmp ax, di
        je L0FE6
        add bx, 2
        loop L0FB6
        mov cx, 2
        mov bx, dx
        shl bx, 1
        shl bx, 1
L0FD1:
        mov ax, si
        add ax, word ptr [bx+_gAheadOffsets2And3]
        and ax, 3FFh
        cmp ax, di
        je L0FE6
        add bx, 2
        loop L0FD1
        mov ax, 0FFFFh
L0FE6:
        pop es
        pop ds
        pop si
        pop di
        pop dx
        pop cx
        pop bx
        pop bp
        retf
_FindNeighbour ENDP

; ---- data 0FEF..0FFA
; 17A6:0FEF: scratch words of the sprite unpackers. gSpriteRows is the row count,
; gSpriteWidth the pixel width, gSpriteStride the source row stride, gSpriteGroups and
; gSpriteRemainder the stride divided by 3, and gSpriteMode a copy of gGraphicsChoice.
gSpriteRows db 0
gSpriteWidth dw 0
gSpriteStride dw 0
gSpriteGroups dw 0
gSpriteRemainder dw 0
gSpriteMode dw 0
; ---- 0FFA..109A
; 17A6:0FFA: unpack a sprite from the far pointer at [bp+6]/[bp+8] to [bp+0A]/[bp+0C]
; for the Tandy and MCGA modes (gGraphicsChoice 1 and 3). Called from overlay 4 (ovl4:0F1D).
        PUBLIC  _BlitSpriteDefault              ; @ 17A6:0FFA
_BlitSpriteDefault PROC FAR
        push bp
        mov bp, sp
        push bx
        push cx
        push dx
        push di
        push si
        push ds
        push es
        mov ax, word ptr [gGraphicsChoice]
        mov word ptr cs:[gSpriteMode], ax
        mov si, word ptr [bp+6]
        mov ds, word ptr [bp+8]
        mov di, word ptr [bp+000Ah]
        mov es, word ptr [bp+000Ch]
        xor ax, ax
        movsb
        lodsb
        inc al
        mov byte ptr cs:[gSpriteRows], al
        shl ax, 1
        xor dx, dx
        mov bx, 3
        div bx
        mov cl, al
        stosb
        xor ax, ax
        lodsb
        mov word ptr cs:[gSpriteWidth], ax
        shl ax, 1
        mov word ptr cs:[gSpriteStride], ax
        shl word ptr cs:[gSpriteStride], 1
        xor dx, dx
        mov bx, 3
        div bx
        or dl, dl
        je L104B
        inc ax
L104B:
        stosb
        mov al, cl
        stosb
        inc si
        cmp word ptr cs:[gSpriteMode], 1
        je L105D
        mov cx, 10h
        rep movsb
L105D:
        mov ax, word ptr cs:[gSpriteStride]
        xor dx, dx
        mov bx, 3
        div bx
        mov word ptr cs:[gSpriteGroups], ax
        mov word ptr cs:[gSpriteRemainder], dx
L1071:
        call _BlitSpriteRowDefault
        dec byte ptr cs:[gSpriteRows]
        je L1091
        call _BlitSpriteRowDefault
        dec byte ptr cs:[gSpriteRows]
        je L1091
        add si, word ptr cs:[gSpriteStride]
        dec byte ptr cs:[gSpriteRows]
        jne L1071
L1091:
        pop es
        pop ds
        pop si
        pop di
        pop dx
        pop cx
        pop bx
        pop bp
        retf
_BlitSpriteDefault ENDP

; ---- 109A..10CD
; 17A6:109A: one row of BlitSpriteDefault: copy CS:0FF4 groups of a byte plus a
; shifted word, then the remainder. Called from 17A6:1071..108F.
        PUBLIC  _BlitSpriteRowDefault              ; @ 17A6:109A
_BlitSpriteRowDefault PROC NEAR
        mov bx, word ptr cs:[gSpriteGroups]
        mov cl, 4
L10A1:
        movsb
        lodsw
        xchg ah, al
        shr ax, cl
        stosb
        dec bx
        jne L10A1
        cmp word ptr cs:[gSpriteRemainder], 1
        jne L10B8
        movsb
        xor ax, ax
        stosb
        ret
L10B8:
        cmp word ptr cs:[gSpriteRemainder], 2
        jne L10CC
        lodsw
        and al, 0F0h
        shr ah, cl
        or al, ah
        stosb
        xor ax, ax
        stosw
        stosb
L10CC:
        ret
_BlitSpriteRowDefault ENDP

; ---- data 10CD..11CD
; 17A6:10CD: 256-byte table used by the CGA sprite rows (17A6:128B, 12EC): entry i is
; ((i >> 4) << 2) | (i & 3), so each group of four entries repeats the same four values and
; the value rises by 4 every 16 entries.
gTable10CD:
        db      00h, 01h, 02h, 03h, 00h, 01h, 02h, 03h, 00h, 01h, 02h, 03h, 00h, 01h, 02h, 03h
        db      04h, 05h, 06h, 07h, 04h, 05h, 06h, 07h, 04h, 05h, 06h, 07h, 04h, 05h, 06h, 07h
        db      08h, 09h, 0Ah, 0Bh, 08h, 09h, 0Ah, 0Bh, 08h, 09h, 0Ah, 0Bh, 08h, 09h, 0Ah, 0Bh
        db      0Ch, 0Dh, 0Eh, 0Fh, 0Ch, 0Dh, 0Eh, 0Fh, 0Ch, 0Dh, 0Eh, 0Fh, 0Ch, 0Dh, 0Eh, 0Fh
        db      10h, 11h, 12h, 13h, 10h, 11h, 12h, 13h, 10h, 11h, 12h, 13h, 10h, 11h, 12h, 13h
        db      14h, 15h, 16h, 17h, 14h, 15h, 16h, 17h, 14h, 15h, 16h, 17h, 14h, 15h, 16h, 17h
        db      18h, 19h, 1Ah, 1Bh, 18h, 19h, 1Ah, 1Bh, 18h, 19h, 1Ah, 1Bh, 18h, 19h, 1Ah, 1Bh
        db      1Ch, 1Dh, 1Eh, 1Fh, 1Ch, 1Dh, 1Eh, 1Fh, 1Ch, 1Dh, 1Eh, 1Fh, 1Ch, 1Dh, 1Eh, 1Fh
        db      20h, 21h, 22h, 23h, 20h, 21h, 22h, 23h, 20h, 21h, 22h, 23h, 20h, 21h, 22h, 23h
        db      24h, 25h, 26h, 27h, 24h, 25h, 26h, 27h, 24h, 25h, 26h, 27h, 24h, 25h, 26h, 27h
        db      28h, 29h, 2Ah, 2Bh, 28h, 29h, 2Ah, 2Bh, 28h, 29h, 2Ah, 2Bh, 28h, 29h, 2Ah, 2Bh
        db      2Ch, 2Dh, 2Eh, 2Fh, 2Ch, 2Dh, 2Eh, 2Fh, 2Ch, 2Dh, 2Eh, 2Fh, 2Ch, 2Dh, 2Eh, 2Fh
        db      30h, 31h, 32h, 33h, 30h, 31h, 32h, 33h, 30h, 31h, 32h, 33h, 30h, 31h, 32h, 33h
        db      34h, 35h, 36h, 37h, 34h, 35h, 36h, 37h, 34h, 35h, 36h, 37h, 34h, 35h, 36h, 37h
        db      38h, 39h, 3Ah, 3Bh, 38h, 39h, 3Ah, 3Bh, 38h, 39h, 3Ah, 3Bh, 38h, 39h, 3Ah, 3Bh
        db      3Ch, 3Dh, 3Eh, 3Fh, 3Ch, 3Dh, 3Eh, 3Fh, 3Ch, 3Dh, 3Eh, 3Fh, 3Ch, 3Dh, 3Eh, 3Fh
; ---- 11CD..128B
; 17A6:11CD: unpack a sprite from the far pointer at [bp+6]/[bp+8] to [bp+0A]/[bp+0C]
; for the CGA mode (gGraphicsChoice 0). Called from overlay 4 (ovl4:0F08).
        PUBLIC  _BlitSpriteCga              ; @ 17A6:11CD
_BlitSpriteCga PROC FAR
        push bp
        mov bp, sp
        push bx
        push cx
        push dx
        push di
        push si
        push ds
        push es
        mov si, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        mov di, word ptr [bp+000Ah]
        mov ax, word ptr [bp+000Ch]
        mov es, ax
        mov ds, dx
        xor ax, ax
        movsb
        lodsb
        mov byte ptr cs:[gSpriteRows], al
        shl ax, 1
        xor dx, dx
        mov bx, 3
        div bx
        mov cl, al
        stosb
        xor ax, ax
        lodsb
        mov word ptr cs:[gSpriteWidth], ax
        shl ax, 1
        mov word ptr cs:[gSpriteStride], ax
        xor dx, dx
        mov bx, 3
        div bx
        or dl, dl
        je L1214
        inc ax
L1214:
        stosb
        mul cl
        shl ax, 1
        add ax, di
        inc ax
        mov bp, ax
        mov al, cl
        stosb
        inc si
        mov ax, word ptr cs:[gSpriteStride]
        xor dx, dx
        mov bx, 3
        div bx
        mov word ptr cs:[gSpriteGroups], ax
        mov word ptr cs:[gSpriteRemainder], dx
        mov al, byte ptr cs:[gSpriteRows]
        push ax
L123B:
        call _BlitSpriteRowCga
        dec byte ptr cs:[gSpriteRows]
        je L125B
        call _BlitSpriteRowCga
        dec byte ptr cs:[gSpriteRows]
        je L125B
        add si, word ptr cs:[gSpriteStride]
        dec byte ptr cs:[gSpriteRows]
        jne L123B
L125B:
        pop ax
        mov byte ptr cs:[gSpriteRows], al
        shl bp, 1
L1262:
        call _BlitSpriteRowCga2
        dec byte ptr cs:[gSpriteRows]
        je L1282
        call _BlitSpriteRowCga2
        dec byte ptr cs:[gSpriteRows]
        je L1282
        add si, word ptr cs:[gSpriteStride]
        dec byte ptr cs:[gSpriteRows]
        jne L1262
L1282:
        pop es
        pop ds
        pop si
        pop di
        pop dx
        pop cx
        pop bx
        pop bp
        retf
_BlitSpriteCga ENDP

; ---- 128B..12EC
; 17A6:128B: one row of BlitSpriteCga: copy CS:0FF4 groups through the table at
; CS:10CD, then the remainder. Called from 17A6:123B..1259.
        PUBLIC  _BlitSpriteRowCga              ; @ 17A6:128B
_BlitSpriteRowCga PROC NEAR
        cmp di, bp
        jb L1295
        add si, word ptr cs:[gSpriteStride]
        ret
L1295:
        mov cx, word ptr cs:[gSpriteGroups]
        mov bx, 10CDh
L129D:
        lodsb
        xlatb cs:[bx]
        mov ah, al
        lodsb
        shl al, 1
        rcl ah, 1
        shl al, 1
        rcl ah, 1
        xchg ah, al
        stosb
        shl ah, 1
        shl ah, 1
        lodsb
        shr al, 1
        shr al, 1
        and al, 0Fh
        or al, ah
        stosb
        loop L129D
        cmp word ptr cs:[gSpriteRemainder], 1
        jne L12D2
        lodsb
        xlatb cs:[bx]
        shl al, 1
        shl al, 1
        stosb
        xor ax, ax
        stosb
        ret
L12D2:
        cmp word ptr cs:[gSpriteRemainder], 2
        jne L12EB
        lodsb
        xlatb cs:[bx]
        mov ah, al
        lodsb
        shl al, 1
        rcl ah, 1
        shl al, 1
        rcl ah, 1
        xchg ah, al
        stosw
L12EB:
        ret
_BlitSpriteRowCga ENDP

; ---- 12EC..1352
; 17A6:12EC: the second row variant of BlitSpriteCga, with the mask bits set.
; Called from 17A6:1262..1280.
        PUBLIC  _BlitSpriteRowCga2              ; @ 17A6:12EC
_BlitSpriteRowCga2 PROC NEAR
        cmp di, bp
        jb L12F6
        add si, word ptr cs:[gSpriteStride]
        ret
L12F6:
        mov cx, word ptr cs:[gSpriteGroups]
        mov bx, 10CDh
L12FE:
        lodsb
        xlatb cs:[bx]
        mov ah, al
        lodsb
        shl al, 1
        rcl ah, 1
        shl al, 1
        rcl ah, 1
        xchg ah, al
        stosb
        shl ah, 1
        shl ah, 1
        lodsb
        shr al, 1
        shr al, 1
        and al, 0Fh
        or al, ah
        stosb
        loop L12FE
        cmp word ptr cs:[gSpriteRemainder], 1
        jne L1335
        lodsb
        xlatb cs:[bx]
        shl al, 1
        shl al, 1
        or al, 3
        stosb
        mov al, 0FFh
        stosb
        ret
L1335:
        cmp word ptr cs:[gSpriteRemainder], 2
        jne L1351
        lodsb
        xlatb cs:[bx]
        mov ah, al
        lodsb
        shl al, 1
        rcl ah, 1
        shl al, 1
        rcl ah, 1
        xchg ah, al
        or ah, 3
        stosw
L1351:
        ret
_BlitSpriteRowCga2 ENDP

; ---- data 1352..15F5
; 17A6:1352: scratch of BlitSpriteEga (17A6:1715) and its row helper (17A6:179E).
; gSpriteHeight is the row count, gSpriteDst the destination offset, gSpritePlaneUsed the
; 16 plane flags and gSpritePlaneIndex the 16 plane indices; the 640 zero bytes are the
; 16 records of 40 bytes that gSpritePlanePtrs points into.
gSpriteHeight db 0
gSpriteDst dw 0
gSpritePlaneUsed db 16 dup (0)
gSpritePlaneIndex db 16 dup (0)
        db      640 dup (0)
; ---- data 15F5..1615
; 17A6:15F5: 16 near pointers to the 40-byte records in the zero buffer at 17A6:1375..15F4,
; one per plane; BlitSpriteEga (17A6:1715) and its row helper (17A6:179E) index it by plane.
gSpritePlanePtrs:
        dw      1375h, 139Dh, 13C5h, 13EDh, 1415h, 143Dh, 1465h, 148Dh, 14B5h, 14DDh, 1505h, 152Dh, 1555h, 157Dh, 15A5h, 15CDh
; ---- data 1615..1715
; 17A6:1615: 256-byte table used by the EGA sprite row (17A6:179E): entry i is
; (((i >> 3) & 3) << 2) | (i & 3), so each group of four entries repeats twice and the
; pattern restarts every 32 bytes.
gTable1615:
        db      00h, 01h, 02h, 03h, 00h, 01h, 02h, 03h, 04h, 05h, 06h, 07h, 04h, 05h, 06h, 07h
        db      08h, 09h, 0Ah, 0Bh, 08h, 09h, 0Ah, 0Bh, 0Ch, 0Dh, 0Eh, 0Fh, 0Ch, 0Dh, 0Eh, 0Fh
        db      00h, 01h, 02h, 03h, 00h, 01h, 02h, 03h, 04h, 05h, 06h, 07h, 04h, 05h, 06h, 07h
        db      08h, 09h, 0Ah, 0Bh, 08h, 09h, 0Ah, 0Bh, 0Ch, 0Dh, 0Eh, 0Fh, 0Ch, 0Dh, 0Eh, 0Fh
        db      10h, 11h, 12h, 13h, 10h, 11h, 12h, 13h, 14h, 15h, 16h, 17h, 14h, 15h, 16h, 17h
        db      18h, 19h, 1Ah, 1Bh, 18h, 19h, 1Ah, 1Bh, 1Ch, 1Dh, 1Eh, 1Fh, 1Ch, 1Dh, 1Eh, 1Fh
        db      10h, 11h, 12h, 13h, 10h, 11h, 12h, 13h, 14h, 15h, 16h, 17h, 14h, 15h, 16h, 17h
        db      18h, 19h, 1Ah, 1Bh, 18h, 19h, 1Ah, 1Bh, 1Ch, 1Dh, 1Eh, 1Fh, 1Ch, 1Dh, 1Eh, 1Fh
        db      20h, 21h, 22h, 23h, 20h, 21h, 22h, 23h, 24h, 25h, 26h, 27h, 24h, 25h, 26h, 27h
        db      28h, 29h, 2Ah, 2Bh, 28h, 29h, 2Ah, 2Bh, 2Ch, 2Dh, 2Eh, 2Fh, 2Ch, 2Dh, 2Eh, 2Fh
        db      20h, 21h, 22h, 23h, 20h, 21h, 22h, 23h, 24h, 25h, 26h, 27h, 24h, 25h, 26h, 27h
        db      28h, 29h, 2Ah, 2Bh, 28h, 29h, 2Ah, 2Bh, 2Ch, 2Dh, 2Eh, 2Fh, 2Ch, 2Dh, 2Eh, 2Fh
        db      30h, 31h, 32h, 33h, 30h, 31h, 32h, 33h, 34h, 35h, 36h, 37h, 34h, 35h, 36h, 37h
        db      38h, 39h, 3Ah, 3Bh, 38h, 39h, 3Ah, 3Bh, 3Ch, 3Dh, 3Eh, 3Fh, 3Ch, 3Dh, 3Eh, 3Fh
        db      30h, 31h, 32h, 33h, 30h, 31h, 32h, 33h, 34h, 35h, 36h, 37h, 34h, 35h, 36h, 37h
        db      38h, 39h, 3Ah, 3Bh, 38h, 39h, 3Ah, 3Bh, 3Ch, 3Dh, 3Eh, 3Fh, 3Ch, 3Dh, 3Eh, 3Fh
; ---- 1715..179E
; 17A6:1715: unpack a sprite from the far pointer at [bp+6]/[bp+8] to [bp+0A]/[bp+0C]
; for the EGA mode (gGraphicsChoice 2). Called from overlay 4 (ovl4:0EEC).
        PUBLIC  _BlitSpriteEga              ; @ 17A6:1715
_BlitSpriteEga PROC FAR
        push bp
        mov bp, sp
        push bx
        push cx
        push dx
        push di
        push si
        push ds
        push es
        mov si, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        mov di, word ptr [bp+000Ah]
        mov ax, word ptr [bp+000Ch]
        mov es, ax
        mov ds, dx
        xor ax, ax
        movsb
        lodsb
        mov byte ptr cs:[gSpriteRows], al
        shl ax, 1
        xor dx, dx
        mov bx, 3
        div bx
        mov cl, al
        stosb
        xor ax, ax
        lodsb
        mov word ptr cs:[gSpriteWidth], ax
        shl ax, 1
        xor dx, dx
        div bx
        or dl, dl
        je L1755
        inc ax
L1755:
        stosb
        mov byte ptr cs:[gSpriteHeight], al
        mov al, cl
        stosb
        inc si
        mov ax, word ptr cs:[gSpriteWidth]
        xor dx, dx
        div bx
        mov word ptr cs:[gSpriteGroups], ax
        mov word ptr cs:[gSpriteRemainder], dx
L176F:
        call _BlitSpriteRowEga
        dec byte ptr cs:[gSpriteRows]
        je L1790
        call _BlitSpriteRowEga
        dec byte ptr cs:[gSpriteRows]
        je L1790
L1783:
        lodsw
        test ah, 40h
        je L1783
        dec byte ptr cs:[gSpriteRows]
        jne L176F
L1790:
        or byte ptr es:[di-1], 20h
        pop es
        pop ds
        pop si
        pop di
        pop dx
        pop cx
        pop bx
        pop bp
        retf
_BlitSpriteEga ENDP

; ---- 179E..18E6
; 17A6:179E: one row of BlitSpriteEga: build the plane masks in CS:1355/1365 and copy
; the row through the table at CS:1615. Called from 17A6:176F..178E.
        PUBLIC  _BlitSpriteRowEga              ; @ 17A6:179E
_BlitSpriteRowEga PROC NEAR
        mov bp, 1355h
        mov cx, 10h
        xor ax, ax
L17A6:
        mov word ptr cs:[bp], ax
        add bp, 2
        loop L17A6
        mov word ptr cs:[gSpriteDst], di
        xor dx, dx
        mov bx, dx
        mov cx, bx
L17BA:
        lodsw
        or al, al
        je L1800
        mov dl, ah
        and dl, 0Fh
        mov bp, dx
        cmp byte ptr cs:[bp+gSpritePlaneUsed], 0
        jne L17F6
        inc byte ptr cs:[bp+gSpritePlaneUsed]
        shl bp, 1
        mov di, word ptr cs:[bp+gSpritePlanePtrs]
        mov bp, cx
        mov byte ptr cs:[bp+gSpritePlaneIndex], dl
        mov cx, word ptr cs:[gSpriteWidth]
L17E6:
        mov byte ptr cs:[di], dh
        inc di
        loop L17E6
        sub di, word ptr cs:[gSpriteWidth]
        mov cx, bp
        inc cx
        jmp short L17FD
L17F6:
        shl bp, 1
        mov di, word ptr cs:[bp+gSpritePlanePtrs]
L17FD:
        mov byte ptr cs:[bx+di], al
L1800:
        shl ah, 1
        jae L17BA
        inc bx
        shl ah, 1
        jae L17BA
        push si
        xor dx, dx
L180C:
        mov bp, dx
        cmp byte ptr cs:[bp+gSpritePlaneUsed], 0
        jne L181E
L1816:
        inc dx
        cmp dx, 10h
        jne L180C
        jmp short L1890
L181E:
        shl bp, 1
        mov si, word ptr cs:[bp+gSpritePlanePtrs]
        mov di, si
        push ds
        push es
        mov cx, cs
        mov es, cx
        mov ds, cx
        mov cx, word ptr cs:[gSpriteGroups]
        mov bx, 1615h
L1837:
        lodsw
        xlatb cs:[bx]
        xchg ah, al
        shl al, 1
        shl al, 1
        rcl ah, 1
        shl al, 1
        rcl ah, 1
        xchg ah, al
        stosb
        shl ah, 1
        and ah, 0E0h
        lodsb
        shr al, 1
        xlatb cs:[bx]
        or al, ah
        stosb
        loop L1837
        cmp word ptr cs:[gSpriteRemainder], 1
        jne L1871
        lodsb
        shr al, 1
        xlatb cs:[bx]
        shl al, 1
        shl al, 1
        shl al, 1
        stosb
        xor ax, ax
        stosb
        jmp short L188C
L1871:
        cmp word ptr cs:[gSpriteRemainder], 2
        jne L188C
        lodsb
        xlatb cs:[bx]
        mov ah, al
        lodsb
        shl al, 1
        shl al, 1
        rcl ah, 1
        shl al, 1
        rcl ah, 1
        xchg ah, al
        stosw
L188C:
        pop es
        pop ds
        jmp short L1816
L1890:
        mov di, word ptr cs:[gSpriteDst]
        xor cx, cx
        mov cl, byte ptr cs:[gSpriteHeight]
        xor bx, bx
L189E:
        xor dx, dx
L18A0:
        mov bp, dx
        and bp, 0Fh
        mov al, byte ptr cs:[bp+gSpritePlaneIndex]
        or al, al
        je L18D0
        xor ah, ah
        shl ax, 1
        mov bp, ax
        mov si, word ptr cs:[bp+gSpritePlanePtrs]
        mov ah, al
        mov al, byte ptr cs:[bx+si]
        or al, al
        je L18CA
        stosb
        mov al, ah
        shr al, 1
        stosb
        inc dh
L18CA:
        inc dx
        cmp dl, 10h
        jne L18A0
L18D0:
        or dh, dh
        jne L18D7
        xor ax, ax
        stosw
L18D7:
        or byte ptr es:[di-1], 80h
        inc bx
        loop L189E
        or byte ptr es:[di-1], 40h
        pop si
        ret
_BlitSpriteRowEga ENDP

; ---- data 18E6..18EA
; 17A6:18E6: the two table offsets BuildMask (17A6:18EA) computes from ds:80CE and ds:80D0.
gMaskTableOff dw 0
gMaskTableOff2 dw 0
; ---- 18EA..197E
; 17A6:18EA: build a mask from the image at the far pointer [bp+6]/[bp+8] and write it
; back: for each pair of bytes, translate through one of two 256-byte tables in segment
; ds:80D0, at ds:80CE + 190h and ds:80CE + 290h (the L1917 setup), chosen by bit 0 of dh
; (the `xor dh, 1` after L1966), and set the mask bits. Called from overlay 3 (ovl3:0934).
        PUBLIC  _BuildMask              ; @ 17A6:18EA
_BuildMask PROC FAR
        push bp
        mov bp, sp
        push bx
        push cx
        push dx
        push di
        push si
        push ds
        push es
        mov si, word ptr [bp+6]
        mov es, word ptr [bp+8]
        mov cx, 6590h
        xor dx, dx
        mov bx, word ptr [gRowOffsetTableOff]
        add bx, 190h
        mov word ptr cs:[gMaskTableOff], bx
        inc bh
        mov word ptr cs:[gMaskTableOff2], bx
        mov ds, word ptr [gRowOffsetTableSeg]
L1917:
        mov bx, word ptr cs:[gMaskTableOff]
L191C:
        mov dl, 2
L191E:
        mov al, byte ptr es:[si]
        xlatb
        shl al, 1
        shl al, 1
        shl al, 1
        shl al, 1
        mov ah, al
        mov al, byte ptr es:[si+1]
        xlatb
        or al, ah
        xchg al, byte ptr es:[si]
        inc si
        xor ah, ah
        test al, 0F0h
        je L1940
        or ah, 0C0h
L1940:
        test al, 0Fh
        je L1947
        or ah, 30h
L1947:
        mov al, byte ptr es:[si]
        test al, 0F0h
        je L1951
        or ah, 0Ch
L1951:
        test al, 0Fh
        je L1958
        or ah, 3
L1958:
        not ah
        mov byte ptr es:[si], ah
        inc si
        dec dl
        je L1966
        loop L191E
        jmp short L1975
L1966:
        dec cx
        je L1975
        xor dh, 1
        je L1917
        mov bx, word ptr cs:[gMaskTableOff2]
        jmp short L191C
L1975:
        pop es
        pop ds
        pop si
        pop di
        pop dx
        pop cx
        pop bx
        pop bp
        retf
_BuildMask ENDP

; ---- 197E..1A26
; 17A6:197E: copy an image from the far pointer at [bp+6]/[bp+8] to [bp+0A]/[bp+0C]
; through the translation table at [bp+0E]/[bp+10], the mode word at [bp+12] choosing the
; pixel format: mode 0 translates count * 2 bytes (the `shl ax, 1` before L19BA), mode 2
; copies two words and then a translated stream until a value has the 20h bit (L19EB), mode 3
; copies 8 words and then count * 2 words (L19FB), and every other mode translates count * 4
; bytes (the two `shl ax, 1` before L19E0). Called from SetMouseItem (07A9:010C), call at
; 07A9:018F, to translate the mouse icon; the name comes from that use.
        PUBLIC  _TranslateImage              ; @ 17A6:197E
_TranslateImage PROC FAR
        push bp
        mov bp, sp
        push ax
        push bx
        push cx
        push dx
        push si
        push di
        push ds
        push es
        mov si, word ptr [bp+6]
        mov ds, word ptr [bp+8]
        mov di, word ptr [bp+000Ah]
        mov es, word ptr [bp+000Ch]
        mov bx, word ptr [bp+000Eh]
        mov dx, word ptr [bp+0010h]
        mov ax, word ptr [bp+0012h]
        cmp ax, 3
        je L19FB
        cmp ax, 0
        jne L19C8
        movsw
        lodsw
        stosw
        push dx
        mov dl, ah
        xor ah, ah
        mov dh, ah
        mul dx
        shl ax, 1
        mov cx, ax
        pop dx
        push cx
L19BA:
        lodsb
        push ds
        mov ds, dx
        xlatb
        pop ds
        stosb
        loop L19BA
        pop cx
        rep movsb
        jmp short L1A1C
L19C8:
        cmp ax, 2
        je L19EB
        movsw
        lodsw
        stosw
        push dx
        mov dl, ah
        xor ah, ah
        mov dh, ah
        mul dx
        shl ax, 1
        shl ax, 1
        mov cx, ax
        pop dx
L19E0:
        lodsb
        push ds
        mov ds, dx
        xlatb
        pop ds
        stosb
        loop L19E0
        jmp short L1A1C
L19EB:
        movsw
        movsw
L19ED:
        movsb
        lodsb
        push ds
        mov ds, dx
        xlatb
        pop ds
        stosb
        test al, 20h
        je L19ED
        jmp short L1A1C
L19FB:
        movsw
        lodsw
        stosw
        push ds
        mov ds, dx
        xchg si, bx
        mov cx, 8
        rep movsw
        pop ds
        mov si, bx
        add si, 10h
        mov dl, ah
        xor ah, ah
        mov dh, ah
        mul dx
        shl ax, 1
        mov cx, ax
        rep movsw
L1A1C:
        pop es
        pop ds
        pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
        pop bp
        retf
_TranslateImage ENDP

        END
