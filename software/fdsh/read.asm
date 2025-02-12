; Flash Disk Shell
; Copyright (c) retain2025 Arvid Juskaitis

; Read file, load into memory, eventualy execute loaded program

execute:
    lda #1
    sta execute_flag
    jmp read_entry
read:
    lda #0
    sta execute_flag
read_entry:
    lda #CMD_READ
    jsr send_request
    bcc read_request_ok         ; ok, continue
    cmp #ST_DONE
    beq read_done
    cmp #ST_ERROR
    beq read_err
read_request_ok:
    cmp #BODT
    bne read_done               ; nope, no data

; ------------------------------------------------------------------------
; receive 32 bytes- store in the buffer
read_fileentry:
    ldx #0
read_fileentry_byte:
    jsr receive_data_byte
    bcc read_fileentry_store    ; ok, continue
    cmp #ST_DONE
    beq read_done
    cmp #ST_ERROR
    beq read_err
read_fileentry_store:
    sta buffer, x
    inx
    cpx #32
    bne read_fileentry_byte
    jsr calc_prg_stop
    jsr read_print_messages_start_stop

; keep receiving data bytes, laod into memory

    ; initialize ptr with buff_fe_start
    lda buff_fe_start
    sta ptr
    lda buff_fe_start+1
    sta ptr+1

read_receive_data_byte:
    jsr receive_data_byte
    bcc read_prg_store          ; ok, continue
    cmp #ST_DONE
    beq read_prg_done
    cmp #ST_ERROR
    beq read_err
read_prg_store:
    ldy #$00
    sta (ptr),y
.if DEBUG == 1
    jsr PRBYTE
    lda #' '
    jsr ECHO
.endif     
    ; increment ptr
    inc ptr
    bne read_skip_high          ; if ptr low byte did not wrap, skip high byte increment
    inc ptr+1
read_skip_high:
    ; check if ptr reached prg_stop
    lda ptr+1
    cmp prg_stop+1              ; compare high byte first
    bcc read_receive_data_byte  ; if ptr+1 < prg_stop+1, continue
    bne read_prg_done           ; if ptr+1 > prg_stop+1, exit
    lda ptr
    cmp prg_stop
    bcc read_receive_data_byte  ; if ptr < prg_stop, continue

read_prg_done:
    SET_PTR read_msg3
    jsr print_msg

; jump into start address
    lda execute_flag
    beq read_done
    jmp (buff_fe_start)        

read_done:
    lda #KEY_CR
    jsr ECHO
    rts
read_err:
    lda #'!'
    jsr ECHO
    rts

; print messages start, stop values
read_print_messages_start_stop:
    SET_PTR read_msg1
    jsr print_msg

    SET_PTR buffer+6
    jsr print_msg

    SET_PTR read_msg2
    jsr print_msg

    lda buff_fe_start+1         ; start high
    jsr PRBYTE
    lda buff_fe_start           ; start low
    jsr PRBYTE
    lda #' '
    jsr ECHO
    lda #'-'
    jsr ECHO
    lda #' '
    jsr ECHO
    lda prg_stop+1              ; stop high
    jsr PRBYTE          
    lda prg_stop                ; stop low
    jsr PRBYTE
    lda #' '
    jsr ECHO
    rts

read_msg1:  .text "Reading ", 0
read_msg2:  .text " into memory ", 0
read_msg3:  .text " .. done.", 0
