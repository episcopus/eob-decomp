; Unit at START1.EXE 0000:0239 (827 bytes): the video-hardware probes Main calls before the menu. The
; functions after the string printer have no frame and pass the four-byte info record in DI and their
; value in AX, so they are assembly: InitVideo (052D) zeroes the record, runs the four probe routines
; through the table at ds:0334, and normalises the record; StoreVideoWord (0496) writes AX into the
; record's first zero word; TestCrtcRegister (04A3) tests one CRTC register through the port in DX and
; returns carry set on failure; DetectVideoAdapter (04BD) uses 3B4/3BA to tell MDA from CGA/EGA and
; calls StoreVideoWord; NormalizeVideoInfo (04FC) swaps the record's two words when the BIOS mode
; disagrees with byte +1. The probes at 056A, 05BA and 05E6 (reached only through the table, and past
; the unit's end) are not written here. Assembled by TASM 2.0, small model, DS = DGROUP.

        .MODEL  SMALL

        .DATA
        EXTRN   gProbeTable:BYTE        ; @ ds:0334: four {enable, routine} records, 3 bytes each
        EXTRN   gProbeFlag1:BYTE        ; @ ds:0337: enable byte of the second record
        EXTRN   gProbeFlag2:BYTE        ; @ ds:033A: enable byte of the third record
        EXTRN   gProbeFlag3:BYTE        ; @ ds:033D: enable byte of the fourth record

        .CODE

; 0000:0496: write AX into the info record at DI: the first word when it is zero, else the second.
; DI is the caller's record pointer and AX the value, both registers (no frame). Called by the
; adapter probes at 04BD, 05BA and 05E6.
        PUBLIC  StoreVideoWord          ; @ 0000:0496
StoreVideoWord PROC NEAR
        lea     bx,[di]
        cmp     byte ptr [bx],0
        je      L_04A0
        lea     bx,[di+2]
L_04A0:
        mov     word ptr [bx],ax
        ret
StoreVideoWord ENDP

; 0000:04A3: probe one CRTC register: write index 0Fh to the port in DX, read it back from DX+1 into
; AH, write 66h, wait 100h loops, read the port, write the saved value back, and return carry set
; unless the read-back was 66h. The port base is in DX, the value in AL/AH; no frame. Called by
; DetectVideoAdapter (04BD) and the VGA probe (05E6).
        PUBLIC  TestCrtcRegister        ; @ 0000:04A3
TestCrtcRegister PROC NEAR
        mov     al,0fh
        out     dx,al
        inc     dx
        in      al,dx
        mov     ah,al
        mov     al,66h
        out     dx,al
        mov     cx,100h
L_04B0:
        loop    L_04B0
        in      al,dx
        xchg    ah,al
        out     dx,al
        cmp     ah,66h
        je      L_04BC
        stc
L_04BC:
        ret
TestCrtcRegister ENDP

; 0000:04BD: tell the video adapter from the MDA status port: test the CRTC at 3B4h (carry set means
; no MDA), then sample bit 7 of port 3BAh 8000h times; a steady value means MDA and stores 0101h,
; otherwise the adapter is CGA or EGA and stores 0080h, 0180h or 0283h by the value read. DI is the
; info record, no frame. Called by InitVideo (052D) through the table at ds:0334.
        PUBLIC  DetectVideoAdapter      ; @ 0000:04BD
DetectVideoAdapter PROC NEAR
        mov     dx,3b4h
        call    TestCrtcRegister
        jb      L_04FB
        mov     dl,0bah
        in      al,dx
        and     al,80h
        mov     ah,al
        mov     cx,8000h
L_04CF:
        in      al,dx
        and     al,80h
        cmp     ah,al
        loope   L_04CF
        jne     L_04E1
        mov     al,1
        mov     ah,1
        call    StoreVideoWord
        jmp     short L_04FB
L_04E1:
        in      al,dx
        mov     dl,al
        mov     ah,1
        mov     al,80h
        and     dl,70h
        je      L_04F8
        mov     al,81h
        cmp     dl,10h
        je      L_04F8
        mov     al,82h
        mov     ah,3
L_04F8:
        call    StoreVideoWord
L_04FB:
        ret
DetectVideoAdapter ENDP

; 0000:04FC: normalise the info record at DI against the BIOS mode: do nothing when the second word
; is zero or either word's low byte is 4 or more; otherwise read the mode with INT 10h AH=0Fh and,
; when the mode is 7 (MDA) the record's byte +1 must not be 1, else it must be 1, and the two words
; are swapped. No frame; DI is the record. Called by InitVideo (052D).
        PUBLIC  NormalizeVideoInfo      ; @ 0000:04FC
NormalizeVideoInfo PROC NEAR
        cmp     word ptr [di+2],0
        je      L_052C
        cmp     byte ptr [di],4
        jge     L_052C
        cmp     byte ptr [di+2],4
        jge     L_052C
        mov     ah,0fh
        int     10h
        and     al,7
        cmp     al,7
        je      L_051F
        cmp     byte ptr [di+1],1
        jne     L_052C
        jmp     short L_0525
L_051F:
        cmp     byte ptr [di+1],1
        je      L_052C
L_0525:
        mov     ax,[di]
        xchg    word ptr [di+2],ax
        mov     [di],ax
L_052C:
        ret
NormalizeVideoInfo ENDP

; 0000:052D: fill the four-byte video info record at the argument (zero both words), enable the
; second, third and fourth records of the probe table at ds:0334, run every enabled probe in order
; (each record is {enable byte, near routine offset}), then normalise the record. Takes the record
; pointer at [bp+4]; called by Main (0000:0239) with the address of gDetectedMode.
        PUBLIC  InitVideo               ; @ 0000:052D
InitVideo PROC NEAR
        push    bp
        mov     bp,sp
        push    si
        push    di
        mov     di,[bp+4]
        mov     word ptr [di],0
        mov     word ptr [di+2],0
        mov     byte ptr [gProbeFlag2],1
        mov     byte ptr [gProbeFlag1],1
        mov     byte ptr [gProbeFlag3],1
        mov     cx,4
        mov     si,offset gProbeTable
L_0553:
        lodsb
        test    al,al
        lodsw
        je      L_055F
        push    si
        push    cx
        call    ax
        pop     cx
        pop     si
L_055F:
        loop    L_0553
        call    NormalizeVideoInfo
        pop     di
        pop     si
        mov     sp,bp
        pop     bp
        ret
InitVideo ENDP

        END
