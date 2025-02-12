
# RC6502 FLASH Device Data Exchange Protocol

## Overview
FLASH Device (W26Q64F) is driven by Atmega8515 MCU. This protocol ensures robust communication between the CPU and MCU, with clear mechanisms for synchronisation, acknowledgment, and error handling.

The CPU is responsible for initiating all data transfers. Communication occurs through a dedicated address range, currently set to $C800–$CFFF. There are two modes of operation: DATA and CTRL/STATUS, distinguished by the 5th bit of the byte it determines the byte type: if clear, it indicates a command (CPU) or status (MCU); if set, it represents a data nibble.

In CTRL/STATUS mode, the 5th bit is set to 0. Bits [3..0] carry commands (from the CPU) or status (from the MCU). The MCU provides two status flags: BSY (bit 6) indicates the MCU is busy, and RDY (bit 7) indicates data is ready. The CPU waits for BSY to clear before writing a byte and for RDY to be set before reading from register.

In DATA mode, the 5th bit is set to 1. Data is transmitted in nibbles using bits [3..0]. The most significant nibble (MSN) is sent first, followed by the least significant nibble (LSN).

When the CPU sends a command, the MCU replies with an ACK or NACK message. Note this is for only command, does not apply to data transfers. Similarly, when the MCU sends data to the CPU, the CPU must reply with an ACK for each byte received. If the CPU responds with NACK, the data transfer is aborted.

All data transmissions starts with begin-of-data-transfer (BODT) and concluded with an end-of-data-transfer (EODT) marker. BODT could be used to restart transmission, in some context this could mean separate data chunk, like file name and data in CMD_WRITE. If a new command is sent while a data transfer is in progress, it interrupts and cancels the current transmission.

```
## Status flags
RDY_FLAG  = (1 << 7)
BSY_FLAG  = (1 << 6)
ACK_FLAG  = (1 << 5)
DAT_FLAG  = (1 << 4)
```
```
## Commands
* CMD_LIST = 0x01     - list files. 
* CMD_WRITE = 0x02    - save data to disk. 
* CMD_READ = 0x03     - read data from disk. 
* CMD_DELETE = 0x04   - delete file.
* BODT = 0x80         - indicates the beginning of data transfer.
* EODT = 0x8F         - indicates the end of data transfer.
* ACK  = 0x90         - MCU confirms operation
* NACK = 0x9F         - MCU declines operation
```
Note in CMD_READ and CMD_DELETE commands it is possible to use block id instead of file name, eg. "#123"

## Design Issues
* Writing to MCU's input register. 6502 runs at 1 MHz clock speed, it writes to $c800 address in sync with /WR+PHI2, so we have less than 500ns window to read the data. When CPU writes data, data is latched an interrupt is triggered in MCU but IRQ latency and additional cycles delays reading for 1125ns, But after 500ns window LEWRITE- goes up and we must keep that signal low until data is read in ISR. One possible solution is to add RS latch and reset it from ISR after register is read.

## Low level data exchange protocol 
```
### CPU -> MCU
Wait for BSY is cleared
Write a command byte
Wait for RDY is set
Read a byte, ACK or NACK is expected
```
### MCU -> CPU
```
In ISR:
Set BSY, clear RDY
Read byte, check value- command, data nibble, ACK or NACK is expected
Update state in SM
If NACK is received - stop current operation.
Generate strobe CLEWRITE- to release latch
In main loop:
If a new command is initiate - start executing and return ACK or NACK
Otherwise continue with data transfer or put to idle.
```
## High level data exchange protocol

### CMD_LIST
```
; CPU rquests list all files starting with 't'
; MCU response with single file entry
; 0 Type - Unused=0, File=1, Executable=2
; 1 Block number - starting from 0
; 2-3 Length - number of bytes (little endian)
; 4-15 Name - case insensitive
; Type=File, Block=1, Length=10, Name=test.txt
CPU, MCU - CMD_LIST, ACK
CPU, MCU - BODT, ACK       	    ; File name search pattern
CPU, MCU - 0x97, ACK, 0x94, ACK ; File starts with 't'
CPU, MCU - EODT, ACK    	    ; Done
MCU, CPU - BODT, ACK            ; Start
MCU, CPU - 0x90, ACK, 0x91, ACK	; Type
MCU, CPU - 0x90, ACK, 0x91, ACK	; Block
MCU, CPU - 0x90, ACK, 0x9A, ACK	; Length LSB
MCU, CPU - 0x90, ACK, 0x90, ACK	; Length MSB
MCU, CPU - 0x97, ACK, 0x94, ACK	; 't'
MCU, CPU - 0x96, ACK, 0x95, ACK	; 'e'
MCU, CPU - 0x97, ACK, 0x93, ACK	; 's'
MCU, CPU - 0x97, ACK, 0x94, ACK	; 't'
MCU, CPU - 0x92, ACK, 0x9E, ACK	; '.'
MCU, CPU - 0x97, ACK, 0x94, ACK	; 't'
MCU, CPU - 0x97, ACK, 0x98, ACK	; 'x'
MCU, CPU - 0x97, ACK, 0x94, ACK	; 't'
MCU, CPU - EODT, ACK            ; Done
```

### CMD_WRITE
```
; write "test" string to "test.txt" file
CPU, MCU - CMD_WRITE, ACK
CPU - BODT, ACK      	; File name
CPU - 0x97, 0x94 	    ; 't'
CPU - 0x96, 0x95		; 'e'
CPU - 0x97, 0x93		; 's'
CPU - 0x97, 0x94		; 't'
CPU - 0x92, 0x9E		; '.'
CPU - 0x97, 0x94		; 't'
CPU - 0x97, 0x98		; 'x'
CPU - 0x97, 0x94		; 't'
CPU - EODT, ACK   	    ; MCU allocates file entry, could return NACK if name is invalid or no room left
CPU - BODT      		; File content
CPU - 0x97, 0x94		; 't'
CPU - 0x96, 0x95		; 'e'
CPU - 0x97, 0x93		; 's'
CPU - 0x97, 0x94		; 't'
CPU - EODT, ACK   	    ; Done
```

### CMD_READ
```
; read "empty" file
CPU, MCU - CMD_WRITE, ACK
CPU - BODT, ACK      	; File name
CPU - 0x96, 0x95		; 'e'
CPU - 0x96, 0x9D		; 'm'
CPU - 0x97, 0x90		; 'p'
CPU - 0x97, 0x94		; 't'
CPU - 0x97, 0x99		; 'y'
CPU - EODT, ACK   	; File found
MCU, CPU - BODT, ACK  ; File content
MCU, CPU - EODT, ACK  ; Done
```

### CMD_DELETE
```
; request delete non-existing file
CPU, MCU - CMD_WRITE, ACK
CPU - BODT, ACK      	; File name
CPU - 0x96, 0x9E		; 'n'
CPU - 0x96, 0x9F		; 'o'
CPU - 0x97, 0x90		; 'p'
CPU - 0x96, 0x95		; 'e'
MCU - NACK  		; File not found
```
