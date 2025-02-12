; Flash Disk Shell
; Copyright (c) retain2025 Arvid Juskaitis

; Delete file

delete:
    SET_PTR delete_msg
    jsr print_msg
delete_confirm:
    jsr KBDIN
    cmp #KEY_ESC
    beq delete_done
    cmp #KEY_CR
    bne delete_confirm

    lda #CMD_DELETE
    jsr send_request
    bcc delete_done         ; ok, continue
    cmp #ST_DONE
    beq delete_done
    cmp #ST_ERROR
    beq delete_err

delete_done:
    lda #KEY_CR
    jsr ECHO
    rts
delete_err:
    lda #'!'
    jsr ECHO
    rts

delete_msg: .text "Press CR to delete or ESC to cancel", 0
