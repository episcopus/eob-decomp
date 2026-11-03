; Unit at START1.EXE 0000:0239 (1097 bytes, two modules; the C module is src/S_TEXT.C): this is the
; assembly module, the video probes (0496 to 05F5) and the console and mouse routines (05F6 to 0681).
; The probes take the four-byte info record in DI and a value in AX; only InitVideo (052D) sets up a
; frame (it reads the record pointer at [bp+4]). StoreVideoWord (0496) writes AX into the record's
; first word when its low byte is zero; TestCrtcRegister (04A3) tests CRTC register 0Fh through the
; port base in DX and returns carry set on failure; DetectVideoAdapter (04BD) uses 3B4/3BA to tell
; MDA from CGA/EGA and calls StoreVideoWord; NormalizeVideoInfo (04FC) swaps the record's two words
; when the BIOS mode disagrees with byte +1; InitVideo (052D) zeroes the record, enables probe records
; 2 to 4 of the table at ds:0334 (record 1, the VGA probe ProbeVga at 056A, is already enabled in the
; data and runs first), runs every enabled probe and normalises. The other three routines the table
; reaches are ProbeVga (056A), ProbeEga (05BA) and ProbeColorCrtc (05E6). The console and mouse
; routines PutChar (05F6), GetKey (0608), Int2FHandler (0620), InstallInt2FHandler (062C) and
; DetectMouse (0642) set up a frame; PutChar reads its character at [bp+4], the rest take no argument.
; Assembled by TASM 2.0, small model, DS = DGROUP.

        .MODEL  SMALL

        .DATA
        EXTRN   gProbeTable:BYTE        ; @ ds:0334: four {enable, routine} records, 3 bytes each
        EXTRN   gProbeFlag1:BYTE        ; @ ds:0337: enable byte of the second record
        EXTRN   gProbeFlag2:BYTE        ; @ ds:033A: enable byte of the third record
        EXTRN   gProbeFlag3:BYTE        ; @ ds:033D: enable byte of the fourth record

; DGROUP tables the probes index, before the probe table (not declared elsewhere in src/):
;   ds:0314 is a byte table the EGA probe reads with XLATB, indexed by the configuration byte >> 1
;   ds:031A is a word table the VGA probe indexes by a display code: 0000h, 0101h, 0202h, 0000h, 0303h
EGA_CONFIG_TABLE   equ 0314h
DISPLAY_CODE_TABLE equ 031Ah

        .CODE

; 0000:0496: write AX into the info record at DI: the first word when its low byte is zero, else the
; second word. DI is the caller's record pointer and AX the value, both registers (no frame). Called
; by DetectVideoAdapter (04BD), ProbeEga (05BA) and ProbeColorCrtc (05E6).
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

; 0000:04A3: probe CRTC register 0Fh: write 0Fh to the index port in DX, read that register back from
; DX+1, write 66h to it, wait 100h loops and read it back, then write the first read-back back to
; restore the register and return carry set unless the second read-back is 66h. DX holds the CRTC
; base, AL the first read-back (written back at the end), AH the second read-back (tested against
; 66h); no frame. Called by DetectVideoAdapter (04BD) and ProbeColorCrtc (05E6).
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
; no MDA), then sample bit 7 of port 3BAh 8000h times; a steady value means MDA [H: the adapter
; identification is an inference from the port and the status bits, not checked against hardware] and
; stores AX = 0101h; otherwise the adapter is CGA or EGA [H] and stores AX = 0180h when the sampled
; status bits 70h are zero, 0181h when they are 10h, and 0382h otherwise. DI is the info record, no
; frame. Called by InitVideo (052D) through the table at ds:0334.
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
; is zero, or either word's low byte is 4 to 127 (the compares are signed, `jge`): a low byte of 80h
; to FFh is negative, so the 80h, 81h and 82h DetectVideoAdapter stores do reach the mode test.
; Otherwise read the mode with INT 10h AH=0Fh, take its low three bits (and al,7) and swap the two
; words when the mode is 7 and byte +1 is not 1, or the mode is not 7 and byte +1 is 1. No frame; DI
; is the record. Called by InitVideo (052D).
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

; 0000:056A: VGA probe (record 1 of the probe table, already enabled in the data, so it runs first):
; ask the BIOS for the display combination (INT 10h AX=1A00h); when it answers (AL=1Ah) store the
; primary and secondary display codes from the word table at ds:031A into the record, disable the
; other three probes, and when a stored code's low byte is 1 (VGA) zero that word and enable the
; fourth probe. DI is the info record, no frame. Called by InitVideo (052D) through the table.
        PUBLIC  ProbeVga                ; @ 0000:056A
ProbeVga PROC NEAR
        mov     ax,1a00h
        int     10h
        cmp     al,1ah
        jne     L_05B9
        mov     cx,bx
        xor     bh,bh
        or      ch,ch
        je      L_058A
        mov     bl,ch
        add     bx,bx
        mov     ax,[bx+DISPLAY_CODE_TABLE]
        mov     [di+2],ax
        mov     bl,cl
        xor     bh,bh
L_058A:
        add     bx,bx
        mov     ax,[bx+DISPLAY_CODE_TABLE]
        mov     [di],ax
        mov     byte ptr [gProbeFlag2],0
        mov     byte ptr [gProbeFlag1],0
        mov     byte ptr [gProbeFlag3],0
        lea     bx,[di]
        cmp     byte ptr [bx],1
        je      L_05B0
        lea     bx,[di+2]
        cmp     byte ptr [bx],1
        jne     L_05B9
L_05B0:
        mov     word ptr [bx],0
        mov     byte ptr [gProbeFlag3],1
L_05B9:
        ret
ProbeVga ENDP

; 0000:05BA: EGA probe (record 2): ask the BIOS for the EGA configuration (INT 10h AH=12h, BL=10h);
; when BL changed an EGA is present: read the configuration byte CL, look up CL>>1 in the byte table
; at ds:0314, and store AX = (table value << 8) or 3 into the record through StoreVideoWord. When the
; stored high byte is 1 disable the fourth probe (ds:033D), else disable the third (ds:033A). DI is
; the info record, no frame. Called by InitVideo (052D) through the table.
        PUBLIC  ProbeEga                ; @ 0000:05BA
ProbeEga PROC NEAR
        mov     bl,10h
        mov     ah,12h
        int     10h
        cmp     bl,10h
        je      L_05E5
        mov     al,cl
        shr     al,1
        mov     bx,EGA_CONFIG_TABLE
        xlatb
        mov     ah,al
        mov     al,3
        call    StoreVideoWord
        cmp     ah,1
        je      L_05E0
        mov     byte ptr [gProbeFlag2],0
        jmp     short L_05E5
L_05E0:
        mov     byte ptr [gProbeFlag3],0
L_05E5:
        ret
ProbeEga ENDP

; 0000:05E6: colour CRTC probe (record 3): probe CRTC register 0Fh at the colour port 3D4h; when it
; succeeds (no carry) store AX = 0202h into the record through StoreVideoWord. DI is the info
; record, no frame. Called by InitVideo (052D) through the table.
        PUBLIC  ProbeColorCrtc          ; @ 0000:05E6
ProbeColorCrtc PROC NEAR
        mov     dx,3d4h
        call    TestCrtcRegister
        jb      L_05F5
        mov     al,2
        mov     ah,2
        call    StoreVideoWord
L_05F5:
        ret
ProbeColorCrtc ENDP

; 0000:05F6: PutChar: write the character at [bp+4] with DOS function 06h (direct console output) and
; return; no result. Main and PrintString call it. The frame saves DI, SI and DS, reproducing the
; original's register saves; why the DOS call needs them saved is not known.
        PUBLIC  PutChar                 ; @ 0000:05F6
PutChar PROC NEAR
        push    bp
        mov     bp,sp
        push    di
        push    si
        push    ds
        mov     dl,[bp+4]
        mov     ah,6
        int     21h
        pop     ds
        pop     si
        pop     di
        pop     bp
        ret
PutChar ENDP

; 0000:0608: GetKey: read one key with BIOS INT 16h AH=0 and return it as an int. An extended key
; (ASCII AL zero) returns the negated scan code; `cbw` then sign-extends AL, so the return is ASCII 1
; to 127 as it is, ASCII 128 to 255 negative, and an extended key as its negated scan code. The menu
; loop calls it. The frame saves DI, SI and DS.
        PUBLIC  GetKey                  ; @ 0000:0608
GetKey PROC NEAR
        push    bp
        mov     bp,sp
        push    di
        push    si
        push    ds
        xor     ah,ah
        int     16h
        or      al,al
        jne     L_061A
        mov     al,ah
        neg     al
L_061A:
        cbw
        pop     ds
        pop     si
        pop     di
        pop     bp
        ret
GetKey ENDP

; 0000:0620: Int2FHandler: an interrupt handler. It pushes BP, DI, SI and DS and then does iret; the
; iret takes the first three of those words as the return IP, CS and FLAGS, so it does not return to
; what interrupted it. InstallInt2FHandler sets it as the interrupt 2Fh vector; no call in START1.EXE
; reaches it (the code bytes contain no INT 2Fh, CD 2F). QUIRK 0000:0620: the frame's pops and ret
; after the iret are dead.
        PUBLIC  Int2FHandler            ; @ 0000:0620
Int2FHandler PROC NEAR
        push    bp
        mov     bp,sp
        push    di
        push    si
        push    ds
        iret
        pop     ds
        pop     si
        pop     di
        pop     bp
        ret
Int2FHandler ENDP

; 0000:062C: InstallInt2FHandler: set the interrupt 2Fh vector with DOS function 25h. That function
; takes the handler in DS:DX; the code puts offset Int2FHandler in DX (0000:0632 to 0636) but never
; sets DS, which holds DGROUP, so the vector it installs is DGROUP:0620h, a data address, not Int2FHandler
; at the code segment's 0620h. Purpose unknown: no caller in START1.EXE and no INT 2Fh in its code
; bytes. QUIRK 0000:062C: because of that DS:DX mismatch the handler is never reached.
        PUBLIC  InstallInt2FHandler     ; @ 0000:062C
InstallInt2FHandler PROC NEAR
        push    bp
        mov     bp,sp
        push    di
        push    si
        push    ds
        mov     ax,offset Int2FHandler
        mov     dx,ax
        mov     ah,25h
        mov     al,2fh
        int     21h
        pop     ds
        pop     si
        pop     di
        pop     bp
        ret
InstallInt2FHandler ENDP

; 0000:0642: DetectMouse: return 1 when a mouse driver is present, else 0. Read the INT 33h vector
; with DOS function 35h; a null vector or one whose first byte is iret (0CFh) means no driver;
; otherwise reset the driver (INT 33h function 0) and require AX = FFFFh. The caller gates the mouse
; prompt on it. The frame reproduces the original's register saves; why the DOS and BIOS calls need
; them saved is not known.
        PUBLIC  DetectMouse             ; @ 0000:0642
DetectMouse PROC NEAR
        push    bp
        mov     bp,sp
        push    di
        push    si
        push    ds
        push    bp
        push    bx
        push    cx
        push    di
        push    ds
        push    dx
        push    es
        push    si
        mov     ax,3533h
        int     21h
        mov     ax,es
        or      ax,bx
        je      L_0663
        mov     bl,es:[bx]
        cmp     bl,0cfh
        jne     L_0668
L_0663:
        mov     ax,0
        jmp     short L_0675
L_0668:
        mov     ax,0
        int     33h
        cmp     ax,0ffffh
        jne     L_0663
        mov     ax,1
L_0675:
        pop     si
        pop     es
        pop     dx
        pop     ds
        pop     di
        pop     cx
        pop     bx
        pop     bp
        pop     ds
        pop     si
        pop     di
        pop     bp
        ret
DetectMouse ENDP

        END
