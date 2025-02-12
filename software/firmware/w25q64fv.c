/**
 * @file w25q64fv.c
 * @author Jeremy Dunne
 * @brief source file for the W25Q64 Flash Chip interface library
 * @version 0.1
 * @date 2022-05-27
 *
 * @author Arvid Juskaitis
 * @brief Code migrated to Atmega8515 MCU, from C++ to C
 * @version 0.9
 * @date 2025-01-20
 *
 * @copyright Copyright (c) 2022, 2025
 */

#include <avr/io.h>
#include "w25q64fv.h"
#include "defs.h"

// Private functions
W25Q64FV_status_t read_reg(uint8_t reg, uint8_t *buffer, unsigned int length);
W25Q64FV_status_t write_reg(uint8_t reg, uint8_t *buffer, unsigned int length);
W25Q64FV_status_t write_command(uint8_t command);
void select_device();
void release_device();

static int _cs;  ///< Chip select pin


W25Q64FV_status_t W25Q64FV_begin(uint8_t cs_pin) {
  // Initialize the CS pin
  _cs = cs_pin;
  DDRB |= (1 << _cs); // Set CS as output
  PORTB |= (1 << _cs); // Deselect device (CS high)

  // Initialize SPI
  SPI.init(SPI_MODE0, SPI_CLOCK_DIV4);

  // Check the device ID
  uint8_t buffer[5];
  read_reg(W25Q64FV_INSTRUCTION_MANUFACTURER_DEVICE_ID, buffer, 5);
  if (buffer[4] == 0) {
    return W25Q64FV_COMMUNICATION_FAIL;
  }

  // Reset the device
  return W25Q64FV_reset();
}

#if WRITE || DELETE || BULK_TRANSFER
W25Q64FV_status_t W25Q64FV_enable_writing() {
  // enable writing on the device
  // check if busy
  if (W25Q64FV_busy())
    return W25Q64FV_BUSY;
  // write the enable command
  W25Q64FV_status_t status = write_command(W25Q64FV_INSTRUCTION_WRITE_ENABLE);
  if (status != W25Q64FV_OK)
    return status;
  // wait until free
  return W25Q64FV_wait_until_free(W25Q64FV_DEFAULT_TIMEOUT);
}
#endif

#if UNUSED
W25Q64FV_status_t W25Q64FV_disable_writing() {
  // disable writing to the device
  // check if busy
  if (W25Q64FV_busy())
    return W25Q64FV_BUSY;
  // write the enable command
  W25Q64FV_status_t status = write_command(W25Q64FV_INSTRUCTION_WRITE_DISABLE);
  if (status != W25Q64FV_OK)
    return status;
  return W25Q64FV_OK;
}
#endif

#if WRITE || BULK_TRANSFER
W25Q64FV_status_t W25Q64FV_write_page(uint32_t start_address, byte *buffer) {
  // write a single page to the flash chip
  // check if busy
  if (W25Q64FV_busy())
    return W25Q64FV_BUSY;
  // check that writing is enables
  W25Q64FV_enable_writing();
  // write the page
  select_device();
  SPI.transfer(W25Q64FV_INSTRUCTION_PAGE_PROGRAM);
  SPI.transfer(start_address >> 16);
  SPI.transfer(start_address >> 8);
  SPI.transfer(start_address);
  for (int i = 0; i < 256; i++) {
    SPI.transfer(*buffer);
    *buffer++;
  }
  release_device();
  return W25Q64FV_OK;
}
#endif

#if LIST || READ || WRITE || BULK_TRANSFER
W25Q64FV_status_t W25Q64FV_read_page(uint32_t start_address, byte *buffer, uint16_t size) {
  // read a single page from the flash chip
  // check if busy
  if (W25Q64FV_busy())
    return W25Q64FV_BUSY;
  // read the page
  select_device();
  SPI.transfer(W25Q64FV_INSTRUCTION_READ_DATA);
  SPI.transfer(start_address >> 16);
  SPI.transfer(start_address >> 8);
  SPI.transfer(start_address);
  for (int i = 0; i < size; i++) {
    *buffer = SPI.transfer(0x00);
    *buffer++;
  }
  release_device();
  return W25Q64FV_OK;
}
#endif

#if BULK_TRANSFER
W25Q64FV_status_t W25Q64FV_erase_chip(bool hold) {
  // erase the entire chip
  // check if busy
  if (W25Q64FV_busy())
    return W25Q64FV_BUSY;
  // check that writing is enables
  W25Q64FV_enable_writing();
  // write the command to erase
  W25Q64FV_status_t status = write_command(W25Q64FV_INSTRUCTION_CHIP_ERASE);
  if (status != W25Q64FV_OK)
    return status;
  // check for the hold
  if (hold)
    return W25Q64FV_wait_until_free(W25Q64FV_CHIP_ERASE_TIMEOUT);
  return W25Q64FV_OK;
}
#endif

#if DELETE
W25Q64FV_status_t W25Q64FV_erase_block_32(uint32_t sector_address, bool hold){
    // check if busy
    if(W25Q64FV_busy())
        return W25Q64FV_BUSY;
    // check that writing is enables
    W25Q64FV_enable_writing();
    // write the command to erase
    byte buffer[3];
    buffer[0] = sector_address >> 16;
    buffer[1] = sector_address >> 8;
    buffer[2] = sector_address;
    W25Q64FV_status_t status = write_reg(W25Q64FV_INSTRUCTION_BLOCK_32K_ERASE, buffer, 3);
    if (status != W25Q64FV_OK)
        return status;
    // check for a hold
    if (hold)
        return W25Q64FV_wait_until_free(W25Q64FV_DEFAULT_TIMEOUT);
    return W25Q64FV_OK;
}
#endif

#if UNUSED
W25Q64FV_status_t W25Q64FV_get_jedec(byte *manufacture_id, byte *memory_type, byte *capacity) {
  // read the jedec id and information
  // check if busy
  if (W25Q64FV_busy())
    return W25Q64FV_BUSY;
  // get the jedec info
  byte buffer[3];
  W25Q64FV_status_t status = read_reg(W25Q64FV_INSTRUCTION_JEDEC_ID, buffer, 3);
  if (status != W25Q64FV_OK)
    return status;
  *manufacture_id = buffer[0];
  *memory_type = buffer[1];
  *capacity = buffer[2];
  return W25Q64FV_OK;
}
#endif

bool W25Q64FV_busy() {
  uint8_t status;
  select_device();
  SPI.transfer(W25Q64FV_INSTRUCTION_READ_STATUS_REGISTER_1);
  status = SPI.transfer(0);
  release_device();
  return (status & 0x01) != 0;
}

#define CYCLES_PER_MS (F_CPU / 1000UL)
W25Q64FV_status_t W25Q64FV_wait_until_free(unsigned long max_timeout_ms) {
  unsigned long max_cycles = max_timeout_ms * CYCLES_PER_MS;
  unsigned long elapsed_cycles = 0;
  while (W25Q64FV_busy()) {
    // Simulate a delay of 1 ms using a cycle-based delay
    for (unsigned long i = 0; i < CYCLES_PER_MS / 4; i++) {
      __asm__ __volatile__("nop"); // Each NOP takes 1 cycle
    }
    // Add 1 ms worth of cycles
    elapsed_cycles += CYCLES_PER_MS;

    // Check if the timeout has been exceeded
    if (elapsed_cycles >= max_cycles) {
      return W25Q64FV_TIMEOUT;
    }
  }
  return W25Q64FV_OK;
}

W25Q64FV_status_t W25Q64FV_reset() {
  // reset the device
  // requires writing the reset enable followed by reset command to complete
  // check if busy
  if (W25Q64FV_busy())
    return W25Q64FV_BUSY;
  // write the reset enable command
  W25Q64FV_status_t status = write_command(W25Q64FV_INSTRUCTION_ENABLE_RESET);
  if (status != W25Q64FV_OK)
    return status;
  status = write_command(W25Q64FV_INSTRUCTION_RESET);
  // delay
  _delay_us(35); // typical reset time 30 us
  return W25Q64FV_OK;
}
 
// Private functions
W25Q64FV_status_t read_reg(uint8_t reg, uint8_t *buffer, unsigned int length) {
  // check if busy
  if (W25Q64FV_busy())
    return W25Q64FV_BUSY;
  // select
  select_device();
  SPI.transfer(reg);
  while (length > 0) {
    *buffer = SPI.transfer(0);
    length--;
    *buffer++;
  }
  release_device();
  return W25Q64FV_OK;
}

W25Q64FV_status_t write_reg(uint8_t reg, uint8_t *buffer, unsigned int length) {
  // check if busy
  if (W25Q64FV_busy())
    return W25Q64FV_BUSY;
  // select
  select_device();
  // write instruction
  SPI.transfer(reg);
  // write accompyning data
  while (length > 0) {
    SPI.transfer(*buffer);
    length--;
    *buffer++;
  }
  release_device();
  return W25Q64FV_OK;
}

W25Q64FV_status_t write_command(uint8_t command) {
  // check if busy
  if (W25Q64FV_busy())
    return W25Q64FV_BUSY;
  // write the command
  select_device();
  SPI.transfer(command);
  release_device();
  return W25Q64FV_OK;
}

void select_device() {
  PORTB &= ~(1 << _cs); // CS low (select device)
}
void release_device() {
  PORTB |= (1 << _cs); // CS high (deselect device)
}
