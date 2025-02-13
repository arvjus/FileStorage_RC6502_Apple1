; Flash Disk Shell
; Copyright (c) 2025 Arvid Juskaitis

.if REAL_HW     ;--------------------------------------

MOCK_HW = 0

; WozMon locations
ECHO    = $FFEF
PRBYTE  = $FFDC
KBD     = $D010
KBDCR   = $D011

; wait for char available, return in A
KBDIN:
    lda KBDCR   ; is char available?
    bpl KBDIN   ; not as long as bit 7 is low
    lda KBD     
    and #$7f    ; clear 7-nth bit
    rts

; if char is available, return in A, otherwise return 0 in A
KBDIN_NOWAIT:
    lda KBDCR   ; is char available?
    bpl KBDIN_NOWAIT_NONE   ; not as long as bit 7 is low
    lda KBD     
    and #$7f                ; clear 7-nth bit
    rts
KBDIN_NOWAIT_NONE:
    lda #0
    rts

.else   ; MOCK_HW ------------------------------------

MOCK_HW = 1

; we're running on py65mon, need to simulate some routines
PRBYTE: PHA             ; Save A for LSD.
        LSR
        LSR
        LSR             ; MSD to LSD position.
        LSR
        JSR PRHEX       ; Output hex digit.
        PLA             ; Restore A.
PRHEX:  AND #$0F        ; Mask LSD for hex print.
        ORA #'0'        ; Add "0".
        CMP #$3A        ; Digit?
        BCC ECHO        ; Yes, output it.
        ADC #$06        ; Add offset for letter.
ECHO:   STA $F001       ; PUTC
        RTS

; wait for char available, return in A
KBDIN:
    lda $f004           ; GETC
    beq KBDIN
; to upper
    cmp #$61            ; 'a'
    bcc KBDIN_done      ; If less than 'a', return unchanged
    cmp #$7B            ; 'z' + 1
    bcs KBDIN_done      ; If greater than 'z', return unchanged
    sec
    sbc #$20            ; Convert to uppercase ('a'-'z' → 'A'-'Z')
KBDIN_done:
    rts

; if char is available, return in A, otherwise return 0 in A
KBDIN_NOWAIT:
    lda $f004       ; GETC
    rts

.endif  ; REAL_HW/MOCK_HW ----------------------------
