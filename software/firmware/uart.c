/*
RC6502 FLASH (W25Q64F) Card firmware
Copyright (c) 2025 Arvid Juskaitis
*/

#include <avr/io.h>
#include "uart.h"

// Initialize UART
void uart_init(unsigned int ubrr) {
    // Set baud rate
    UBRRH = (unsigned char)(ubrr >> 8);
    UBRRL = (unsigned char)ubrr;
    // Enable transmitter and receiver
    UCSRB = (1 << RXEN) | (1 << TXEN);
    // Set frame format: 8 data bits, 1 stop bit
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
}

// Transmit a character
void uart_transmit(unsigned char data) {
    // Wait for empty transmit buffer
    while (!(UCSRA & (1 << UDRE)));
    // Put data into buffer, sends the data
    UDR = data;
}

// Transmit a string
void uart_transmit_string(const char *str) {
    while (*str) {
        uart_transmit(*str++);
    }
}

// Check if a character is available
char uart_available(void) {
    return (UCSRA & (1 << RXC));
}

// Receive a character
unsigned char uart_receive(void) {
    return UDR;
}

// Receive a character
unsigned char uart_receive_block(void) {
    // Wait for data to be received
    while (!uart_available());
    // Get and return received data from buffer
    return UDR;
}
