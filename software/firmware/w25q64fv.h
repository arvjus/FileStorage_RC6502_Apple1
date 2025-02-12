/**
 * @file w25q64fv.h
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

#pragma once

#include <util/delay.h>
#include <stdbool.h>
#include "spi.h"


/********** INSTRUCTION SETS **********/
#define W25Q64FV_INSTRUCTION_WRITE_ENABLE 0x06
#define W25Q64FV_INSTRUCTION_VOLATILE_SR_WRITE_ENABLE 0x50
#define W25Q64FV_INSTRUCTION_WRITE_DISABLE 0x04
#define W25Q64FV_INSTRUCTION_READ_STATUS_REGISTER_1 0x05
#define W25Q64FV_INSTRUCTION_READ_STATUS_REGISTER_2 0x35
#define W25Q64FV_INSTRUCTION_WRITE_STATUS_REGISTER 0x01
#define W25Q64FV_INSTRUCTION_PAGE_PROGRAM 0x02
#define W25Q64FV_INSTRUCTION_SECTOR_4K_ERASE 0x20
#define W25Q64FV_INSTRUCTION_BLOCK_32K_ERASE 0x52
#define W25Q64FV_INSTRUCTION_BLOCK_64K_ERASE 0xD8
#define W25Q64FV_INSTRUCTION_CHIP_ERASE 0xC7 // might also be 0x60
#define W25Q64FV_INSTRUCTION_ERASE_PROGRAM_SUSPEND 0x75
#define W25Q64FV_INSTRUCTION_ERASE_PROGRAM_RESUME 0x7A
#define W25Q64FV_INSTRUCTION_POWER_DOWN 0xB9
#define W25Q64FV_INSTRUCTION_READ_DATA 0x03
#define W25Q64FV_INSTRUCTION_FAST_READ 0x0B
#define W25Q64FV_INSTRUCTION_RELEASE_POWERDOWN 0xAB
#define W25Q64FV_INSTRUCTION_MANUFACTURER_DEVICE_ID 0x90
#define W25Q64FV_INSTRUCTION_JEDEC_ID 0x9F
#define W25Q64FV_INSTRUCTION_READ_UNIQUE_ID 0x4B
#define W25Q64FV_INSTRUCTION_READ_SFDP_REGISTER 0x5A
#define W25Q64FV_INSTRUCTION_ERASE_SECURITY_REGISTERS 0x44
#define W25Q64FV_INSTRUCTION_PROGRAM_SECURITY_REGISTERS 0x42
#define W25Q64FV_INSTRUCTION_READ_SECURITY_REGISTERS 0x48
#define W25Q64FV_INSTRUCTION_ENABLE_QPI 0x38
#define W25Q64FV_INSTRUCTION_ENABLE_RESET 0x66
#define W25Q64FV_INSTRUCTION_RESET 0x99
// DUAL IO TABLE
#define W25Q64FV_INSTRUCTION_FAST_READ_DUAL_OUTPUT 0x3B
#define W25Q64FV_INSTRUCTION_FAST_READ_DUAL_IO 0xBB
#define W25Q64FV_INSTRUCTION_MANUFACTURER_DEVICE_ID_DUAL_IO 0x92
// QUAD IO TABLE
#define W25Q64FV_INSTRUCTION_QUAD_PAGE_PROGRAM 0x32
#define W25Q64FV_INSTRUCTION_FAST_READ_QUAD_OUTPUT 0x6B
#define W25Q64FV_INSTRUCTION_FAST_READ_QUAD_IO 0xEB
#define W25Q64FV_INSTRUCTION_WORD_READ_QUADO_IO 0xE7
#define W25Q64FV_INSTRUCTION_OCTAL_WORD_READ_QUAD_IO 0xE3
#define W25Q64FV_INSTRUCTION_SET_BURST_WRAP 0x77
#define W25Q64FV_INSTRUCTION_MANUFACTURER_DEVICE_ID_QUAD_IO 0x94

/********** SETTINGS **********/
#define W25Q64FV_SPI_SPEED 20000000 // 104 MHz (104 max)

#define W25Q64FV_DEFAULT_TIMEOUT 5000 // Default timeout for most operations
#define W25Q64FV_CHIP_ERASE_TIMEOUT                                            \
  100000 // Chip erase timeout. Per spec, this is typically 20 seconds, at most
         // 100 seconds.

/// Default Status Return Enum
typedef enum {
  W25Q64FV_OK = 0,             ///< Chip OK
  W25Q64FV_COMMUNICATION_FAIL, ///< Communication fail
  W25Q64FV_BUSY,               ///< Chip report busy
  W25Q64FV_TIMEOUT,            ///< Chip timeout on wait operation
  W25Q64FV_NOT_VALID           ///< Not a valid operation
} W25Q64FV_status_t;


typedef uint8_t byte;


/**
 * @brief Initialize communication with the flash chip
 *
 * Begins SPI communication and attempts to check the ID of the device.
 *
 * @param cs_pin                Chip select pin
 * @return W25Q64FV_status_t    Status return
 */
W25Q64FV_status_t W25Q64FV_begin(uint8_t cs_pin);

/**
 * @brief Enable writing to the flash chip
 *
 * Sets the enable register to allow writing to the flash chip
 *
 * @return W25Q64FV_status_t    Status return
 */
W25Q64FV_status_t W25Q64FV_enable_writing();

/**
 * @brief Disable writing to the device
 *
 * Sets the enable register to dissallow writing to the flash chip
 *
 * @return W25Q64FV_status_t    Status return
 */
W25Q64FV_status_t W25Q64FV_disable_writing();

/**
 * @brief Write a page to the flash chip
 *
 * Writes a single page (256 bytes) to the flash chip
 *
 * @param start_address         Start address to write to. Must be the start
 * of a page
 * @param buffer                Buffer of data to write
 * @return W25Q64FV_status_t    Status return
 */
W25Q64FV_status_t W25Q64FV_write_page(uint32_t start_address, byte *buffer);

/**
 * @brief Read a page from the flash chip
 *
 * Reads a single page (256 bytes) from the flash chip
 *
 * @param start_address         Start address to read from
 * @param buffer                Buffer of data to read into
 * @return W25Q64FV_status_t    Status return
 */
W25Q64FV_status_t W25Q64FV_read_page(uint32_t start_address, byte *buffer, const uint16_t size);

/**
 * @brief Erase a 32kB block from the flash chip
 *
 * Erases a single block of 32kB from the device. Address is truncated
 *
 * @param block_address         Block start address to erase
 * @param hold                  Hold for the device to finish the erase
 * @return W25Q64FV_status_t    Status return
 */
W25Q64FV_status_t W25Q64FV_erase_block_32(uint32_t block_address, bool hold);

/**
 * @brief Erase entire flash chip contents
 *
 * Erases the entire contents of the flash chip.
 *
 * @param hold                  Hold for the device to finish the erase
 * @return W25Q64FV_status_t    Status return
 */
W25Q64FV_status_t W25Q64FV_erase_chip(bool hold);

/**
 * @brief Get the jedec object id
 *
 * Returns the Jedec ID of the device
 *
 * @return W25Q64FV_status_t    Status return
 */
W25Q64FV_status_t W25Q64FV_get_jedec(byte *manufacture_id, byte *memory_type, byte *capacity);

/**
 * @brief Checks if the flash chip is busy
 *
 * Checks the status register and returns the state of the flash chip
 *
 * @return true     Device is busy (cannot execute new tasks)
 * @return false    Device is free (can execute new tasks)
 */
bool W25Q64FV_busy();

/**
 * @brief W25Q64FV_wait_until_free
 */
W25Q64FV_status_t  W25Q64FV_wait_until_free(unsigned long max_timeout);

/**
 * @brief Reset the flash chip
 *
 * Sends the reset enable and the reset command to reset the flash chip
 *
 * @return W25Q64FV_status_t    Status return
 */
W25Q64FV_status_t W25Q64FV_reset();

