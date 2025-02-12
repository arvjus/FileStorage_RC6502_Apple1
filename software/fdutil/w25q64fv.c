/*
Flash Disk Util
Copyright (c) 2025 Arvid Juskaitis
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w25q64fv.h"

#define FLASH_SIZE (8 * 1024 * 1024) // 8 MB size of W25Q64
#define PAGE_SIZE 256
#define BLOCK_SIZE_32K (32 * 1024) // 32 KB block size


static FILE *flash_file = NULL;
static size_t current_size = 0;

W25Q64FV_status_t W25Q64FV_init(const char *filename, short numberOfFiles) {
    if (!filename || numberOfFiles <= 0) {
        return W25Q64FV_NOT_VALID; // Invalid arguments
    }

    // Calculate the required size in bytes
    size_t required_size = (size_t)numberOfFiles * BLOCK_SIZE_32K;

    // Open the file for reading and writing, creating it if it doesn't exist
    flash_file = fopen(filename, "r+b");
    if (!flash_file) {
        flash_file = fopen(filename, "w+b");
        if (!flash_file) {
            return W25Q64FV_COMMUNICATION_FAIL; // Failed to create the file
        }
    }

    // Determine the current file size
    fseek(flash_file, 0, SEEK_END);
    size_t current_size = ftell(flash_file);

    // Adjust the file size as needed
    if (current_size < required_size) {
        // Extend the file
        fseek(flash_file, 0, SEEK_END);
        size_t extra_bytes = required_size - current_size;
        byte *buffer = (byte *)malloc(extra_bytes);
        if (!buffer) {
            fclose(flash_file);
            flash_file = NULL;
            return W25Q64FV_NOT_VALID; // Memory allocation failure
        }
        memset(buffer, 0xFF, extra_bytes); // Fill with 0xFF (erased state)
        fwrite(buffer, 1, extra_bytes, flash_file);
        free(buffer);
    } else if (current_size > required_size) {
        // Manually truncate by overwriting the file
        FILE *temp_file = fopen(filename, "wb");
        if (!temp_file) {
            fclose(flash_file);
            flash_file = NULL;
            return W25Q64FV_COMMUNICATION_FAIL;
        }
        byte *buffer = (byte *)malloc(required_size);
        if (!buffer) {
            fclose(temp_file);
            fclose(flash_file);
            flash_file = NULL;
            return W25Q64FV_NOT_VALID;
        }
        memset(buffer, 0xFF, required_size); // Fill with 0xFF
        fwrite(buffer, 1, required_size, temp_file);
        free(buffer);
        fclose(temp_file);
        flash_file = fopen(filename, "r+b"); // Reopen the resized file
        if (!flash_file) {
            return W25Q64FV_COMMUNICATION_FAIL;
        }
    }

    // Ensure changes are saved
    fflush(flash_file);

    return W25Q64FV_OK;
}

// Initialize simulated flash memory
W25Q64FV_status_t W25Q64FV_begin(const char* filename) {
    flash_file = fopen(filename, "r+b");
    if (!flash_file) {
        // Create the file if it doesn't exist
        flash_file = fopen(filename, "w+b");
        if (!flash_file) {
            return W25Q64FV_COMMUNICATION_FAIL;
        }
    }

    // Determine the current file size
    fseek(flash_file, 0, SEEK_END);
    current_size = ftell(flash_file);
    return W25Q64FV_OK;
}

// Write a page of data to the simulated flash
W25Q64FV_status_t W25Q64FV_write_page(uint32_t start_address, byte *buffer) {
    if (!flash_file || start_address >= current_size || !buffer) {
        return W25Q64FV_NOT_VALID;
    }
    fseek(flash_file, start_address, SEEK_SET);
    fwrite(buffer, 1, PAGE_SIZE, flash_file);
    fflush(flash_file); // Ensure data is written to disk
    return W25Q64FV_OK;
}

// Read a page of data from the simulated flash
W25Q64FV_status_t W25Q64FV_read_page(uint32_t start_address, byte *buffer, uint16_t size) {
    if (!flash_file || start_address + size > current_size || !buffer) {
        return W25Q64FV_NOT_VALID;
    }
    fseek(flash_file, start_address, SEEK_SET);
    fread(buffer, 1, size, flash_file);
    return W25Q64FV_OK;
}

// Erase the entire chip by setting it to 0xFF
W25Q64FV_status_t W25Q64FV_erase_chip(bool hold) {
    (void)hold; // Unused
    if (!flash_file) {
        return W25Q64FV_NOT_VALID;
    }
    fseek(flash_file, 0, SEEK_SET);
    byte empty[FLASH_SIZE];
    memset(empty, 0xFF, current_size);
    fwrite(empty, 1, current_size, flash_file);
    fflush(flash_file);
    return W25Q64FV_OK;
}

W25Q64FV_status_t W25Q64FV_erase_block_32(uint32_t block_address, bool hold) {
    (void)hold; // Unused, for compatibility with hardware behavior
    if (!flash_file || block_address >= current_size || block_address % BLOCK_SIZE_32K != 0) {
        return W25Q64FV_NOT_VALID; // Invalid address or uninitialized file
    }

    // Move to the block address
    fseek(flash_file, block_address, SEEK_SET);

    // Fill the block with 0xFF
    byte empty[BLOCK_SIZE_32K];
    memset(empty, 0xFF, BLOCK_SIZE_32K);

    if (fwrite(empty, 1, BLOCK_SIZE_32K, flash_file) != BLOCK_SIZE_32K) {
        return W25Q64FV_COMMUNICATION_FAIL; // Write operation failed
    }

    fflush(flash_file); // Ensure changes are written to the file
    return W25Q64FV_OK;
}

// Close the simulated flash file
W25Q64FV_status_t W25Q64FV_end() {
    if (flash_file) {
        fclose(flash_file);
        flash_file = NULL;
    }
    return W25Q64FV_OK;
}

bool W25Q64FV_busy() {
    return false;
}

W25Q64FV_status_t W25Q64FV_wait_until_free(unsigned long max_timeout_ms) {
    return W25Q64FV_OK;
}

W25Q64FV_status_t W25Q64FV_enable_writing() {
    return W25Q64FV_OK;
}

