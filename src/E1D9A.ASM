; Unit at EOB.EXE 1D9A:000B (1067 bytes, 8 functions): the four packed-file decoders the loader
; (E1B82.C Unpack, by the format byte) calls with segment arguments. 1D9A:0013 Unpack4 expands an
; LZ77-style stream (literal runs, back references, a word-count fill and a word-offset copy) to a
; fixed length; 1D9A:0113 Unpack3 expands a literal and fill stream to a 32-bit length; 1D9A:018B
; Unpack1 and 1D9A:02CB Unpack2 expand the two dictionary formats into a work segment, building an
; offset table as they go. Near helpers: 1D9A:0162 subtracts the emitted count and renormalizes
; ES:DI for Unpack3, 1D9A:0299 reads the 12-bit codes of format 1, 1D9A:03CC reads the 14-bit codes
; of format 2 through a four-state machine. The names Unpack1..4 come from their callers (E1B82.C);
; the rest are from the bytes. All eight entries end in a far return; the near helpers are internal.
;
; The scratch words the decoders keep in their own code segment are addressed by their absolute
; offsets (cs:[0Ch], cs:[10h], cs:[12h] at the unit start, cs:[185h]/[187h]/[189h] in Unpack1's
; range, cs:[2C9h] in Unpack2's), exactly as the original encodes them.

        .MODEL  LARGE

        EXTRN   _NormFar:FAR            ; @ 1BA8:0081 (NormalizeFar, E1BA8.ASM)

        .CODE

; 1D9A:000B: eight bytes of scratch, zero at load: the saved packed source segment and bit counter
; used by Unpack1's code reader, and the output limit used by Unpack1 and Unpack2.
        PUBLIC  _UnpackScratch
_UnpackScratch LABEL BYTE               ; @ 1D9A:000B
        dw      0,0,0,0

; 1D9A:0013: expand a stream to len bytes. src (far, [bp+6]) and dst (far, [bp+0Ah]) are normalized,
; the output start and end offsets are kept in cs:[109h]/cs:[10Dh]. Loop: a control byte with bit 7
; clear copies ((byte >> 4) + 3) bytes from (next byte) back in the output; 80h ends; 81h..0BFh copy
; (byte & 3Fh) bytes straight from the input; 0C0h..0FDh copy (byte & 3Fh) + 3 bytes from an absolute
; output offset after a word; 0FEh fills a word count of one byte; 0FFh copies a word count from a
; word offset. Every count is clamped to the bytes left. Returns the bytes written in AX.
        PUBLIC  _Unpack4                ; @ 1D9A:0013
_Unpack4 PROC FAR
        push    bp
        mov     bp,sp
        push    bx
        push    cx
        push    di
        push    ds
        push    dx
        push    es
        push    si
        mov     ax,[bp+6]
        mov     dx,[bp+8]
        call    far ptr _NormFar
        mov     si,ax
        mov     ds,dx
        mov     ax,[bp+0Ah]
        mov     dx,[bp+0Ch]
        call    far ptr _NormFar
        mov     di,ax
        mov     es,dx
        mov     dx,[bp+0Eh]
        mov     word ptr cs:[109h],di           ; output start offset
        add     dx,di
        mov     word ptr cs:[10Dh],dx           ; output end offset
        cld
        mov     bx,si
        mov     dx,ds
        mov     bp,es
u4_loop:
        mov     ax,word ptr cs:[10Dh]
        sub     ax,di
        je      u4_done
        mov     word ptr cs:[10Bh],ax           ; bytes left to fill
        mov     si,bx
        mov     ds,dx
        lodsb
        or      al,al
        js      u4_control
        mov     ah,al
        and     ah,0Fh
        mov     ch,al
        mov     cl,4
        shr     ch,cl
        mov     cl,ch
        xor     ch,ch
        add     cx,3
        cmp     cx,word ptr cs:[10Bh]
        jbe     u4_back
        mov     cx,word ptr cs:[10Bh]
u4_back:
        lodsb
        mov     bx,si
        mov     si,di
        sub     si,ax
        mov     ds,bp
        rep     movsb
        jmp     short u4_loop
u4_control:
        test    al,40h
        jne     u4_fill
        cmp     al,80h
        je      u4_done
        mov     cl,al
        and     cx,3Fh
        cmp     cx,word ptr cs:[10Bh]
        jbe     u4_copy
        mov     cx,word ptr cs:[10Bh]
u4_copy:
        rep     movsb
        mov     bx,si
        jmp     short u4_loop
u4_done:
        mov     ax,di
        sub     ax,word ptr cs:[109h]
        jmp     short u4_exit
u4_fill:
        mov     cl,al
        and     cx,3Fh
        add     cx,3
        cmp     al,0FEh
        jne     u4_absref
        lodsw
        mov     cx,ax
        lodsb
        mov     bx,si
        cmp     cx,word ptr cs:[10Bh]
        jbe     u4_fillit
        mov     cx,word ptr cs:[10Bh]
u4_fillit:
        rep     stosb
        jmp     u4_loop
u4_absref:
        cmp     al,0FFh
        jne     u4_havecount
        lodsw
        mov     cx,ax
u4_havecount:
        lodsw
        mov     bx,si
        add     ax,word ptr cs:[109h]
        mov     si,ax
        mov     ds,bp
        cmp     cx,word ptr cs:[10Bh]
        jbe     u4_abs
        mov     cx,word ptr cs:[10Bh]
u4_abs:
        rep     movsb
        jmp     u4_loop
u4_exit:
        pop     si
        pop     es
        pop     dx
        pop     ds
        pop     di
        pop     cx
        pop     bx
        pop     bp
        ret
_Unpack4 ENDP
        dw      0,0,0,0,0               ; 1D9A:0109 output start, +0Bh bytes left, +0Dh end

; 1D9A:0113: expand the segment src ([bp+6]) into the segment dst ([bp+8]). The stream starts with
; a 32-bit output length and a skipped word. Then each control byte subtracts its own count from the
; 32-bit remainder and renormalizes ES:DI (1D9A:0162): a positive byte copies that many bytes from
; the input; 0 reads a big-endian word count and fills that many copies of the next byte; a byte at
; or above 80h fills (100h - byte) copies of the next byte. Stops when the 32-bit length runs out.
        PUBLIC  _Unpack3                ; @ 1D9A:0113
_Unpack3 PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    cx
        push    di
        push    ds
        push    dx
        push    es
        push    si
        cld
        mov     es,[bp+8]
        mov     ds,[bp+6]
        xor     cx,cx
        xor     di,di
        xor     si,si
        add     si,2
        mov     bx,[si]                         ; 32-bit output length
        mov     dx,[si+2]
        add     si,4
        lodsw
        add     si,ax                           ; skip the header word
u3_read:
        mov     cl,[si]
        inc     si
        or      cl,cl
        je      u3_zerocount
        js      u3_negcount
        call    _NormDest
        rep     movsb
        jmp     short u3_next
u3_zerocount:
        mov     cx,[si]
        xchg    cl,ch
        add     si,2
        jmp     short u3_emit
u3_negcount:
        neg     cl
u3_emit:
        call    _NormDest
        lodsb
        rep     stosb
u3_next:
        mov     ax,bx
        or      ax,dx
        jne     u3_read
        jmp     short u3_exit
_Unpack3 ENDP

; 1D9A:0162: subtract the count just emitted (CX) from the 32-bit output remainder (BX:DX) and
; renormalize the destination far pointer ES:DI (carry DI's paragraphs into ES, keep DI below 16).
        PUBLIC  _NormDest               ; @ 1D9A:0162
_NormDest PROC NEAR
        sub     bx,cx
        sbb     dx,0
        push    di
        shr     di,1
        shr     di,1
        shr     di,1
        shr     di,1
        mov     ax,es
        add     ax,di
        mov     es,ax
        pop     di
        and     di,0Fh
        ret
_NormDest ENDP

; 1D9A:017B: Unpack3's exit (the map folds it into 1D9A:0162's range, after the helper above).
u3_exit:
        pop     si
        pop     es
        pop     dx
        pop     ds
        pop     di
        pop     cx
        pop     bx
        pop     ax
        pop     bp
        retf
        dw      0,0,0                   ; 1D9A:0185 Unpack1's saved output offset and count

; 1D9A:018B: expand the dictionary format 1 into the work segment ([bp+0Ah]). The stream starts with
; a 16-bit output limit and a skipped word; codes are 12 bits (1D9A:0299). A code whose high byte is
; nonzero names an entry in the work segment's 4-byte offset table at BP: it holds a far pointer to
; a dictionary string and its length. Copy that string, then emit the byte it ends with (the next
; match byte); a zero high byte is a literal. Entries are added to the table as the output grows,
; up to 3C00h. The table pairs (offset at cs:[185h], count at cs:[189h]) and cs:[187h] hold the
; dictionary pointer while copying.
        PUBLIC  _Unpack1                ; @ 1D9A:018B
_Unpack1 PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    cx
        push    dx
        push    es
        push    ds
        push    si
        push    di
        cld
        mov     es,[bp+8]
        mov     ds,[bp+6]
        mov     bp,[bp+0Ah]
        xor     di,di
        mov     si,2
        lodsw
        mov     word ptr cs:[0Ch],ax            ; output limit
        lodsw
        lodsw
        add     si,ax
        mov     word ptr cs:[10h],ds            ; packed source segment
        mov     ds,bp
        xor     cx,cx
        mov     byte ptr cs:[12h],ch            ; code counter
        mov     cl,4
        xor     bp,bp                           ; output table offset
        call    _ReadCode1
        mov     dx,di
        mov     word ptr cs:[185h],1
        mov     word ptr cs:[189h],1
        mov     ch,al
        stosb
        jmp     short u1_test
u1_break:
        jmp     u1_exit
u1_test:
        cmp     di,word ptr cs:[0Ch]
        jae     u1_break
        call    _ReadCode1
        or      ah,ah
        je      u1_literal
        dec     ah
        mov     bx,ax
        shl     bx,1
        shl     bx,1
        cmp     bx,bp
        jae     u1_copybytes
        push    si
        mov     si,[bx]                         ; dictionary string offset
        mov     cx,[bx+2]                       ; its length
        mov     word ptr cs:[189h],cx
        push    ds
        push    es
        pop     ds
        mov     word ptr cs:[187h],di
        rep     movsb
        pop     ds
        mov     si,word ptr cs:[187h]
        mov     ch,byte ptr es:[si]
        pop     si
u1_store:
        cmp     bp,3C00h
        jae     u1_skip
        mov     word ptr ds:[bp],dx
        inc     word ptr cs:[185h]
        mov     bx,word ptr cs:[185h]
        mov     word ptr ds:[bp+2],bx
        add     bp,4
u1_skip:
        mov     dx,word ptr cs:[187h]
        mov     bx,word ptr cs:[189h]
        mov     word ptr cs:[185h],bx
        jmp     short u1_test
u1_copybytes:
        push    si
        mov     si,dx
        push    ds
        push    es
        pop     ds
        mov     al,ch
        mov     dx,di
        mov     word ptr cs:[187h],dx
        mov     cx,word ptr cs:[189h]
        mov     word ptr cs:[185h],cx
        rep     movsb
        pop     ds
        pop     si
        stosb
        mov     ch,al
        inc     word ptr cs:[189h]
        jmp     short u1_store
u1_literal:
        stosb
        mov     ch,al
        cmp     bp,3C00h
        jae     u1_reset
        mov     word ptr ds:[bp],dx
        inc     word ptr cs:[185h]
        mov     bx,word ptr cs:[185h]
        mov     word ptr ds:[bp+2],bx
        add     bp,4
u1_reset:
        mov     dx,di
        dec     dx
        mov     word ptr cs:[185h],1
        mov     word ptr cs:[189h],1
        jmp     u1_test
_Unpack1 ENDP

; 1D9A:0299: read the next 12-bit code from the packed stream (whose segment Unpack1 saved in
; cs:[10h]). It counts codes in cs:[12h]: every other call takes a big-endian word and keeps its
; low 12 bits; the rest take the same word shifted right four (consuming one byte), so the two
; reads together take 12 bits from every 3 bytes.
        PUBLIC  _ReadCode1              ; @ 1D9A:0299
_ReadCode1 PROC NEAR
        mov     cl,4
        push    ds
        mov     ds,word ptr cs:[10h]
        inc     byte ptr cs:[12h]
        test    byte ptr cs:[12h],1
        jne     u1r_odd
        lodsw
        xchg    ah,al
        and     ah,0Fh
        pop     ds
        ret
u1r_odd:
        mov     ax,[si]
        xchg    ah,al
        shr     ax,cl
        inc     si
        pop     ds
        ret
_ReadCode1 ENDP

; 1D9A:02BF: Unpack1's exit (the map folds it into 1D9A:0299's range, after the helper above).
u1_exit:
        pop     di
        pop     si
        pop     ds
        pop     es
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        pop     bp
        retf
        dw      0                       ; 1D9A:02C9 Unpack2's next reader state

; 1D9A:02CB: expand the dictionary format 2 into the work segment ([bp+0Ah]), as Unpack1 but with
; 14-bit codes (1D9A:03CC) and two tables in the work segment: the entry offsets at BP and the entry
; lengths at BP+7E00h. The output limit is 7E00h; entries grow by 2 words per match. Each code with
; a nonzero high byte is a dictionary entry (index * 2 for the offset, * 2 for the length); a zero
; high byte is a literal. cs:[2C9h] steps the code reader, cs:[185h]/[189h]/[187h] hold the table
; pair and the dictionary pointer while copying.
        PUBLIC  _Unpack2                ; @ 1D9A:02CB
_Unpack2 PROC FAR
        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    cx
        push    dx
        push    es
        push    ds
        push    si
        push    di
        cld
        mov     es,[bp+8]
        mov     ds,[bp+6]
        mov     bp,[bp+0Ah]
        xor     di,di
        mov     si,2
        lodsw
        mov     word ptr cs:[0Ch],ax            ; output limit
        lodsw
        lodsw
        add     si,ax
        mov     word ptr cs:[10h],ds            ; packed source segment
        mov     ds,bp
        xor     cx,cx
        mov     ax,400h
        mov     word ptr cs:[2C9h],ax           ; first reader state
        mov     bp,cx
        call    _ReadCode2
        mov     dx,di
        mov     word ptr cs:[185h],1
        mov     word ptr cs:[189h],1
        mov     ch,al
        stosb
        jmp     short u2_test
u2_break:
        jmp     u2_exit
u2_test:
        cmp     di,word ptr cs:[0Ch]
        jae     u2_break
        call    _ReadCode2
        or      ah,ah
        je      u2_literal
        dec     ah
        mov     bx,ax
        shl     bx,1
        cmp     bx,bp
        jae     u2_copybytes
        push    si
        mov     si,[bx]                         ; dictionary string offset
        mov     cx,[bx+7E00h]                   ; its length
        mov     word ptr cs:[189h],cx
        push    ds
        push    es
        pop     ds
        mov     bx,di
        rep     movsb
        mov     ch,[bx]
        pop     ds
        pop     si
u2_store:
        cmp     bp,7E00h
        jae     u2_skip
        mov     word ptr ds:[bp],dx
        inc     word ptr cs:[185h]
        mov     ax,word ptr cs:[185h]
        mov     word ptr ds:[bp+7E00h],ax
        add     bp,2
u2_skip:
        mov     dx,bx
        mov     bx,word ptr cs:[189h]
        mov     word ptr cs:[185h],bx
        jmp     short u2_test
u2_copybytes:
        push    si
        push    ds
        mov     si,es
        mov     ds,si
        mov     si,dx
        mov     al,ch
        mov     dx,di
        mov     bx,dx
        mov     cx,word ptr cs:[189h]
        mov     word ptr cs:[185h],cx
        rep     movsb
        pop     ds
        pop     si
        stosb
        mov     ch,al
        inc     word ptr cs:[189h]
        jmp     short u2_store
u2_literal:
        stosb
        mov     ch,al
        cmp     bp,7E00h
        jae     u2_reset
        mov     word ptr ds:[bp],dx
        inc     word ptr cs:[185h]
        mov     bx,word ptr cs:[185h]
        mov     word ptr ds:[bp+7E00h],bx
        add     bp,2
u2_reset:
        mov     dx,di
        dec     dx
        mov     word ptr cs:[185h],1
        mov     word ptr cs:[189h],1
        jmp     u2_test
_Unpack2 ENDP

; 1D9A:03CC: read the next 14-bit code of format 2 from the packed stream (segment in cs:[10h]).
; cs:[2C9h] holds the address of the next of four reader states; each state reads its bits, stores
; the following state and returns. State 3F1h reads 14 bits from a word, 400h reads 14 bits from a
; byte pair, 3D8h reads 14 bits across two bytes and 412h reads the low 14 bits of a byte pair.
        PUBLIC  _ReadCode2              ; @ 1D9A:03CC
_ReadCode2 PROC NEAR
        push    ds
        mov     ds,word ptr cs:[10h]
        mov     bx,2C9h
        jmp     word ptr cs:[bx]
u2r_word:
        mov     ax,3F1h
        mov     word ptr cs:[2C9h],ax
        lodsw
        xchg    ah,al
        mov     cl,[si]
        shl     cl,1
        rcl     ax,1
        shl     cl,1
        rcl     ax,1
        and     ah,3Fh
        pop     ds
        ret
u2r_pair:
        mov     ax,400h
        mov     word ptr cs:[2C9h],ax
        lodsw
        xchg    ah,al
        and     ah,3Fh
        pop     ds
        ret
u2r_low:
        mov     ax,412h
        mov     word ptr cs:[2C9h],ax
        lodsb
        mov     ah,[si]
        xchg    ah,al
        shr     ax,1
        shr     ax,1
        pop     ds
        ret
u2r_split:
        mov     ax,3D8h
        mov     word ptr cs:[2C9h],ax
        lodsw
        mov     cl,al
        mov     al,[si]
        shr     cl,1
        rcr     ax,1
        shr     cl,1
        rcr     ax,1
        shr     ax,1
        shr     ax,1
        pop     ds
        ret
_ReadCode2 ENDP

; 1D9A:042C: Unpack2's exit (the map folds it into 1D9A:03CC's range, after the helper above).
u2_exit:
        pop     di
        pop     si
        pop     ds
        pop     es
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        pop     bp
        retf

        END
