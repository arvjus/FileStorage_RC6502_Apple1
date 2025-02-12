/*
RC6502 FLASH (W25Q64F) Card firmware
Copyright (c) 2025 Arvid Juskaitis
*/

#include "spi.h"

// Function definitions
void SPI_init(uint8_t mode, uint8_t clock_div) {
    // Set MOSI, SCK as Output
    DDRB |= (1 << PB5) | (1 << PB7); // MOSI, SCK
    DDRB &= ~(1 << PB6);             // MISO as Input

    // Enable SPI, Master mode
    SPCR = (1 << SPE) | (1 << MSTR);

    // Set SPI Clock Rate
    SPCR = (SPCR & ~0x03) | (clock_div & 0x03);
    SPSR = (SPSR & ~0x01) | ((clock_div >> 2) & 0x01);

    // Set SPI Mode
    SPCR = (SPCR & ~((1 << CPOL) | (1 << CPHA))) | (mode << CPHA);
}

uint8_t SPI_transfer(uint8_t data) {
    SPDR = data;                      // Load data into the buffer
    while (!(SPSR & (1 << SPIF)));    // Wait until transmission is complete
    return SPDR;                      // Return received data
}

void SPI_setCS(uint8_t cs_pin, uint8_t state) {
    if (state) {
        PORTB |= (1 << cs_pin); // Set CS high
    } else {
        PORTB &= ~(1 << cs_pin); // Set CS low
    }
}

// Define the SPI instance
SPI_t SPI = {
    .init = SPI_init,
    .transfer = SPI_transfer,
    .setCS = SPI_setCS,
};


/*  example usage:

    // Initialize SPI in MODE0, clock divider 4
    SPI.init(SPI_MODE0, SPI_CLOCK_DIV4);

    // Set CS pin as output
    DDRB |= (1 << PB4); // Assuming PB4 is used for CS

    // Communicate with SPI slave
    SPI.setCS(PB4, 0);         // Select slave
    uint8_t received = SPI.transfer(0x55); // Send 0x55 and receive response
    SPI.setCS(PB4, 1);         // Deselect slave

*/