#!/usr/bin/python3

#########################################################
# Bulk operations utility for Flash Disk storage device
# Copyright (c) 2025 Arvid Juskaitis

import serial
import struct
import time
import argparse

def main():
    # Argument parser for serial port configuration
    parser = argparse.ArgumentParser(description="Send 'E' command and wait for ACK or NACK.")
    parser.add_argument("--blocks", type=int, default=0, help="Number of 32 kb blocks to erase (default: 0 for unlimited)")
    parser.add_argument("--port", default="/dev/ttyUSB1", help="Serial port (default: /dev/ttyUSB1)")
    parser.add_argument("--baudrate", type=int, default=250000, help="Baud rate (default: 250000)")
    parser.add_argument("--timeout", type=float, default=600, help="Serial timeout in seconds (default: 600)")
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

    print("Receiving initial output from the device...")
    initial_data = ser.read_all()  # Read all available data
    if initial_data:
        print("Initial data received:")
        print(initial_data.decode('utf-8', errors='replace'))  # Decode and print
    else:
        print("No initial data received.")

    # Send the "E" command
    try:
        print(f"Size in blocks: {args.blocks}")
        print(f"WARNING! The data will be erased, type 'YES' in order to continue:")
        yes = input()
        if yes != "YES":
            return

        # Prepare and send the "R" command with size
        blocks_pack = struct.pack("<H", args.blocks)  # Little-endian 2-byte size
        ser.write(b"E" + blocks_pack)
        print(f"Sent command 'E' {blocks_pack}.")

        # Wait for response
        print("Waiting for response...")
        response = ser.read(1)  # Read 1 byte from the serial port

        if not response:
            print("Error: No response received. Timeout occurred.")
        elif response == b'\xA0':  # ACK
            print("ACK received. Command executed successfully.")
        elif response == b'\xAF':  # NACK
            print("NACK received. Command execution failed.")
        else:
            print(f"Unexpected response: {response}")

    except serial.SerialException as e:
        print(f"Error during communication: {e}")
    finally:
        ser.close()
        print(f"Serial port {args.port} closed.")

if __name__ == "__main__":
    main()

