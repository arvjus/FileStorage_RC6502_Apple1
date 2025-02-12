.PHONY: all flash clean

MCU = atmega8515
F_CPU = 8000000
CC = avr-gcc
OBJCOPY = avr-objcopy
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -Wno-unused-value

TARGET = rc6502_fd
SRC = rc6502_fd.c uart.c simplefs.c  w25q64fv.c spi.c

all: $(TARGET).hex

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $(TARGET).elf $(TARGET).hex

$(TARGET).elf: $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET).elf $(SRC)
	avr-size --mcu=$(MCU) --format=avr $(TARGET).elf

upload: $(TARGET).hex
	avrdude -c arduino -p m8515 -P /dev/ttyUSB1 -b 38400 -V -U flash:w:$(TARGET).hex

flash-usbtiny: $(TARGET).hex
	avrdude -c usbtiny -p m8515 -V -U flash:w:$(TARGET).hex:i

clean:
	rm -f $(TARGET).elf $(TARGET).hex $(TARGET).map
