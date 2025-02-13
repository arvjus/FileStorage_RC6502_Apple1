; Flash Disk Shell
; Copyright (c) 2025 Arvid Juskaitis

; Save memory, write to file

write:
    jsr write_parse_cmd_args
    bcs write_err           ; invalid command line

    jsr write_print_messages

; send command
    lda #CMD_WRITE
    jsr send_request
    bcc write_data_start    ; ok, continue
    cmp #ST_DONE
    beq write_done
    cmp #ST_ERROR
    beq write_err

; start sending data
write_data_start:
    lda #BODT
    jsr send_byte
    bcs write_err           ; timeout
    jsr receive_byte        ; ACK is expected
    bcs write_err           ; timeout
    cmp #NACK
    beq write_done

    ; init ptr
    lda prg_start
    sta ptr
    lda prg_start+1
    sta ptr+1

write_prg_loop:
    ; check if ptr reached prg_stop
    lda ptr+1
    cmp prg_stop+1
    bne write_prg_store
    lda ptr
    cmp prg_stop
    beq write_prg_stop

write_prg_store:
    ldy #$00
    lda (ptr),y
.if DEBUG == 1
    pha
    jsr PRBYTE
    lda #' '
    jsr ECHO
    pla
.endif     
    jsr send_data_byte
    bcs write_err           ; timeout

    ; increment ptr
    inc ptr
    bne write_prg_loop
    inc ptr+1
    jmp write_prg_loop

write_prg_stop:
; stop sending data
    lda #EODT
    jsr send_byte
    bcs write_err           ; timeout
    jsr receive_byte        ; ACK is expected
    bcs write_err           ; timeout

    SET_PTR write_msg3
    jsr print_msg

write_done:
    lda #KEY_CR
    jsr ECHO
    rts
write_err:
    lda #'!'
    jsr ECHO
    rts


; Parse string in format 'wname#xxxx#xxxx' and extract xxxx values
write_parse_cmd_args:
    ldx #$00                ; index in string

    ; Find first '#'
find_first_hash:
    lda buffer, x
    inx
    beq write_parse_cmd_args_err ; no '#' within buffer
    cmp #$23                ; '#'
    bne find_first_hash
    cpy #$02
    beq write_parse_cmd_args_err ; name is empy

    ; Parse first xxxx into prg_start
    jsr parse_addr          ; Parse 4-digit hex value into ptr
    lda ptr
    sta prg_start
    lda ptr+1
    sta prg_start+1

    lda buffer, x         
    cmp #$23                ; second '#' is expected
    bne write_parse_cmd_args_err
    inx                     ; point to next value

    ; Parse second xxxx into prg_stop
    jsr parse_addr          ; Parse 4-digit hex value into ptr
    lda ptr
    sta prg_stop
    lda ptr+1
    sta prg_stop+1
    clc
    rts
write_parse_cmd_args_err:
    sec
    rts

; Parse 4-digit hex value into (ptr, ptr+1). x points to the first digit in buffer
parse_addr:
    lda #$00
    sta ptr
    sta ptr+1
    ldy #$00                ; Digit counter (4 per value)

parse_addr_loop:
    lda buffer, x         
    beq parse_addr_done     ; null terminator
    cmp #$23                ; next '#'
    beq parse_addr_done   
    jsr hex_to_bin          ; ASCII hex to binary nibble

    ; Shift destination left by 4 (equivalent to multiplying by 16)
    asl ptr
    rol ptr+1
    asl ptr
    rol ptr+1
    asl ptr
    rol ptr+1
    asl ptr
    rol ptr+1

    ora ptr                 ; Add nibble to lower byte
    sta ptr

    inx
    iny
    cpy #$04                ; Process exactly 4 digits
    bne parse_addr_loop

parse_addr_done:
    rts


; print messages start, stop values
write_print_messages:
    SET_PTR write_msg1
    jsr print_msg

    lda prg_start+1         ; start high
    jsr PRBYTE
    lda prg_start           ; start low
    jsr PRBYTE
    lda #' '
    jsr ECHO
    lda #'-'
    jsr ECHO
    lda #' '
    jsr ECHO
    lda prg_stop+1          ; stop high
    jsr PRBYTE          
    lda prg_stop            ; stop low
    jsr PRBYTE

    SET_PTR write_msg2
    jsr print_msg
    rts

write_msg1:  .text "Writing memory ", 0
write_msg2:  .text " to file", 0
write_msg3:  .text " .. done.", 0
