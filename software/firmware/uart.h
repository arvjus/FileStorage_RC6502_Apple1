/*
RC6502 FLASH (W25Q64F) Card firmware
Copyright (c) 2025 Arvid Juskaitis
*/

#pragma once

// Initialize UART
void uart_init(unsigned int ubrr);
// Transmit a character
void uart_transmit(unsigned char data);
// Transmit a string
void uart_transmit_string(const char *str);
// Check if a character is available
char uart_available(void);
// Receive a character
unsigned char uart_receive(void);
// Wait, receive a character
unsigned char uart_receive_blocking(void);
