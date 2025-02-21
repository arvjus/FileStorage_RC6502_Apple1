; Flash Disk Shell
; Copyright (c) 2025 Arvid Juskaitis

; Save memory, write to file


; save Integer-BASIC in ProDOS format
save:
    ; store lomem, himem variables into prg_start, prg_stop.
    lda lomem
    sta prg_start
    lda lomem+1
    sta prg_start+1
    lda himem
    sta prg_stop
    lda himem+1
    sta prg_stop+1
    
    ; calculate size, add 512, put into tmp_buffer
    sec                     ; clear borrow
    lda prg_stop
    sbc prg_start
    sta tmp_buffer
    lda prg_stop+1
    sbc prg_start+1         ; Subtract with borrow
    clc
    adc #$02
    sta tmp_buffer+1    
    
    ; append #start#size to cmd line
    ldx #2                  ; Start searchging for end-of-string from name position
save_eos_loop:
    lda buffer, x
    beq save_eos_found
    inx
    jmp save_eos_loop
save_eos_found:
    ; hash sep
    lda #'#'
    sta buffer, x
    inx
    ; start
    lda prg_start+1
    jsr write_byte_hex_to_buffer
    lda prg_start
    jsr write_byte_hex_to_buffer
    ; hash sep
    lda #'#'
    sta buffer, x
    inx
    ; size
    lda tmp_buffer+1
    jsr write_byte_hex_to_buffer
    lda tmp_buffer
    jsr write_byte_hex_to_buffer
    ; null-terminate
    lda #0
    sta buffer, x

    ; send cmd_write request
    lda #CMD_WRITE
    jsr send_request
    bcc store_header        ; ok, continue
    cmp #ST_DONE
    beq store_done
    cmp #ST_ERROR
    beq store_err
    
    ; write first 2 pages
store_header:
    ; store signature
    lda #'A'
    sta $00
    lda #'1'
    sta $01

    ; begin data transfer
    lda #BODT
    jsr send_byte
    bcs store_err           ; timeout
    jsr receive_byte        ; ACK is expected
    bcs store_err           ; timeout
    cmp #NACK
    beq store_done

    ; init ptr
    lda #0
    sta ptr
    sta ptr+1

store_header_loop:
    ; check if ptr reached prg_stop
    lda ptr+1
    cmp #$02
    bne store_header_store
    lda ptr
    cmp #$00
    beq store_header_done

store_header_store:
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
    bcs store_err           ; timeout

    ; increment ptr
    inc ptr
    bne store_header_loop
    inc ptr+1
    jmp store_header_loop

store_header_done:
    jsr write_print_messages
    jmp write_prg_loop_init
store_done:
    jmp write_done
store_err:
    jmp write_err

; write regular file
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

write_prg_loop_init:
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
    lda #CR
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
    txa                     ; save this position
    pha

    ; Parse second xxxx into prg_stop
    jsr parse_addr          ; Parse 4-digit hex value into ptr
    lda ptr
    sta prg_stop
    lda ptr+1
    sta prg_stop+1
    
    ; Calculate size = prg_stop - prg_start, store into tmp_buffer
    sec                     ; clear borrow
    lda prg_stop
    sbc prg_start
    sta tmp_buffer
    lda prg_stop+1
    sbc prg_start+1         ; Subtract with borrow
    sta tmp_buffer+1    
    
    ; Go back to saved position and replace stop value with size in cmd
    pla
    tax
    lda tmp_buffer+1
    jsr write_byte_hex_to_buffer
    lda tmp_buffer
    jsr write_byte_hex_to_buffer
  
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

; A to hex, store in buffer. x points to the first digit in buffer
write_byte_hex_to_buffer:
    pha              ; Save A
    lsr              ; Shift high nibble to low
    lsr
    lsr
    lsr
    jsr nibble_to_ascii
    sta buffer,x     ; Store in buffer
    inx

    pla              ; Restore original value
    and #$0F         ; Mask out low nibble
    jsr nibble_to_ascii
    sta buffer,x     ; Store in buffer
    inx
    rts

nibble_to_ascii:
    cmp #10          ; If >= 10, it's A-F
    bcc nibble_to_ascii_digit
    adc #6           ; Adjust for ASCII 'A'-'F'
nibble_to_ascii_digit:
    adc #$30         ; Convert to ASCII ('0'-'9' or 'A'-'F')
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
