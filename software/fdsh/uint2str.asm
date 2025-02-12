; Flash Disk Shell
; Copyright (c) retain2025 Arvid Juskaitis

; conversion from uint16 to string
; input - uint2str_number, uint2str_number+1
; output - uint2str_buffer (5 bytes instead of 6)
uint2str:
    cld                     ; Ensure decimal mode is off

    ; Initialize buffer with spaces for right alignment
    ldx #4                  ; Now the last index is 4 (instead of 5)
    lda #' '                ; Space character
fill_buffer:
    sta uint2str_buffer, x
    dex
    bpl fill_buffer

    ldx #4                  ; Start storing digits from the rightmost position

uint2str_loop:
    lda uint2str_number
    ora uint2str_number+1
    beq adjust_output       ; If number is zero, stop converting

    lda #0
    sta uint2str_remainder    ; Clear remainder

    ldy #16                 ; 16-bit division by 10
div_loop:
    asl uint2str_number       ; Shift left (multiplying by 2)
    rol uint2str_number+1
    rol uint2str_remainder    ; Shift remainder

    lda uint2str_remainder
    sec
    sbc #10                 ; Try subtracting 10
    bcc no_subtract         ; If borrow set, restore remainder

    sta uint2str_remainder
    inc uint2str_number       ; Properly store quotient

no_subtract:
    dey
    bne div_loop            ; Loop until division is done

    lda uint2str_remainder    ; Extract digit (remainder)
    clc
    adc #'0'                ; Convert to ASCII
    sta uint2str_buffer, x    ; Store digit in buffer
    dex
    bpl uint2str_loop        ; Keep converting

adjust_output:
    ; Ensure at least one digit is printed
    inx                     ; Move pointer to first digit
    cpx #5                  ; Now check against 5 instead of 6
    bne done
    lda #'0'
    sta uint2str_buffer+4     ; Last character

done:
    rts
