; Flash Disk Shell
; Copyright (c) 2025 Arvid Juskaitis

; List directory

list:
    lda #CMD_LIST
    jsr send_request
    bcc list_request_ok     ; ok, continue
    cmp #ST_DONE
    beq list_done
    cmp #ST_ERROR
    beq list_err
list_request_ok:
    cmp #BODT
    bne list_done           ; nope, no data

; ------------------------------------------------------------------------
; receive 32 bytes- store in the buffer
list_fileentry:
    ldx #0
list_fileentry_byte:
    jsr receive_data_byte
    bcc list_fileentry_store ; ok, continue
    cmp #ST_DONE
    beq list_done
    cmp #ST_ERROR
    beq list_err
list_fileentry_store:
    sta buffer, x
    inx
    cpx #32
    bne list_fileentry_byte
    ; print and check if user has not canceled
    jsr list_print_fileentry
    jsr KBDIN_NOWAIT        ; 0 in A if no key pressed
    beq list_fileentry      ; next file entry
list_done:
    rts
list_err:
    lda #'!'
    jsr ECHO
    rts

; ------------------------------------------------------------------------
; print buffer
list_print_fileentry:
; print 2 byte start address (offs=2) as hex
    lda buff_fe_start+1 ; start high
    jsr PRBYTE
    lda buff_fe_start   ; start low
    jsr PRBYTE
    lda #' '
    jsr ECHO
    lda #'-'
    jsr ECHO
    lda #' '
    jsr ECHO
; print 2 byte stop address (start address + size) as hex
    jsr calc_prg_stop
    lda prg_stop+1      ; stop high
    jsr PRBYTE          
    lda prg_stop        ; stop low
    jsr PRBYTE
    lda #' '
    jsr ECHO
; print 2 byte size (offs=4)  as decimal
    lda buff_fe_size    ; size low
    sta uint2str_number
    lda buff_fe_size+1  ; size high
    sta uint2str_number+1
    jsr uint2str
    ldx #0
    jsr list_print_uint2str_buffer
    lda #' '
    jsr ECHO
; print 1 byte block number (offs=0) as decimal
    lda buffer          ; block low
    sta uint2str_number
    lda buffer+1        ; block high
    sta uint2str_number+1
    jsr uint2str
    ldx #2              ; skip first two spaces
    jsr list_print_uint2str_buffer
    lda #' '
    jsr ECHO
; print name (offs=6)  as char string     
    SET_PTR buffer+6
    jsr print_msg
list_print_fileentry_done:
    lda #CR
    jsr ECHO
    rts

; ------------------------------------------------------------------------
; load offset into x, e.g. x=0 from the beginning
list_print_uint2str_buffer:
    lda uint2str_buffer, x
    jsr ECHO
    inx
    cpx #5
    bne list_print_uint2str_buffer
    rts
