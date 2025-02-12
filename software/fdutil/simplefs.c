/*
Flash Disk Util
Copyright (c) 2025 Arvid Juskaitis
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "simplefs.h"

/*'
 *  Helper functions/ predicates
 */
int strncasecmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned char c1 = tolower((unsigned char)s1[i]);
        unsigned char c2 = tolower((unsigned char)s2[i]);
        if (c1 != c2) {
            return c1 - c2;
        }
        if (c1 == '\0') { // End of string
            break;
        }
    }
    return 0;
}

#if WRITE
bool availableEntry(FileEntry_t *fe, void *context) { return fe->block == 0xffff; }
#endif

#if LIST
bool nameBeginsWith(FileEntry_t *fe, void *context) {
  const char *prefix = (const char *)context;
  return fe->block != 0xffff && (!prefix || strncasecmp(fe->name, prefix, strlen(prefix)) == 0);
}
#endif

#if READ || DELETE
bool nameExactMatch(FileEntry_t *fe, void *context) {
  const char *name = (const char *)context;
  return fe->block != 0xffff && strncasecmp(fe->name, name, strlen(fe->name)) == 0;
}
#endif

#if  LIST || READ || WRITE || DELETE
uint8_t find_entry(uint8_t *buff, uint16_t *pblock, bool (*predicate)(FileEntry_t *, void *), void *context) {
  for (uint16_t block = *pblock; block < MAX_BLOCKS; block++) {
      uint8_t status = W25Q64FV_read_page(block * BLOCK_SIZE, buff, sizeof(FileEntry_t));
    if (status != W25Q64FV_OK) {
      return status;
    }
    if (predicate((FileEntry_t *)buff, context)) {
      *pblock = block;
      return OK;
    }
  }
  return FILE_ENTRY_IS_NOT_FOUND;
}
#endif

// Function to parse the input string
#if WRITE
bool parseWriteFileInput(const char *input, char *name, uint16_t *pstart, uint16_t *pstop) {
    // Initialize outputs
    *pstart = *pstop = 0x0000;

    // Locate the first colon
    const char *hash = strchr(input, '#');
    if (!hash) {
        return false; // Invalid format, no colon found
    }

    // Copy and pad the name
    size_t name_len = hash - input;
    if (name_len >= MAX_NAME_SIZE) {
        name_len = MAX_NAME_SIZE - 1; // Truncate if too long
    }
    strncpy(name, input, name_len);
    for (size_t i = name_len; i < MAX_NAME_SIZE; i++) {
        name[i] = '\0';
    }

    // Parse the start value
    const char *current = hash + 1;
    char *endptr;
    *pstart = (uint16_t) strtoul(current, &endptr, 16);
    if (*endptr != '#') {
        return false; // Invalid format
    }

    // Parse the size value
    current = endptr + 1;
    *pstop = (uint16_t) strtoul(current, &endptr, 16);

    return true; // Successfully parsed
}
#endif

#if READ || WRITE
static uint32_t current_page_address;
#endif

#if LIST
uint8_t SimpleFS_listFiles(uint8_t *buff, uint16_t *pblock, const char *prefix) {
  return find_entry(buff, pblock, nameBeginsWith, (void *)prefix);
}
#endif

#if WRITE
uint8_t SimpleFS_createFileEntry(uint8_t *buff, const char *input, uint16_t *pblock, uint16_t *psize) {
  uint8_t status = find_entry(buff, pblock, availableEntry, (void*)0);
  if (status == OK) {
    memset(buff, 0, PAGE_SIZE);
    FileEntry_t *fe = (FileEntry_t *)buff;
    uint16_t stop;
    if (!parseWriteFileInput(input, fe->name, &(fe->start), &stop) || fe->start > stop) {
      return INVALID_DATA;
    }
    fe->block = *pblock;
    fe->size = stop - fe->start;
    *psize = sizeof(FileEntry_t) + fe->size;
    current_page_address = *pblock * BLOCK_SIZE;
  }
  return status;
}

uint8_t SimpleFS_writeFile(uint8_t *buff) {
  W25Q64FV_enable_writing();
  W25Q64FV_write_page(current_page_address, buff);
  W25Q64FV_wait_until_free(W25Q64FV_DEFAULT_TIMEOUT);
  current_page_address += PAGE_SIZE;
  return W25Q64FV_OK;
}
#endif

#if READ
uint8_t SimpleFS_readFileByName(uint8_t *buff, const char *filename, uint16_t *psize) {
  uint16_t block = 0;
  uint8_t status = find_entry(buff, &block, nameExactMatch, (void *)filename);
  if (status == OK) {
    FileEntry_t *fe = (FileEntry_t *)buff;
    current_page_address = block * BLOCK_SIZE;
    *psize = sizeof(FileEntry_t) + fe->size;
    status = W25Q64FV_read_page(current_page_address, buff, PAGE_SIZE);
  }
  return status;
}

uint8_t SimpleFS_readFileByBlockNo(uint8_t *buff, uint16_t block, uint16_t *psize) {
  uint8_t status = W25Q64FV_read_page(block * BLOCK_SIZE, buff, sizeof(FileEntry_t));
  if (status != W25Q64FV_OK) {
    return status;
  }
  FileEntry_t *fe = (FileEntry_t *)buff;
  if (fe->block == block) {
    current_page_address = block * BLOCK_SIZE;
    *psize = sizeof(FileEntry_t) + fe->size;
    status = W25Q64FV_read_page(current_page_address, buff, PAGE_SIZE);
  } else {
    status = BLOCK_IS_NOT_VALID;
  }
  return status;
}

uint8_t SimpleFS_readFileNextPage(uint8_t *buff) {
  current_page_address += PAGE_SIZE;
  return W25Q64FV_read_page(current_page_address, buff, PAGE_SIZE);
}
#endif

#if DELETE
uint8_t SimpleFS_deleteFileByName(uint8_t *buff, const char *filename) {
  uint16_t block = 0;
  uint8_t status = find_entry(buff, &block, nameExactMatch, (void *)filename);
  if (status == OK) {
    return W25Q64FV_erase_block_32(block * BLOCK_SIZE, true);
  }
  return status;
}

uint8_t SimpleFS_deleteFileByBlockNo(uint8_t *buff, uint16_t block) {
#if 0   // force
    uint8_t status = W25Q64FV_erase_block_64(block * BLOCK_SIZE, true);
#else
  uint8_t status = W25Q64FV_read_page(block * BLOCK_SIZE, buff, 32);
  if (status != W25Q64FV_OK) {
    return status;
  }
  FileEntry_t *fe = (FileEntry_t *)buff;
  if (fe->block == block) {
    status = W25Q64FV_erase_block_32(block * BLOCK_SIZE, true);
  } else {
    status = BLOCK_IS_NOT_VALID;
  }
#endif
  return status;
}
#endif
