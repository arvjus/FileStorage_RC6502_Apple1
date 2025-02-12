/*
Flash Disk Util
Copyright (c) 2025 Arvid Juskaitis
*/

#pragma once

#include "w25q64fv.h"
#include "defs.h"

#define PAGE_SIZE 256
#define BLOCK_SIZE 32768UL
#define MAX_BLOCKS (8192 / 32)
#define MAX_NAME_SIZE  26

// Define the structure for a file entry
typedef struct {
    uint16_t block;     // Block number, starting from 0
    uint16_t start;     // Start address of the file in memory (little endian)
    uint16_t size;      // Te size of the file in memory (little endian)
    char name[MAX_NAME_SIZE];  // File name, case-insensitive, padded with zeros
} FileEntry_t;

// Define status
typedef enum {
    OK = 0,
    FILE_ENTRY_IS_NOT_FOUND = 10, // 0x0a
    BLOCK_IS_NOT_VALID = 11,      // 0x0c
    INVALID_DATA = 12             // 0x0c
} SimpleFS_Status_t;

uint8_t SimpleFS_listFiles(uint8_t *buff, uint16_t *pblock, const char *prefix);
uint8_t SimpleFS_createFileEntry(uint8_t *buff, const char *input, uint16_t *pblock, uint16_t *psize);
uint8_t SimpleFS_writeFile(uint8_t *buff);
uint8_t SimpleFS_readFileByName(uint8_t *buff, const char *filename, uint16_t *psize);
uint8_t SimpleFS_readFileByBlockNo(uint8_t *buff, uint16_t block, uint16_t *psize);
uint8_t SimpleFS_readFileNextPage(uint8_t *buff);
uint8_t SimpleFS_deleteFileByName(uint8_t *buff, const char *filename);
uint8_t SimpleFS_deleteFileByBlockNo(uint8_t *buff, uint16_t block);
