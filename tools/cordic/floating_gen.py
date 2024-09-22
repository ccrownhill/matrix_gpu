import struct

def decimal_to_float32_hex(decimal_number):
    # Pack the decimal number as a 32-bit float
    packed_float = struct.pack('>f', decimal_number)
    # Convert the packed float to a hexadecimal representation
    hex_value = packed_float.hex()
    return hex_value

def decimal_to_float32_bits(decimal_number):
    # Pack the decimal number as a 32-bit float and get its integer representation
    packed_float = struct.pack('>f', decimal_number)
    int_representation = struct.unpack('>I', packed_float)[0]
    return f'0x{int_representation:08X}'

# Example usage
if __name__ == "__main__":
    for i in range(64):
        decimal_number = 1 - (i+1)/64
        print(decimal_to_float32_bits(decimal_number))