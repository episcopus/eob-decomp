; Unit at EOB.EXE 1BF3:000D (196 bytes, 4 functions): the mouse pointer. It keeps the cursor
; position at ds:8138/ds:813A, clamped to the limits ds:814C/ds:814E (x) and ds:8150/ds:8152 (y),
; tells the INT 33h driver to move with function 4 (doubling x when ds:8144 == 1, the 640-wide
; mode) only when a driver is present (ds:8148), and when the position changed redraws the game's
; own pointer through the far function pointers at ds:80AA and ds:80A2 (with the two arguments at
; ds:8172/ds:8174), bracketed by the two functions of unit 1BBE. 0087 sets the position, 00A8 moves
; it by a delta; both hold the busy flag at ds:812E across the work. The unit's first byte at
; 000D is a lone 0x00 (data, not code: the map marks a code segment's first byte as a function, as
; in E1990); it is emitted as a public byte symbol (_f1BF3_000D) so the registry records it at L2
; and the unit counts complete, as E1BA8 does. The three functions follow. Placeholder names.

        .MODEL  LARGE

        EXTRN   _HidePointer:FAR          ; @ 1BBE:0002
        EXTRN   _ShowPointer:FAR          ; @ 1BBE:00ED

        .CODE

        PUBLIC  _f1BF3_000D
_f1BF3_000D LABEL BYTE                    ; @ 1BF3:000D
        db      0

; 1BF3:000E: place the cursor at (CX, DX), clamped, and redraw it if it moved. Near: called
; only by 0087 and 00A8.
        PUBLIC  _f1BF3_000E              ; @ 1BF3:000E
_f1BF3_000E PROC NEAR
        cmp     cx, ds:[814Ch]
        jl      clampedx
        cmp     cx, ds:[814Eh]
        jle     storex
        mov     cx, ds:[814Eh]
        jmp     short storex
clampedx:
        mov     cx, ds:[814Ch]
storex: mov     ds:[8138h], cx
        cmp     dx, ds:[8150h]
        jl      clampedy
        cmp     dx, ds:[8152h]
        jle     storey
        mov     dx, ds:[8152h]
        jmp     short storey
clampedy:
        mov     dx, ds:[8150h]
storey: mov     ds:[813Ah], dx
        cmp     word ptr ds:[8148h], 0
        je      checkmove
        cmp     word ptr ds:[8144h], 1
        jne     setpos
        shl     cx, 1
setpos:
        mov     ax, 4
        int     33h
checkmove:
        mov     ax, ds:[8138h]
        cmp     ds:[815Ch], ax
        jne     redraw
        mov     ax, ds:[813Ah]
        cmp     ds:[815Eh], ax
        je      done
redraw:
        push    word ptr ds:[8174h]
        push    word ptr ds:[8172h]
        call    dword ptr ds:[80AAh]
        add     sp, 4
        call    _HidePointer
        call    _ShowPointer
        call    dword ptr ds:[80A2h]
done:   ret
_f1BF3_000E ENDP

; 1BF3:0087: set the cursor to (x, y), guarding the work with the busy flag at ds:812E.
        PUBLIC  _f1BF3_0087              ; @ 1BF3:0087
_f1BF3_0087 PROC FAR
        push    bp
        mov     bp, sp
        push    cx
        push    dx
        mov     cx, [bp + 6]
        mov     dx, [bp + 8]
busyset:
        cmp     word ptr ds:[812Eh], 0
        jne     busyset
        inc     word ptr ds:[812Eh]
        call    _f1BF3_000E
        dec     word ptr ds:[812Eh]
        pop     dx
        pop     cx
        pop     bp
        ret
_f1BF3_0087 ENDP

; 1BF3:00A8: move the cursor by (dx, dy), guarding the work with the busy flag at ds:812E.
        PUBLIC  _MovePointerBy              ; @ 1BF3:00A8
_MovePointerBy PROC FAR
        push    bp
        mov     bp, sp
        push    cx
        push    dx
        mov     cx, [bp + 6]
        mov     dx, [bp + 8]
busymove:
        cmp     word ptr ds:[812Eh], 0
        jne     busymove
        inc     word ptr ds:[812Eh]
        add     cx, ds:[8138h]
        add     dx, ds:[813Ah]
        call    _f1BF3_000E
        dec     word ptr ds:[812Eh]
        pop     dx
        pop     cx
        pop     bp
        ret
_MovePointerBy ENDP

        END
