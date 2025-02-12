/*
RC6502 FLASH (W25Q64F) Card firmware
Copyright (c) 2025 Arvid Juskaitis
*/

#pragma once

#include <avr/io.h>

// SPI Mode definitions
#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3

// SPI Clock Divider definitions
#define SPI_CLOCK_DIV4   0x00
#define SPI_CLOCK_DIV16  0x01
#define SPI_CLOCK_DIV64  0x02
#define SPI_CLOCK_DIV128 0x03

// SPI API
typedef struct {
    void (*init)(uint8_t mode, uint8_t clock_div);
    uint8_t (*transfer)(uint8_t data);
    void (*setCS)(uint8_t cs_pin, uint8_t state);
} SPI_t;

// Externally accessible SPI instance
extern SPI_t SPI;
