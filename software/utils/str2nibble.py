#!/usr/bin/python3

import sys

def print_nibbles(input_string):
    for c in input_string:
        val = ord(c)
        ms = (val >> 4) & 0x0F
        ms += 0x10
        ls = val & 0x0F
        ls += 0x10
        
        # Print the hex values
        # Format as hex, ensuring two-digit hex with uppercase letters:
        #print(f"MCU, CPU - 0x{ms:02X}, ACK, 0x{ls:02X}, ACK")
        print(f"0x{ms:02X}, 0x{ls:02X}")

# Example usage:
if __name__ == "__main__":
    if len(sys.argv) > 1:
        input_str = sys.argv[1]
        print_nibbles(input_str)
    else:
        print("Usage: python script.py <string>")

