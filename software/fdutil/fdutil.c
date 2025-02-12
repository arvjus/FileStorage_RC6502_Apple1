/*
Flash Disk Util
Copyright (c) 2025 Arvid Juskaitis
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simplefs.h"
#include "w25q64fv.h"

// Function Prototypes
void usage(const char *progname);
int handle_init(const char *imagefile, short numberOfBlocks);
int handle_move(const char *imagefile, short firstBlock);
int handle_list(const char *imagefile, const char *prefix);
int handle_write(const char *imagefile, const char *input, const char *filename);
int handle_read(const char *imagefile, const char *input, const char *filename);
int handle_delete(const char *imagefile, const char *command);

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    const char *command = argv[2];

    if (strcmp(command, "i") == 0) {
        if (argc != 4) {
            usage(argv[0]);
            return 1;
        }
        short numberOfBlocks = (short) atoi(argv[3]);
        return handle_init(filename, numberOfBlocks);
    } else if (strcmp(command, "m") == 0) {
        if (argc != 4) {
            usage(argv[0]);
            return 1;
        }
        short firstBlock = (short) atoi(argv[3]);
        return handle_move(filename, firstBlock);
    } else if (strcmp(command, "l") == 0 || strncmp(command, "l", 1) == 0) {
        const char *prefix = command + 1; // Extract prefix if any
        return handle_list(filename, prefix);
    } else if (command[0] == 'w') {
        if (argc != 4) {
            usage(argv[0]);
            return 1;
        }
        return handle_write(filename, command + 1, argv[3]);
    } else if (command[0] == 'r') {
        if (argc != 4) {
            usage(argv[0]);
            return 1;
        }
        return handle_read(filename, command + 1, argv[3]);
    } else if (command[0] == 'd') {
        return handle_delete(filename, command + 1);
    }

    usage(argv[0]);
    return 1;
}

void usage(const char *progname) {
    printf("Usage: %s <image_file> <command> [args]\n", progname);
    printf("Commands:\n");
    printf("  i <num_blocks>                Initialize image for <num_blocks> blocks\n");
    printf("  m <first_block>               Move/reindex image starting with <first_block>\n");
    printf("  l[prefix]                     List files, optionally filtered by prefix\n");
    printf("  w<name>#<start>#<stop> <file> Write file to disk image. start, stop - hex\n");
    printf("  r<name|#block> <file>         Read file by name or block ID\n");
    printf("  d<name|#block>                Delete file by name or block ID\n");
}

int handle_init(const char *imagefile, short numberOfBlocks) {
    if (W25Q64FV_init(imagefile, numberOfBlocks) != W25Q64FV_OK) {
        fprintf(stderr, "Error: Failed to initialize file system image.\n");
        return 1;
    }
    printf("File system initialized for %d blocks.\n", numberOfBlocks);
    W25Q64FV_end();
    return 0;
}

int handle_move(const char *imagefile, short firstBlock) {
    FILE *file = fopen(imagefile, "r+b"); // Open for reading and writing in binary mode
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    uint16_t blockIndex = firstBlock;
    uint8_t buffer[BLOCK_SIZE];
    size_t bytesRead;
    size_t blockOffset = 0;

    while ((bytesRead = fread(buffer, 1, BLOCK_SIZE, file)) == BLOCK_SIZE) {
        // Update first two bytes with the new block index (little-endian)
        buffer[0] = blockIndex & 0xFF;
        buffer[1] = (blockIndex >> 8) & 0xFF;

        // Move back and write the modified block
        fseek(file, blockOffset, SEEK_SET);
        if (fwrite(buffer, 1, BLOCK_SIZE, file) != BLOCK_SIZE) {
            perror("Error writing to file");
            fclose(file);
            return 1;
        }

        blockIndex++;
        blockOffset += BLOCK_SIZE;
        fseek(file, blockOffset, SEEK_SET); // Move to next block position
    }

    if (ferror(file)) {
        perror("Error reading file");
        return 1;
    }

    fclose(file);
    return 0;
}

int handle_list(const char *imagefile, const char *prefix) {
    uint8_t buffer[BLOCK_SIZE + PAGE_SIZE];
    uint16_t block = 0;

    if (W25Q64FV_begin(imagefile) != W25Q64FV_OK) {
        fprintf(stderr, "Error: Failed to open file system image.\n");
        return 1;
    }

    uint8_t status;
    FileEntry_t *entry = (FileEntry_t *) buffer;
    printf("Start    Stop  Size Blck Name\n-----------------------------\n");
     do {
        status = SimpleFS_listFiles(buffer, &block, prefix);
        if (status != OK) break;
        printf("$%04X - $%04X %5d %4d %s\n", 
            entry->start, entry->start + entry->size, entry->size, entry->block, entry->name);
        block++;
    } while (status == OK);

    W25Q64FV_end();
    return 0;
}

int handle_write(const char *imagefile, const char *input, const char *filename) {
    char name[MAX_NAME_SIZE], name_start_stop[MAX_NAME_SIZE + 20];
    uint16_t start = 0, stop = 0, size = 0;
    uint8_t buffer[BLOCK_SIZE + PAGE_SIZE];

    if (sscanf(input, "%[^#]#%hx#%hx", name, &start, &stop) < 3) {
        fprintf(stderr, "Error: Invalid syntax for write.\n");
        return 1;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open input file %s.\n", filename);
        W25Q64FV_end();
        return 1;
    }
    // Determine the current file size
    fseek(fp, 0, SEEK_END);
    uint16_t actual_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    sprintf(name_start_stop, "%s#%x#%x", name, start, start + actual_size);

    if (W25Q64FV_begin(imagefile) != W25Q64FV_OK) {
        fprintf(stderr, "Error: Failed to open file system image.\n");
        return 1;
    }

    uint16_t block = 0;
    size = actual_size;
    memset(buffer, 0xFF, BLOCK_SIZE);
    if (SimpleFS_createFileEntry(buffer, name_start_stop, &block, &size) != OK) {
        fprintf(stderr, "Error: Failed to create file entry for %s.\n", name);
        W25Q64FV_end();
        return 1;
    }
    fprintf(stdout, "Number of bytes to write: %d\n", actual_size);
    fread(buffer + sizeof(FileEntry_t), 1, BLOCK_SIZE - sizeof(FileEntry_t), fp);

    uint8_t *ptr = buffer;
    int16_t size_written = 0;
    while (size_written < size) {
        uint8_t status = SimpleFS_writeFile(ptr);
        if (status != OK) {
            fprintf(stderr, "Error: Failed to write file for %s.\n", name);
            fclose(fp);
            return 1;
        }
        size_written += PAGE_SIZE;
        ptr += PAGE_SIZE;
    }
    fclose(fp);

    printf("File %s size_written successfully.\n", name);
    W25Q64FV_end();
    return 0;
}

int handle_read(const char *imagefile, const char *input, const char *filename) {
    uint8_t buffer[BLOCK_SIZE];
    uint16_t size;
    uint8_t status;

    if (W25Q64FV_begin(imagefile) != W25Q64FV_OK) {
        fprintf(stderr, "Error: Failed to open file system image.\n");
        return 1;
    }

    if (input[0] == '#') {
        uint16_t block = atoi(input + 1);
        status = SimpleFS_readFileByBlockNo(buffer, block, &size);
    } else {
        status = SimpleFS_readFileByName(buffer, input, &size);
    }
    if (status != OK) {
        fprintf(stderr, "Error: Failed to read file %s.\n", input);
        W25Q64FV_end();
        return 1;
    }

    uint8_t *ptr = buffer + PAGE_SIZE;  // we have alread read 1st buffer
    uint16_t current_size = PAGE_SIZE;
    while (current_size < size) {
        status = SimpleFS_readFileNextPage(ptr);
        if (status != OK) {
            fprintf(stderr, "Error: Failed to read file %s.\n", input);
            W25Q64FV_end();
            return 1;
        }
        ptr += PAGE_SIZE;
        current_size += PAGE_SIZE;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open output file %s.\n", filename);
        W25Q64FV_end();
        return 1;
    }

    fwrite(buffer + sizeof(FileEntry_t), 1, size - sizeof(FileEntry_t), fp);
    fclose(fp);

    printf("File %s read successfully to %s.\n", input, filename);
    W25Q64FV_end();
    return 0;
}

int handle_delete(const char *imagefile, const char *command) {
    uint8_t buffer[PAGE_SIZE];
    uint16_t block = (command[0] == '#') ? atoi(command + 1) : 0;
    uint8_t status;

    if (W25Q64FV_begin(imagefile) != W25Q64FV_OK) {
        fprintf(stderr, "Error: Failed to open file system image.\n");
        return 1;
    }

    if (command[0] == '#') {
        status = SimpleFS_deleteFileByBlockNo(buffer, block);
    } else {
        status = SimpleFS_deleteFileByName(buffer, command);
    }

    if (status != OK) {
        fprintf(stderr, "Error: Failed to delete file %s.\n", command);
        W25Q64FV_end();
        return 1;
    }

    printf("File %s deleted successfully.\n", command);
    W25Q64FV_end();
    return 0;
}
