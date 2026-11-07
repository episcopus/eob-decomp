; Module d1aad of EOB.EXE: data only, ds:802A to ds:80EC (194 bytes, 7 relocations), and the empty code
; segment at 1AAD:000C (TLINK's segment table lists it). The graphics driver slot table, ds:802A to
; ds:80E5: 47 far pointers, slots 0 to 6 the game's imports (gCurFont, gPageSegs, gDrawPage, MemAlloc,
; MemFree, _GetFarPointer, _SetFarPointer) and slots 7 to 46 the driver's exports, zero in the image,
; which LinkDriver writes when RunOverlayRequest (1C00:0055) loads a driver (whitepaper 2.5, the driver
; interface); then gSoundOn, gTunesOn and gKeyboardOptions. Assembled: its data relocations rise in the
; relocation table, as TASM writes a module's fixups (Turbo C writes them downward; playbook 80, step D).
; Drafted by make data-skeleton as step D1's pilot; the names other files of src/ declare are used, the
; rest are named by their address until step D2 names them.
.MODEL LARGE
; @data
        EXTRN   _GetFarPointer:FAR ; @ 1A72:033A
        EXTRN   _MemAlloc:FAR ; @ 19BC:0007
        EXTRN   _MemFree:FAR ; @ 19BC:0137
        EXTRN   _SetFarPointer:FAR ; @ 1A72:02FE
        EXTRN   _gCurFont:BYTE ; @ ds:FE4E
        EXTRN   _gDrawPage:BYTE ; @ ds:8115
        EXTRN   _gPageSegs:BYTE ; @ ds:FC0C

.DATA

g802A label byte ; ds:802A, to name from its readers
        dd      _gCurFont
        dd      _gPageSegs
        dd      _gDrawPage
        dd      _MemAlloc
        dd      _MemFree
        dd      _GetFarPointer
        dd      _SetFarPointer

        PUBLIC  _gCopyPage ; @ ds:8046
_gCopyPage label byte
        dd      0

        PUBLIC  _gRestoreRectToPage ; @ ds:804A
_gRestoreRectToPage label byte
        dd      0

        PUBLIC  _gRestoreRect ; @ ds:804E
_gRestoreRect label byte
        dd      0

        PUBLIC  _gCopyRegion ; @ ds:8052
_gCopyRegion label byte
        dd      0

        PUBLIC  _gClearPage ; @ ds:8056
_gClearPage label byte
        dd      0

        PUBLIC  _gDrawCell8x8 ; @ ds:805A
_gDrawCell8x8 label byte
        dd      0

        PUBLIC  _gDrawIndexedTile ; @ ds:805E
_gDrawIndexedTile label byte
        dd      0

        PUBLIC  _gDrawLine ; @ ds:8062
_gDrawLine label byte
        dd      0

        PUBLIC  _gXorRect ; @ ds:8066
_gXorRect label byte
        dd      0

        PUBLIC  _gFillRect ; @ ds:806A
_gFillRect label byte
        dd      0, 0

        PUBLIC  _gGetPixel ; @ ds:8072
_gGetPixel label byte
        dd      0, 0, 0

        PUBLIC  _gSaveRectFromPage ; @ ds:807E
_gSaveRectFromPage label byte
        dd      0

        PUBLIC  _gGetSpriteCellRle ; @ ds:8082
_gGetSpriteCellRle label byte
        dd      0

        PUBLIC  _gGetSpriteCell ; @ ds:8086
_gGetSpriteCell label byte
        dd      0, 0

        PUBLIC  _gCopyToPlanar ; @ ds:808E
_gCopyToPlanar label byte
        dd      0, 0

        PUBLIC  _gDrawShapeMaskedFlipped ; @ ds:8096
_gDrawShapeMaskedFlipped label byte
        dd      0

        PUBLIC  _gDrawShapeMasked ; @ ds:809A
_gDrawShapeMasked label byte
        dd      0

        PUBLIC  _gPutPixel ; @ ds:809E
_gPutPixel label byte
        dd      0

        PUBLIC  _gRestoreDrawState ; @ ds:80A2
_gRestoreDrawState label byte
        dd      0

        PUBLIC  _gDrawShapeFlipped ; @ ds:80A6
_gDrawShapeFlipped label byte
        dd      0

        PUBLIC  _gSaveDrawState ; @ ds:80AA
_gSaveDrawState label byte
        dd      0

        PUBLIC  _gSaveRect ; @ ds:80AE
_gSaveRect label byte
        dd      0, 0

        PUBLIC  _gSetRegion ; @ ds:80B6
_gSetRegion label byte
        dd      0

        PUBLIC  _gSetPalette ; @ ds:80BA
_gSetPalette label byte
        dd      0, 0

        PUBLIC  _gDrawShape ; @ ds:80C2
_gDrawShape label byte
        dd      0

        PUBLIC  _gSaveRectSize ; @ ds:80C6
_gSaveRectSize label byte
        dd      0

        PUBLIC  _gDrawText ; @ ds:80CA
_gDrawText label byte
        dd      0

        PUBLIC  _gRowOffsetTableOff ; @ ds:80CE
_gRowOffsetTableOff label byte
        dw      0

        PUBLIC  _gRowOffsetTableSeg ; @ ds:80D0
_gRowOffsetTableSeg label byte
        dw      0

        PUBLIC  _gSetRemapTable ; @ ds:80D2
_gSetRemapTable label byte
        dd      0

        PUBLIC  _gDrawShapeRemapped ; @ ds:80D6
_gDrawShapeRemapped label byte
        dd      0

        PUBLIC  _gDrawShapeFlippedRemapped ; @ ds:80DA
_gDrawShapeFlippedRemapped label byte
        dd      0, 0

        PUBLIC  _gRemapTable ; @ ds:80E2
_gRemapTable label byte
        dd      0

        PUBLIC  _gSoundOn ; @ ds:80E6
_gSoundOn label byte
        dw      1

        PUBLIC  _gTunesOn ; @ ds:80E8
_gTunesOn label byte
        dw      1

        PUBLIC  _gKeyboardOptions ; @ ds:80EA
_gKeyboardOptions label byte
        dw      0110h

.CODE
; no code: the module's empty code segment (the original's segment table lists it)
END
