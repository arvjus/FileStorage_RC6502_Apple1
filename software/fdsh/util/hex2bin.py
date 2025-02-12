#!/usr/bin/python3

import sys
import re

def hex_to_bin(filename):
    try:
        with open(filename, 'r') as infile:
            lines = infile.readlines()

        output_filename = filename + '.bin'
        binary_data = bytearray()
        nibble = False

        for line in lines:
            # Remove comments (text after ';') and whitespace
            line = re.sub(r';.*', '', line).strip()
            
            # Skip empty lines
            if not line:
                continue

            # Split line into tokens
            tokens = line.split()
            for token in tokens:
                # Set mode
                if token == "NIBBLE:":
                    nibble = True
                elif token == "BYTE:":
                    nibble = False
                elif token.startswith("'") and token.endswith("'") and len(token) == 3:
                    # Handle character values (e.g., 'a')
                    char_value = ord(token[1])
                    if nibble:
                        binary_data.append(0x10+((char_value & 0xF0) >> 4))  # MS nibble
                        binary_data.append(0x10+(char_value & 0x0F))          # LS nibble
                    else:
                        binary_data.append(char_value)
                else:
                    try:
                        # Parse as hexadecimal
                        byte_value = int(token, 16)
                        if nibble:
                            binary_data.append(0x10+((byte_value & 0xF0) >> 4))  # MS nibble
                            binary_data.append(0x10+(byte_value & 0x0F))          # LS nibble
                        else:
                            binary_data.append(byte_value)
                    except ValueError:
                        print(f"Skipping invalid token: {token}")

        with open(output_filename, 'wb') as outfile:
            outfile.write(binary_data)

        print(f"Binary file written to {output_filename}")

    except FileNotFoundError:
        print(f"Error: File {filename} not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py <filename>")
    else:
        hex_to_bin(sys.argv[1])

