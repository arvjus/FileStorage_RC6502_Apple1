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
    # Argument parser for optional size and output file
    parser = argparse.ArgumentParser(description="Read binary data from serial and write to a file.")
    parser.add_argument("output_file", help="Path to the output file")
    parser.add_argument("--blocks", type=int, default=0, help="Number of 32 kb blocks to read (default: 0 for unlimited)")
    parser.add_argument("--port", default="/dev/ttyUSB1", help="Serial port (default: /dev/ttyUSB1)")
    parser.add_argument("--baudrate", type=int, default=250000, help="Baud rate (default: 250000)")
    parser.add_argument("--timeout", type=float, default=None, help="Serial timeout in seconds (default: None for blocking mode)")
    args = parser.parse_args()

    # Open serial port
    try:
        ser = serial.Serial(port=args.port, baudrate=args.baudrate, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=args.timeout)
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        return

    print(f"Serial port {args.port} opened at {args.baudrate} baud.")
    time.sleep(3)

    print("Receiving initial output from the device...")
    initial_data = ser.read_all()  # Read all available data
    if initial_data:
        print("Initial data received:")
        print(initial_data.decode('utf-8', errors='replace'))  # Decode and print
    else:
        print("No initial data received.")

    # Open the output file
    try:
        with open(args.output_file, "wb") as f:
            print(f"Output file {args.output_file} opened.")

            pages = args.blocks * 128
            print(f"Size in blocks: {args.blocks}, pages: {pages}")

            # Prepare and send the "R" command with size
            pages_pack = struct.pack("<H", pages)  # Little-endian 2-byte size
            ser.write(b"R" + pages_pack)
            print(f"Sent command 'R' {pages_pack}.")

            time.sleep(3)

            # Read and write data in 256-byte chunks
            bytes_to_read = pages * 256 if pages > 0 else float("inf")  # Unlimited if size is 0
            total_bytes_read = 0

            while total_bytes_read < bytes_to_read:
                chunk_size = min(256, bytes_to_read - total_bytes_read)
                data = ser.read(chunk_size)

                if not data:  # No data received, exit loop
                    print("No more data received.")
                    break

                f.write(data)
                total_bytes_read += len(data)

                print(f"Read {len(data)} bytes, total {total_bytes_read} bytes.")

            print(f"Finished reading. Total bytes written: {total_bytes_read}.")

    except IOError as e:
        print(f"Error writing to file: {e}")
    finally:
        ser.close()
        print(f"Serial port {args.port} closed.")

if __name__ == "__main__":
    main()

