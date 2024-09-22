#!/usr/bin/env python3

import sys

if len(sys.argv) != 2:
    print("Usage: python3 print_li.py <value>")
    sys.exit(1)

value = int(sys.argv[1])
print(f"li r6, {value}")