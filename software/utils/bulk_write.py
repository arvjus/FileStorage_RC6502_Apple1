#!/usr/bin/python3

#########################################################
# Bulk operations utility for Flash Disk storage device
# Copyright (c) 2025 Arvid Juskaitis

import serial
import argparse
import struct
import time
import os

def main():
    # Argument parser for file and serial port configuration
    parser = argparse.ArgumentParser(description="Stream binary data to serial with handshaking.")
    parser.add_argument("input_file", help="Path to the input binary file")
    parser.add_argument("--offset", type=int, default=0, help="Offset in terms of 32 kb blocks to start writing from (default: 0). Note this number must match the first block in image file.")
    parser.add_argument("--port", default="/dev/ttyUSB1", help="Serial port (default: /dev/ttyUSB1)")
    parser.add_argument("--baudrate", type=int, default=250000, help="Baud rate (default: 250000)")
    parser.add_argument("--timeout", type=float, default=5, help="Serial timeout in seconds (default: 5)")
    args = parser.parse_args()

    # Open serial port
    try:
        ser = serial.Serial(
            port=args.port,
            baudrate=args.baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=args.timeout
        )
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        return

    print(f"Serial port {args.port} opened at {args.baudrate} baud.")
    time.sleep(3)

    offs = args.offset * 128
    print(f"Offset in blocks: {args.offset}, pages: {offs}")

    print("Receiving initial output from the device...")
    initial_data = ser.read_all()  # Read all available data
    if initial_data:
        print("Initial data received:")
        print(initial_data.decode('utf-8', errors='replace'))  # Decode and print
    else:
        print("No initial data received.")

    # Open the input file
    try:
        file_size = os.path.getsize(args.input_file)
        if file_size % 256 != 0:
            print("Error: File size is not a multiple of 256 bytes.")
            return

        with open(args.input_file, "rb") as f:
            print(f"Input file {args.input_file} opened. Size: {file_size} bytes.")

            # Calculate the number of pages
            blocks = file_size // 32768
            pages = file_size // 256
            print(f"Number of blocks: {blocks}, pages to send: {pages}")

            # Send the "W" command with the offs, size
            offs_pack = struct.pack("<H", offs) # Little-endian 2-byte offs
            pages_pack = struct.pack("<H", pages)  # Little-endian 2-byte size
            ser.write(b"W" + offs_pack + pages_pack)
            print(f"Sent command 'W' {offs_pack}, {pages_pack}.")

            # Stream the content page by page
            for page in range(pages):
                page_data = f.read(256)  # Read a 256-byte page
                if len(page_data) != 256:
                    print(f"Error: Incomplete page read at page {page}.")
                    return

                # Send the page
                ser.write(page_data)
                print(f"Page {page + 1}/{pages} sent. Waiting for ACK...")

                # Wait for ACK or NACK
                ack = ser.read(1)  # Read 1 byte
                if not ack:
                    print("Error: No response received. Terminating transmission.")
                    return

                if ack == b'\xA0':  # ACK
                    print(f"Page {page + 1}/{pages} acknowledged.")
                elif ack == b'\xAF':  # NACK
                    print(f"Page {page + 1}/{pages} rejected. Terminating transmission.")
                    return
                else:
                    print(f"Unexpected response: {ack}. Terminating transmission.")
                    return

            print("Transmission completed successfully.")

    except IOError as e:
        print(f"Error reading the file: {e}")
    finally:
        ser.close()
        print(f"Serial port {args.port} closed.")

if __name__ == "__main__":
    main()

