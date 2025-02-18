/*
RC6502 FLASH (W25Q64F) Card firmware

PINC  - CPU -> MCU
PORTA - MCU -> CPU
PD2   - LEWRITE-
PD6   - CLEWRITE-
*/

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "simplefs.h"
#include "uart.h"

#define DEBUG   0
#define BAUD 250000

/* ------------------------------------------------------------------------
 *  Protocol constants
 * ------------------------------------------------------------------------
 */

// Status bits
#define RDY_FLAG    (1 << 7)  // MCU sets if data is ready
#define BSY_FLAG    (1 << 6)  // MCU sets if busy
#define ACK_FLAG    (1 << 5)  // ACK, NACK
#define DAT_FLAG    (1 << 4)  // Data nibble flag

// Commands
#define CMD_RESET   0x00
#define CMD_LIST    0x01
#define CMD_READ    0x02
#define CMD_WRITE   0x03
#define CMD_DELETE  0x04

// Markers - Note by setting markers we're setting RDY_FLAG and clearing BSY_FLAG
#define BODT        0x80
#define EODT        0x8F
#define ACK         0xA0
#define NACK        0xAF

/* ------------------------------------------------------------------------
 *  Ports / Bit manipulation
 * ------------------------------------------------------------------------
 */
#define MCU_IN          PINC
#define MCU_OUT         PORTA

#define SET_RDY_FLAG()  (MCU_OUT |= RDY_FLAG)
#define CLR_RDY_FLAG()  (MCU_OUT &= ~RDY_FLAG)

#define SET_BSY_FLAG()  (MCU_OUT |= BSY_FLAG)
#define CLR_BSY_FLAG()  (MCU_OUT &= ~BSY_FLAG)

#define SET_DAT_FLAG()  (MCU_OUT |= DAT_FLAG)
#define CLR_DAT_FLAG()  (MCU_OUT &= ~DAT_FLAG)

#define SET_CLEWRITE()  (PORTD |= (1 << PD6))
#define CLR_CLEWRITE()  (PORTD &= ~(1 << PD6))

// State machine enumerations
typedef enum {
    SM_IDLE = 0,            // Doing nothing, waiting for next command from CPU
    SM_RECEIVE_CMD = 1,     // We are receiving a new command
    SM_PROCESS_CMD = 2,     // We have a command to process
    SM_SEND_DATA = 3,       // We are sending data back to CPU
    SM_RECEIVE_DATA = 4,    // We are receiving data from CPU
    SM_FINISH = 5,          // We are about to finish with current command
} mcu_state_t;

// global variables
volatile uint8_t state = SM_IDLE;
volatile uint8_t command = 0, ms_nibble = 0;
volatile uint16_t block = 0;
volatile bool handle_disk_data = false; // set true to request more data for CMD_LIST and CMD_READ, set true to flush data for CMD_WRITE
volatile uint16_t buff_idx = 0, buff_max = 0, file_size = 0;
volatile uint8_t buff[PAGE_SIZE];
char buff_aux[MAX_NAME_SIZE];

// forward declarations
void init_mcu();
void reset();
void send_data_nibble();
bool handle_cmd_list(bool init);
bool handle_cmd_read(bool initial);
bool handle_cmd_write(bool initial);
bool handle_cmd_delete();
#if BULK_TRANSFER
void bulk_erase();
void bulk_read();
void bulk_write();
#endif            
#if DEBUG
void value_to_hex(uint8_t value);
void print_msg(const char *msg);
void print_msg_string(const char *msg, const char *value);
void print_msg_hex(const char *msg, uint16_t value);
void print_status();
void print_buffer();
#else
#define print_msg(msg)              /**/
#define print_msg_string(msg,value) /**/
#define print_msg_hex(msg,value)    /**/
#define print_status()              /**/
#define print_buffer()              /**/
#endif

int main(void) {
    init_mcu();
    if (W25Q64FV_begin(PB4) == W25Q64FV_OK)
        print_msg("FD ");

    reset();
    MCU_OUT = 0x00; // not busy, not ready
    while (1) {
        // Check for UART command
        if (uart_available()) {
            uint8_t ch = uart_receive();
            if (ch == 'r') reset();
            else if (ch == 's') print_status();
            else if (ch == 'b') print_buffer();
#if BULK_TRANSFER
            else if (ch == 'E') bulk_erase();
            else if (ch == 'R') bulk_read();
            else if (ch == 'W') bulk_write();
#endif            
        }

        switch (state) {
            case SM_IDLE:
                break;

            case SM_PROCESS_CMD:
                // New request from CPU, buff contains request data
                switch (command) {
                    case CMD_LIST:
                        if (handle_cmd_list(true)) {
                            state = SM_SEND_DATA;
                            MCU_OUT = BODT;
                        } else {
                            state = SM_IDLE;
                            MCU_OUT = EODT;
                        }
                        break;
                    case CMD_READ:
                        if (handle_cmd_read(true)) {
                            state = SM_SEND_DATA;
                            MCU_OUT = BODT;
                        } else {
                            state = SM_IDLE;
                            MCU_OUT = EODT;
                        }
                        break;
                    case CMD_WRITE:
                        if (handle_cmd_write(true)) {
                            state = SM_RECEIVE_DATA;
                            MCU_OUT = ACK;
                        } else {
                            state = SM_IDLE;
                            MCU_OUT = NACK;
                        }
                        break;
                    case CMD_DELETE:
                        if (handle_cmd_delete()) {
                            MCU_OUT = ACK;
                        } else {
                            MCU_OUT = NACK;
                        }
                        state = SM_IDLE;
                        break;
                }
                break;

            case SM_SEND_DATA:
                if (command == CMD_LIST && handle_disk_data) {
                    handle_disk_data = false;
                    if (handle_cmd_list(false)) {
                        send_data_nibble();
                    } else {
                        state = SM_FINISH;
                        MCU_OUT = EODT;
                    }
                } else if (command == CMD_READ && handle_disk_data) {
                    handle_disk_data = false;
                    if (buff_max == PAGE_SIZE && handle_cmd_read(false)) {
                        send_data_nibble();
                    } else {
                        state = SM_FINISH;
                        MCU_OUT = EODT;
                    }
                }
                break;
            case SM_FINISH:
            case SM_RECEIVE_DATA:
                if (command == CMD_WRITE && handle_disk_data) {
                    handle_disk_data = false;
                    if (handle_cmd_write(false)) {
                        if (buff_max == 0) {
                            reset();
                        }
                        MCU_OUT = ACK;
                    } else {
                        MCU_OUT = NACK;
                    }
                }
                break;
            default:
                break;
        }
    }
}

// Interrupt Service Routine for INT0
ISR(INT0_vect) {
    // Set BSY_FLAG, clear RDY_FLAG
    MCU_OUT = BSY_FLAG;

    uint8_t in_byte = MCU_IN;
#if DEBUG
    value_to_hex(in_byte);
    uart_transmit_string(buff_aux);
#endif
    if (in_byte & DAT_FLAG) {
        if (state == SM_RECEIVE_CMD || state == SM_RECEIVE_DATA) {
            if (buff_idx < sizeof(buff)) {
                if (ms_nibble) {
                    buff[buff_idx ++] = ((ms_nibble & 0x0f) << 4) | (in_byte & 0x0f);
                    ms_nibble = 0;
                    if (buff_idx >= sizeof(buff)) {
                        handle_disk_data = true;
                    }                  
                } else {
                    ms_nibble = in_byte;
                }
            } else {
                print_msg("MEM!");  // we're out of bounds - very bad
            }
            if (!handle_disk_data) {
                MCU_OUT = ACK;
            }
        }
    } else {
        // 5th bit not set -> control/status mode
        switch (in_byte) {
            case CMD_RESET:
                reset();
                MCU_OUT = ACK;
                break;
            case CMD_LIST:
            case CMD_READ:
            case CMD_WRITE:
            case CMD_DELETE:
                command = in_byte;
                state = SM_RECEIVE_CMD;
                buff_max = MAX_NAME_SIZE;  // max number of bytes to transfer
                buff_idx = 0;   // from 0
                ms_nibble = 0;  // no last nibble
                handle_disk_data = false;
                MCU_OUT = ACK;
                break;
            case BODT:
                MCU_OUT = ACK;
                break;
            case EODT:
                if (state == SM_RECEIVE_CMD) {
                    // here have we received command, start processing..
                    buff[buff_idx] = '\0';
                    state = SM_PROCESS_CMD;
                } else if (command == CMD_WRITE && state == SM_RECEIVE_DATA) {
                    // finish writing rest of data to disk
                    state = SM_FINISH;
                    MCU_OUT = ACK;
                    handle_disk_data = true;
                }
                break;
            case ACK:
                switch (command) {
                    case CMD_LIST:
                    case CMD_READ:
                        if (state == SM_SEND_DATA) {
                            if (buff_idx < buff_max) {
                                send_data_nibble();
                            } else {            // end of buffer
                                handle_disk_data = true;
                            }
                        }
                        break;
                    case CMD_WRITE:
                    case CMD_DELETE:
                        break;
                    default:
                        print_msg("ACK?");
                        break;
                }

                if (state == SM_FINISH) {
                    print_msg("FIN");
                    reset();
                    MCU_OUT = 0x00;     // not busy, not ready
                }
                CLR_BSY_FLAG();
                break;
            case NACK:
                print_msg("NACK");
                reset();
                MCU_OUT = 0x00; // not busy, not ready
                break;
            default:
                // Some other status or control byte
                break;
        }
    }

    // Strobe CLEWRITE for 1 ms
    CLR_CLEWRITE();
    _delay_ms(1);
    SET_CLEWRITE();
}


void init_mcu() {
    // Initialize UART with calculated UBRR
    uart_init(F_CPU / 16 / BAUD - 1);
#if DEBUG
    // Send a string over UART
    print_msg("RC6502 ");
#endif

    // Configure PORTA, PINC
    DDRA = 0xff;    // output
    DDRC = 0x00;    // input

    // Configure PD6 as an output and set it high
    DDRD |= (1 << PD6);  // Set PD6 as output
    PORTD |= (1 << PD6); // Set PD6 high

    // Configure PD2 (INT0) for falling edge trigger
    MCUCR |= (1 << ISC01);  // ISC01 = 1, ISC00 = 0 for falling edge
    MCUCR &= ~(1 << ISC00); // Ensure ISC00 is 0

    // Enable external interrupt INT0
    GICR |= (1 << INT0);

    // Enable global interrupts
    sei();
}

void reset() {
    print_msg("RST");
    state = SM_IDLE;
    command = 0;
    block = 0;
    ms_nibble = 0;
    buff_max = 0;
    buff_idx = 0;
    handle_disk_data = false;
    file_size = 0;
}

void send_data_nibble() {
    if (ms_nibble == 0) {
        MCU_OUT = RDY_FLAG | DAT_FLAG | ((buff[buff_idx] >> 4) & 0x0f);
        ms_nibble = DAT_FLAG;
    } else {
        MCU_OUT = RDY_FLAG | DAT_FLAG | (buff[buff_idx] & 0x0f);
        ms_nibble = 0;
        buff_idx++;
    }
}

bool handle_cmd_list(bool initial) {
#if LIST
    buff_max = sizeof(FileEntry_t);  // number of bytes to transfer
    buff_idx = 0;   // reset index
    ms_nibble = 0;  // no last nibble
    // SimpleFS_listFiles will use 32 bytes of the buff, so we can reuse other space in th buffer
    char* prefix = (char*)buff + 100;
    if (initial) {
        memcpy(prefix, (const char*)buff, MAX_NAME_SIZE);
        block = 0;
        print_msg_string("L!", prefix);
    } else {
        block ++;
    }

    uint8_t status = SimpleFS_listFiles((uint8_t*)buff, (uint16_t*)&block, prefix);
    if (status != OK && status != FILE_ENTRY_IS_NOT_FOUND) {
        buff_max = 0;
        print_msg_hex("err:", status);
    }
    return status == OK; // true if buffer contains a valid file entry
#else
    return false;
#endif
}

bool handle_cmd_read(bool initial) {
#if READ
    buff_max = PAGE_SIZE;
    buff_idx = 0;   // reset index
    ms_nibble = 0;  // no last nibble
    uint8_t status;
    if (initial) {
        print_msg_string("R!", (const char *)buff);
        if (*buff == '#') {
            status = SimpleFS_readFileByBlockNo((uint8_t*)buff, (uint8_t)atoi((const char*)buff+1), (uint16_t*)&file_size);
        } else {
            memcpy(buff_aux, (const char*)buff, sizeof(buff_aux));
            status = SimpleFS_readFileByName((uint8_t*)buff, buff_aux, (uint16_t*)&file_size);
        }
    } else {
        status = SimpleFS_readFileNextPage((uint8_t*)buff);
    }
    if (status == OK) {
        buff_max = file_size < PAGE_SIZE ? file_size : PAGE_SIZE;
        file_size -= buff_max;  // we already have page in the buffer
    } else {
        print_msg_hex("err:", status);
    }
    return status == OK; // true if buffer contains a valid data
#else
    return false;
#endif
}

bool handle_cmd_write(bool initial) {
#if WRITE
    uint8_t status;
    if (initial) {          // create a file structure first
        print_msg_string("W!", (const char *)buff);
        memcpy(buff_aux, (const char*)buff, sizeof(buff_aux));
        uint16_t block = 0;  // A new entry will be allocated starting from this block
        status = SimpleFS_createFileEntry((uint8_t*)buff, buff_aux, &block, (uint16_t*)&file_size);
        buff_max = file_size < PAGE_SIZE ? file_size : PAGE_SIZE;
        buff_idx = sizeof(FileEntry_t);
        ms_nibble = 0;  // no last nibble
        print_status();
    } else {
        print_msg_string("W!", "");
        print_status();
        print_buffer();
        status = SimpleFS_writeFile((uint8_t*)buff);
        file_size -= buff_max;
        buff_max = file_size < PAGE_SIZE ? file_size : PAGE_SIZE;
        buff_idx = 0;
        print_status();
    }
    if (status != OK) {
        print_msg_hex("err:", status);
    }
    return status == OK; // true if data is written and needs more to write
#else
    return false;
#endif
}

bool handle_cmd_delete() {
#if DELETE
    uint8_t status;
    print_msg_string("D!", (const char *)buff);
    if (*buff == '#') {
        status = SimpleFS_deleteFileByBlockNo((uint8_t*)buff, (uint8_t)atoi((const char*)buff+1));
    } else {
        memcpy(buff_aux, (const char*)buff, sizeof(buff_aux));
        status = SimpleFS_deleteFileByName((uint8_t*)buff, buff_aux);
    }
    if (status != OK && status != FILE_ENTRY_IS_NOT_FOUND) {
        print_msg_hex("err:", status);
    }
    return status == OK; // true if buffer contains a valid data
#else
    return false;
#endif
}

#if DEBUG
void value_to_hex(uint8_t value) {
    buff_aux[0] = '0';
    itoa(value, value < 0xf ? &buff_aux[1] : buff_aux, 16);
    buff_aux[2] = ' ';
    buff_aux[3] = '\0';
}

void print_msg(const char *msg) {
    uart_transmit_string(msg);
}

void print_msg_string(const char *msg, const char *value) {
    uart_transmit_string(msg);
    uart_transmit_string(value);
}

void print_msg_hex(const char *msg, uint16_t value) {
    uart_transmit_string(msg);
    itoa(value, buff_aux, 16);
    uart_transmit_string(buff_aux);
}

void print_status() {
    print_msg_hex("st:", state);
    print_msg_hex(",cmd:", command);
    print_msg_hex(",idx:", buff_idx);
    print_msg_hex(",max:", buff_max);
    print_msg_hex(",fs:", file_size);
}
void print_buffer() {
    for (int i = 0; i < buff_max; i++) {
        value_to_hex(buff[i]);
        uart_transmit_string(buff_aux);
    }
}
#endif

#if BULK_TRANSFER
#include "w25q64fv.h"
// two bytes are expected- number of pages in binary, little endian. 
uint16_t receive_uint16() {
    while (!uart_available());
    uint8_t lsb = uart_receive();
    while (!uart_available());
    uint8_t msb = uart_receive();
    return (msb << 8) | lsb;
}

// one parameter is expected.
// size - number of 32k blocks to erase. if 0 is given, all chip is erased
void bulk_erase() {
    uint16_t size = receive_uint16();
    if (size) {
        W25Q64FV_status_t status = W25Q64FV_OK;
        for (int i = 0; i < size && status == W25Q64FV_OK; i++) {
            uint32_t address = ((uint32_t)i) * PAGE_SIZE;
            status = W25Q64FV_erase_block_32(address, true);
        }
        uart_transmit(status == W25Q64FV_OK ? ACK : NACK);
    } else {
        W25Q64FV_status_t status = W25Q64FV_erase_chip(true);
        uart_transmit(status == W25Q64FV_OK ? ACK : NACK);
    }
}

// one parameter is expected.
// size - in terms of page size. if 0 is given, all size 32768 is assumed
void bulk_read() {
    uint16_t size = receive_uint16();
    if (!size) {
        size = 32768;
    }
    for (int i = 0; i < size; i++) {
        uint32_t address = ((uint32_t)i) * PAGE_SIZE;
        W25Q64FV_status_t status = W25Q64FV_read_page(address, (byte*)buff, PAGE_SIZE);
        if (status == W25Q64FV_OK) {
            for (int j = 0; j < PAGE_SIZE; j++) {
                uart_transmit(buff[j]);
            }
            _delay_ms(1000);            
        } else {
            return; // Something went wrong, abort 
        }
    }
}

// two parameters are expected.
// offset - number of pages to skip.
// size - number of pages to write. if 0 is given, all size 32768 is assumed
void bulk_write() {
    uint16_t offs = receive_uint16();
    uint16_t size = receive_uint16();
    if (!size) {
        size = 32768;
    }
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < PAGE_SIZE; j++) {
            while (!uart_available());
            buff[j] = uart_receive();
        }

        uint32_t address = ((uint32_t)(offs + i)) * PAGE_SIZE;
        W25Q64FV_enable_writing();
        W25Q64FV_write_page(address, (byte*)buff);
        W25Q64FV_status_t status = W25Q64FV_wait_until_free(W25Q64FV_DEFAULT_TIMEOUT);
        if (status == W25Q64FV_OK) {
            _delay_ms(1000);
            uart_transmit(ACK);
        } else {
            uart_transmit(NACK);
            return; // Something went wrong, abort 
        }
    }
}
#endif            

