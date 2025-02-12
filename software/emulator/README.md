
# CPU Emulator 

Purpose of this device is to speed up development/debugging of communication protocol and underlying file system routines. 
Based on Arduino Uno, accepts commands from serial input and communicates with RC6502_flash card, using defined data communication protocol.
Since there is only one address used ($c800), address lines can be hardwired, arduino generates /WR, PHI2 and data bus signals. 

Baud rate - 38400

![emulator](https://github.com/arvjus/FDStorage_RC6502_Apple1/blob/main/gallery/emulator.jpg)

##  Supported commands

### s
reads and displays byte on the data bus

### r
waits for RDY flag set, displays byte on the data bus in hex. Or reads another nibble if bytes has DAT bit set. 
For every byte received, it sends ACK automatically if received byte is other than ACK (no ACK on ACK).  

### R
similar to r, but loops until EODT is received. 

### wHH|wC
waits for BSY flag cleared, sends a byte HH in hex or two data nibbles of C.  

### a
sends ACK (0x90) byte

### n
sends NACK (0x9f) byte

### b
sends BODT (0x80) byte

### e
sends EODT (0x8f) byte

### l
print log data

### lr
reset log data

