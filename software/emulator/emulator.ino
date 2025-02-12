/*
RC6502 FLASH (W25Q64F) Card emulator
Copyright (c) 2025 Arvid Juskaitis

PORTD (Digital Pins 2–7):

    PD2: Digital Pin 2      29
    PD3: Digital Pin 3      30
    PD4: Digital Pin 4      31
    PD5: Digital Pin 5      32
    PD6: Digital Pin 6      33
    PD7: Digital Pin 7      34

PORTB (Digital Pins 8–13):

    PB0: Digital Pin 8      27
    PB1: Digital Pin 9      28
    PB2: Digital Pin 10     24
    PB3: Digital Pin 11     19
 */

#define BODT        0x80
#define EODT        0x8F
#define ACK         0xA0
#define NACK        0xAF

#define RDY_FLAG    (1 << 7)  // MCU sets if data is ready
#define BSY_FLAG    (1 << 6)  // MCU sets if busy
#define DAT_FLAG    (1 << 4)  // Data nibble flag

const int dataPins[] = {2, 3, 4, 5, 6, 7, 8, 9}; // PD2–PD7 for MS, PB0–PB1 for LS
const int rwPin = 10; // Read/Write control pin
const int phi2Pin = 11; // PHI2 clock signal pin

const char* usage = "Usage: wHH, wC, s, r, R, a, n, b, e, l, lr"; 
uint16_t log_buff[512];
uint16_t log_idx = 0;

void setup() {
    Serial.begin(38400);
    Serial.println(usage);

    pinModeData(INPUT);
    pinMode(rwPin, OUTPUT);
    pinMode(phi2Pin, OUTPUT);
    digitalWrite(rwPin, HIGH); // Default to read
    digitalWrite(phi2Pin, LOW); // Default clock low
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        if (command.startsWith("w")) {
            short len = command.length();
            if (len == 2) {
                byte data = command.charAt(1);
                wait_no_bsy();
                writeData(DAT_FLAG | ((data >> 4) & 0x0f));
                byte rx = wait_rdy_read();
                if (rx != ACK) {
                    Serial.print("RX: ");
                    print_hex(rx);
                    Serial.println();
                    return;
                }
                writeData(DAT_FLAG | (data & 0x0f));
            } else if (len == 3) {
                String hexValue = command.substring(1);
                byte data = strtol(hexValue.c_str(), NULL, 16);
                wait_no_bsy();
                writeData(data);
            }
        } else if (command == "a") {
            byte data = ACK; // 0xa0
            wait_no_bsy();
            writeData(data);
        } else if (command == "n") {
            byte data = NACK; // 0xaf
            wait_no_bsy();
            writeData(data);
        } else if (command == "b") {
            byte data = BODT; // 0x80
            wait_no_bsy();
            writeData(data);
        } else if (command == "e") {
            byte data = EODT; // 0x8f
            wait_no_bsy();
            writeData(data);
        } else if (command == "s") {
            byte data = readData();
            print_hex(data);
            Serial.print(", ");
            print_bin(data);
            Serial.println();
        } else if (command == "r") {
            byte data = wait_rdy_read();
            if (data != ACK) {
                wait_no_bsy();
                writeData(ACK);  // 0xa0
            }
            if (data & DAT_FLAG) {
                byte nibble = wait_rdy_read();
                wait_no_bsy();
                writeData(ACK);  // 0xa0
                data = ((data & 0x0f) << 4) | (nibble & 0x0f);
                if (data >= 0x20 && data <= 0x7f) {
                    Serial.print((char) data);
                } else {
                    print_hex(data);
                }
            } else {
                print_hex(data);
            }
            Serial.print(" ");
        } else if (command == "R") {
              while (true) {
                byte data = wait_rdy_read();
                wait_no_bsy();
                writeData(ACK);  // 0xa0
                if (data == EODT) break;
                if (data & DAT_FLAG) {
                    byte nibble = wait_rdy_read();
                    wait_no_bsy();
                    writeData(ACK);  // 0xa0
                    data = ((data & 0x0f) << 4) | (nibble & 0x0f);
                    if (data >= 0x20 && data <= 0x7f) {
                        Serial.print((char) data);
                    } else {
                        print_hex(data);
                    }
                } else {
                    print_hex(data);
                }
                Serial.print(" ");
            }
        } else if (command == "l") {
            log_print();
        } else if (command == "lr") {
            log_idx = 0;
        } else {
            Serial.print("Invalid command: ");
            Serial.print(command);
            Serial.print(", ");
            Serial.println(usage);
        }
    }
}

void print_hex(byte value) {
  if (value < 0x10) Serial.print('0');  
  Serial.print(value, HEX);
}

void print_bin(byte value) {
for (int i = 7; i >= 0; i--) {
    Serial.print((value >> i) & 1); 
  }  
}

void wait_no_bsy() {
    byte status;
    do
        status = readData();
    while ((status & BSY_FLAG) != 0);
    delay(10);
}

byte wait_rdy_read() {
    byte data;
    do
        data = readData();
    while ((data & RDY_FLAG) == 0);
    
    log(0, data);
    return data;
}

void pinModeData(short mode) {
    // Set all data pins (PD2–PD7 and PB0–PB1)
    if (mode == INPUT) {
        DDRD &= ~0b11111100; // Configure PD2–PD7 as input
        DDRB &= ~0b00000011; // Configure PB0–PB1 as input
    } else {
        DDRD |= 0b11111100; // Configure PD2–PD7 as output
        DDRB |= 0b00000011; // Configure PB0–PB1 as output
    }
}

void writeData(byte data) {
    log(1, data);

    pinModeData(OUTPUT);
    digitalWrite(rwPin, LOW); // Set to write

    // Write MS part (6 bits) to PD2–PD7
    PORTD = (PORTD & 0x03) | (data & 0xFC); // Clear PD2–PD7, set MS bits
    // Write LS part (2 bits) to PB0–PB1
    PORTB = (PORTB & 0xFC) | (data & 0x03); // Clear PB0–PB1, set LS bits

    // Simulate clock pulse
    PORTB |= (1 << PB3);  // Set PHI2 HIGH
    __asm__ __volatile__("nop\n nop\n nop\n nop\n"); // ~0.5 µs delay
    PORTB &= ~(1 << PB3); // Set PHI2 LOW
}

byte readData() {
    pinModeData(INPUT);
    digitalWrite(rwPin, HIGH); // Set to read

    // Simulate clock pulse
    PORTB |= (1 << PB3);  // Set PHI2 HIGH
    __asm__ __volatile__("nop\n nop\n"); // ~0.25 µs delay

    // Read MS part (6 bits) from PD2–PD7
    byte msPart = PIND & 0xFC; // Mask PD2–PD7
    // Read LS part (2 bits) from PB0–PB1
    byte lsPart = PINB & 0x03; // Mask PB0–PB1

    PORTB &= ~(1 << PB3); // Set PHI2 LOW

    delay(10);

    // Combine and return
    return msPart | lsPart;
}

void log(uint16_t rw, uint8_t data) {
    if (log_idx + 1 >= sizeof log_buff) {
        Serial.println("log buffer is full");
        return;
    }
    log_buff[log_idx ++] = (rw << 8) | data;
}

void log_print() {
    Serial.println();
    for (int i = 0; i < log_idx; i++) {
        uint16_t data = log_buff[i];
        Serial.print((data & 0xff00) ? "TX: " : "RX: ");
        print_hex(data & 0xff);
        Serial.print(", ");
        print_bin(data & 0xff);
        Serial.println("");
    }
}
