#!/usr/bin/env python3

import sys
import numpy as np


liinstruction = int.from_bytes(sys.stdin.buffer.read(4), byteorder='little')

top_lui_imm = liinstruction &   0x1fff8000
lower_lui_imm = liinstruction & 0x000003e0

value = np.uint32(sys.argv[1])

outputimm = (top_lui_imm >> 10) | (lower_lui_imm >> 5)
if outputimm == value:
    print("Success")
else:
    print("Failure")
    print("Expected: " + str(value))
    print("Actual: " + str(outputimm))

